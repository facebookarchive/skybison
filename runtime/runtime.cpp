#include "runtime.h"

#include <unistd.h>
#include <cinttypes>
#include <climits>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <memory>

#include "builtins-module.h"
#include "bytearray-builtins.h"
#include "bytecode.h"
#include "bytes-builtins.h"
#include "callback.h"
#include "code-builtins.h"
#include "codecs-module.h"
#include "complex-builtins.h"
#include "debugging.h"
#include "descriptor-builtins.h"
#include "dict-builtins.h"
#include "exception-builtins.h"
#include "float-builtins.h"
#include "frame.h"
#include "frozen-modules.h"
#include "function-builtins.h"
#include "generator-builtins.h"
#include "globals.h"
#include "handles.h"
#include "heap.h"
#include "imp-module.h"
#include "int-builtins.h"
#include "interpreter.h"
#include "io-module.h"
#include "iterator-builtins.h"
#include "layout.h"
#include "list-builtins.h"
#include "marshal-module.h"
#include "marshal.h"
#include "memoryview-builtins.h"
#include "module-builtins.h"
#include "object-builtins.h"
#include "operator-module.h"
#include "os.h"
#include "range-builtins.h"
#include "ref-builtins.h"
#include "scavenger.h"
#include "set-builtins.h"
#include "siphash.h"
#include "slice-builtins.h"
#include "str-builtins.h"
#include "strarray-builtins.h"
#include "super-builtins.h"
#include "sys-module.h"
#include "thread.h"
#include "tuple-builtins.h"
#include "type-builtins.h"
#include "under-builtins-module.h"
#include "under-str-mod-module.h"
#include "utils.h"
#include "visitor.h"
#include "warnings-module.h"
#include "weakref-module.h"

namespace python {

extern "C" struct _inittab _PyImport_Inittab[];

static const SymbolId kBinaryOperationSelector[] = {
    SymbolId::kDunderAdd,     SymbolId::kDunderSub,
    SymbolId::kDunderMul,     SymbolId::kDunderMatmul,
    SymbolId::kDunderTruediv, SymbolId::kDunderFloordiv,
    SymbolId::kDunderMod,     SymbolId::kDunderDivmod,
    SymbolId::kDunderPow,     SymbolId::kDunderLshift,
    SymbolId::kDunderRshift,  SymbolId::kDunderAnd,
    SymbolId::kDunderXor,     SymbolId::kDunderOr};

static const SymbolId kSwappedBinaryOperationSelector[] = {
    SymbolId::kDunderRadd,     SymbolId::kDunderRsub,
    SymbolId::kDunderRmul,     SymbolId::kDunderRmatmul,
    SymbolId::kDunderRtruediv, SymbolId::kDunderRfloordiv,
    SymbolId::kDunderRmod,     SymbolId::kDunderRdivmod,
    SymbolId::kDunderRpow,     SymbolId::kDunderRlshift,
    SymbolId::kDunderRrshift,  SymbolId::kDunderRand,
    SymbolId::kDunderRxor,     SymbolId::kDunderRor};

static const SymbolId kInplaceOperationSelector[] = {
    SymbolId::kDunderIadd,     SymbolId::kDunderIsub,
    SymbolId::kDunderImul,     SymbolId::kDunderImatmul,
    SymbolId::kDunderItruediv, SymbolId::kDunderIfloordiv,
    SymbolId::kDunderImod,     SymbolId::kMaxId,
    SymbolId::kDunderIpow,     SymbolId::kDunderIlshift,
    SymbolId::kDunderIrshift,  SymbolId::kDunderIand,
    SymbolId::kDunderIxor,     SymbolId::kDunderIor};

static const SymbolId kComparisonSelector[] = {
    SymbolId::kDunderLt, SymbolId::kDunderLe, SymbolId::kDunderEq,
    SymbolId::kDunderNe, SymbolId::kDunderGt, SymbolId::kDunderGe};

Runtime::Runtime(word heap_size, bool cache_enabled)
    : heap_(heap_size),
      new_value_cell_callback_(this),
      cache_enabled_(cache_enabled) {
  initializeDebugging();
  initializeRandom();
  initializeThreads();
  // This must be called before initializeTypes is called. Methods in
  // initializeTypes rely on instances that are created in this method.
  initializePrimitiveInstances();
  initializeInterned();
  initializeSymbols();
  initializeTypes();
  initializeApiData();
  initializeModules();
}

Runtime::Runtime() : Runtime(64 * kMiB, false) {}
Runtime::Runtime(bool cache_enabled) : Runtime(64 * kMiB, cache_enabled) {}

Runtime::~Runtime() {
  // TODO(T30392425): This is an ugly and fragile workaround for having multiple
  // runtimes created and destroyed by a single thread.
  if (Thread::current() == nullptr) {
    CHECK(threads_ != nullptr, "the runtime does not have any threads");
    Thread::setCurrentThread(threads_);
  }
  atExit();
  freeApiHandles();
  for (Thread* thread = threads_; thread != nullptr;) {
    if (thread == Thread::current()) {
      Thread::setCurrentThread(nullptr);
    } else {
      UNIMPLEMENTED("threading");
    }
    auto prev = thread;
    thread = thread->next();
    delete prev;
  }
  threads_ = nullptr;
  delete symbols_;
}

RawObject Runtime::newBoundMethod(const Object& function, const Object& self) {
  HandleScope scope;
  BoundMethod bound_method(&scope, heap()->create<RawBoundMethod>());
  bound_method.setFunction(*function);
  bound_method.setSelf(*self);
  return *bound_method;
}

RawObject Runtime::newLayout() {
  HandleScope scope;
  Layout layout(&scope, heap()->createLayout(LayoutId::kError));
  layout.setInObjectAttributes(empty_tuple_);
  layout.setOverflowAttributes(empty_tuple_);
  layout.setAdditions(newList());
  layout.setDeletions(newList());
  layout.setNumInObjectAttributes(0);
  return *layout;
}

RawObject Runtime::layoutCreateSubclassWithBuiltins(
    LayoutId subclass_id, LayoutId superclass_id,
    View<BuiltinAttribute> attributes) {
  HandleScope scope;

  // A builtin class is special since it contains attributes that must be
  // located at fixed offsets from the start of an instance.  These attributes
  // are packed at the beginning of the layout starting at offset 0.
  Layout super_layout(&scope, layoutAt(superclass_id));
  Tuple super_attributes(&scope, super_layout.inObjectAttributes());

  // Sanity check that a subclass that has fixed attributes does inherit from a
  // superclass with attributes that are not fixed.
  for (word i = 0; i < super_attributes.length(); i++) {
    Tuple elt(&scope, super_attributes.at(i));
    AttributeInfo info(elt.at(1));
    CHECK(info.isInObject() && info.isFixedOffset(),
          "all superclass attributes must be in-object and fixed");
  }

  // Create an empty layout for the subclass
  Layout result(&scope, newLayout());
  result.setId(subclass_id);

  // Copy down all of the superclass attributes into the subclass layout
  Tuple in_object(&scope,
                  newTuple(super_attributes.length() + attributes.length()));
  super_attributes.copyTo(*in_object);
  appendBuiltinAttributes(attributes, in_object, super_attributes.length());

  // Install the in-object attributes
  result.setInObjectAttributes(*in_object);
  result.setNumInObjectAttributes(in_object.length());

  return *result;
}

void Runtime::appendBuiltinAttributes(View<BuiltinAttribute> attributes,
                                      const Tuple& dst, word start_index) {
  if (attributes.length() == 0) {
    return;
  }
  HandleScope scope;
  Tuple entry(&scope, empty_tuple_);
  for (word i = 0; i < attributes.length(); i++) {
    DCHECK((attributes.get(i).flags &
            (AttributeFlags::kInObject | AttributeFlags::kDeleted |
             AttributeFlags::kFixedOffset)) == 0,
           "flag not allowed");
    AttributeInfo info(attributes.get(i).offset,
                       attributes.get(i).flags | AttributeFlags::kInObject |
                           AttributeFlags::kFixedOffset);
    entry = newTuple(2);
    SymbolId symbol_id = attributes.get(i).name;
    if (symbol_id == SymbolId::kInvalid) {
      entry.atPut(0, NoneType::object());
    } else {
      entry.atPut(0, symbols()->at(symbol_id));
    }
    entry.atPut(1, info.asSmallInt());
    dst.atPut(start_index + i, *entry);
  }
}

RawObject Runtime::addEmptyBuiltinType(SymbolId name, LayoutId subclass_id,
                                       LayoutId superclass_id) {
  return addBuiltinType(name, subclass_id, superclass_id,
                        BuiltinsBase::kAttributes,
                        BuiltinsBase::kBuiltinMethods);
}

RawObject Runtime::addBuiltinType(SymbolId name, LayoutId subclass_id,
                                  LayoutId superclass_id,
                                  const BuiltinAttribute attrs[],
                                  const BuiltinMethod builtins[]) {
  HandleScope scope;

  // Create a class object for the subclass
  Type subclass(&scope, newType());
  subclass.setName(symbols()->at(name));

  word attrs_len = 0;
  for (word i = 0; attrs[i].name != SymbolId::kSentinelId; i++) {
    attrs_len++;
  }
  View<BuiltinAttribute> attrs_view(attrs, attrs_len);
  Layout layout(&scope, layoutCreateSubclassWithBuiltins(
                            subclass_id, superclass_id, attrs_view));

  // Assign the layout to the class
  layout.setDescribedType(*subclass);

  // Now we can create an MRO
  Tuple mro(&scope, createMro(layout, superclass_id));

  subclass.setMro(*mro);
  subclass.setInstanceLayout(*layout);
  Type superclass(&scope, typeAt(superclass_id));
  LayoutId builtin_base = attrs_len == 0 ? superclass_id : subclass_id;
  Type::Flag flags =
      static_cast<Type::Flag>(superclass.flags() & ~Type::Flag::kIsAbstract);
  subclass.setFlagsAndBuiltinBase(flags, builtin_base);

  Tuple bases(&scope, newTuple(1));
  bases.atPut(0, *superclass);
  subclass.setBases(*bases);

  // Install the layout and class
  layoutAtPut(subclass_id, *layout);

  // Add the provided methods.
  for (word i = 0; builtins[i].name != SymbolId::kSentinelId; i++) {
    const BuiltinMethod& meth = builtins[i];
    typeAddBuiltinFunction(subclass, meth.name, meth.address);
  }

  // return the class
  return *subclass;
}

RawObject Runtime::newByteArray() {
  HandleScope scope;
  ByteArray result(&scope, heap()->create<RawByteArray>());
  result.setBytes(empty_mutable_bytes_);
  result.setNumItems(0);
  return *result;
}

RawObject Runtime::newByteArrayIterator(Thread* thread,
                                        const ByteArray& bytearray) {
  HandleScope scope(thread);
  ByteArrayIterator result(&scope, heap()->create<RawByteArrayIterator>());
  result.setIterable(*bytearray);
  result.setIndex(0);
  return *result;
}

RawObject Runtime::newBytes(word length, byte fill) {
  DCHECK(length >= 0, "invalid length %ld", length);
  if (length <= SmallBytes::kMaxLength) {
    byte buffer[SmallBytes::kMaxLength];
    for (word i = 0; i < SmallBytes::kMaxLength; i++) {
      buffer[i] = fill;
    }
    return SmallBytes::fromBytes({buffer, length});
  }
  HandleScope scope;
  LargeBytes result(&scope, heap()->createLargeBytes(length));
  std::memset(reinterpret_cast<byte*>(result.address()), fill, length);
  return *result;
}

RawObject Runtime::newBytesWithAll(View<byte> array) {
  word length = array.length();
  if (length <= SmallBytes::kMaxLength) {
    return SmallBytes::fromBytes(array);
  }
  HandleScope scope;
  LargeBytes result(&scope, heap()->createLargeBytes(length));
  std::memcpy(reinterpret_cast<byte*>(result.address()), array.data(), length);
  return *result;
}

RawObject Runtime::newBytesIterator(Thread* thread, const Bytes& bytes) {
  HandleScope scope(thread);
  BytesIterator result(&scope, heap()->create<RawBytesIterator>());
  result.setIndex(0);
  result.setIterable(*bytes);
  return *result;
}

RawObject Runtime::newType() { return newTypeWithMetaclass(LayoutId::kType); }

RawObject Runtime::newTypeWithMetaclass(LayoutId metaclass_id) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Type result(&scope, heap()->createType(metaclass_id));
  Dict dict(&scope, newDict());
  result.setFlagsAndBuiltinBase(Type::Flag::kNone, LayoutId::kObject);
  result.setDict(*dict);
  result.setDoc(NoneType::object());
  result.setAbstractMethods(Unbound::object());
  return *result;
}

RawObject Runtime::classDelAttr(Thread* thread, const Object& receiver,
                                const Object& name) {
  if (!name.isStr()) {
    // TODO(T25140871): Refactor into something like:
    //     thread->throwUnexpectedTypeError(expected, actual)
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "attribute name must be a string");
  }
  HandleScope scope(thread);
  if (thread->runtime()->isCacheEnabled()) {
    Str name_str(&scope, *name);
    terminateIfUnimplementedTypeAttrCacheInvalidation(thread, name_str);
  }

  Type type(&scope, *receiver);
  // TODO(mpage): This needs to handle built-in extension types.
  if (type.isBuiltin()) {
    Str type_name(&scope, type.name());
    return thread->raiseWithFmt(
        LayoutId::kTypeError,
        "can't set attributes of built-in/extension type '%S'", &type_name);
  }

  // Check for a delete descriptor
  Type metatype(&scope, typeOf(*receiver));
  Object meta_attr(&scope, typeLookupNameInMro(thread, metatype, name));
  if (!meta_attr.isError()) {
    if (isDeleteDescriptor(thread, meta_attr)) {
      return Interpreter::callDescriptorDelete(thread, thread->currentFrame(),
                                               meta_attr, receiver);
    }
  }

  // No delete descriptor found, attempt to delete from the type dict
  Dict type_dict(&scope, type.dict());
  if (dictRemove(thread, type_dict, name).isError()) {
    Str type_name(&scope, type.name());
    return thread->raiseWithFmt(LayoutId::kAttributeError,
                                "type object '%S' has no attribute '%S'",
                                &type_name, &name);
  }

  return NoneType::object();
}

RawObject Runtime::instanceDelAttr(Thread* thread, const Object& receiver,
                                   const Object& name) {
  if (!name.isStr()) {
    // TODO(T25140871): Refactor into something like:
    //     thread->throwUnexpectedTypeError(expected, actual)
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "attribute name must be a string");
  }

  // Check for a descriptor with __delete__
  HandleScope scope(thread);
  Type type(&scope, typeOf(*receiver));
  Object type_attr(&scope, typeLookupNameInMro(thread, type, name));
  if (!type_attr.isError()) {
    if (isDeleteDescriptor(thread, type_attr)) {
      return Interpreter::callDescriptorDelete(thread, thread->currentFrame(),
                                               type_attr, receiver);
    }
  }

  // No delete descriptor found, delete from the instance
  HeapObject instance(&scope, *receiver);
  Object result(&scope, instanceDel(thread, instance, name));
  if (result.isError()) {
    Str type_name(&scope, type.name());
    return thread->raiseWithFmt(LayoutId::kAttributeError,
                                "'%S' object has no attribute '%S'", &type_name,
                                &name);
  }

  return *result;
}

RawObject Runtime::moduleDelAttr(Thread* thread, const Object& receiver,
                                 const Object& name) {
  if (!name.isStr()) {
    // TODO(T25140871): Refactor into something like:
    //     thread->throwUnexpectedTypeError(expected, actual)
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "attribute name must be a string");
  }

  // Check for a descriptor with __delete__
  HandleScope scope(thread);
  Type type(&scope, typeOf(*receiver));
  Object type_attr(&scope, typeLookupNameInMro(thread, type, name));
  if (!type_attr.isError()) {
    if (isDeleteDescriptor(thread, type_attr)) {
      return Interpreter::callDescriptorDelete(thread, thread->currentFrame(),
                                               type_attr, receiver);
    }
  }

  // No delete descriptor found, attempt to delete from the module dict
  Module module(&scope, *receiver);
  Dict module_dict(&scope, module.dict());
  if (dictRemove(thread, module_dict, name).isError()) {
    Str module_name(&scope, module.name());
    return thread->raiseWithFmt(LayoutId::kAttributeError,
                                "module '%S' has no attribute '%S'",
                                &module_name, &name);
  }

  return NoneType::object();
}

void Runtime::seedRandom(const uword random_state[2],
                         const uword hash_secret[2]) {
  random_state_[0] = random_state[0];
  random_state_[1] = random_state[1];
  hash_secret_[0] = hash_secret[0];
  hash_secret_[1] = hash_secret[1];
}

bool Runtime::isCallable(Thread* thread, const Object& obj) {
  HandleScope scope(thread);
  if (obj.isFunction() || obj.isBoundMethod() || obj.isType()) {
    return true;
  }
  Type type(&scope, typeOf(*obj));
  return !typeLookupSymbolInMro(thread, type, SymbolId::kDunderCall).isError();
}

bool Runtime::isDeleteDescriptor(Thread* thread, const Object& object) {
  // TODO(T25692962): Track "descriptorness" through a bit on the class
  HandleScope scope(thread);
  Type type(&scope, typeOf(*object));
  return !typeLookupSymbolInMro(thread, type, SymbolId::kDunderDelete)
              .isError();
}

bool Runtime::isMapping(Thread* thread, const Object& obj) {
  if (obj.isDict()) return true;
  HandleScope scope(thread);
  Type type(&scope, typeOf(*obj));
  return !typeLookupSymbolInMro(thread, type, SymbolId::kDunderGetitem)
              .isError();
}

bool Runtime::isSequence(Thread* thread, const Object& obj) {
  if (isInstanceOfDict(*obj)) {
    return false;
  }
  HandleScope scope(thread);
  Type type(&scope, typeOf(*obj));
  return !typeLookupSymbolInMro(thread, type, SymbolId::kDunderGetitem)
              .isError();
}

RawObject Runtime::newCode(word argcount, word posonlyargcount,
                           word kwonlyargcount, word nlocals, word stacksize,
                           word flags, const Object& code, const Object& consts,
                           const Object& names, const Object& varnames,
                           const Object& freevars, const Object& cellvars,
                           const Object& filename, const Object& name,
                           word firstlineno, const Object& lnotab) {
  DCHECK(code.isInt() || isInstanceOfBytes(*code), "code must be bytes or int");
  DCHECK(isInstanceOfTuple(*consts), "expected tuple");
  DCHECK(isInstanceOfTuple(*names), "expected tuple");
  DCHECK(isInstanceOfTuple(*varnames), "expected tuple");
  DCHECK(isInstanceOfTuple(*freevars), "expected tuple");
  DCHECK(isInstanceOfTuple(*cellvars), "expected tuple");
  DCHECK(isInstanceOfStr(*filename), "expected str");
  DCHECK(isInstanceOfStr(*name), "expected str");
  DCHECK(isInstanceOfBytes(*lnotab), "expected bytes");
  DCHECK(argcount >= 0, "argcount must not be negative");
  DCHECK(posonlyargcount >= 0, "posonlyargcount must not be negative");
  DCHECK(kwonlyargcount >= 0, "kwonlyargcount must not be negative");
  DCHECK(nlocals >= 0, "nlocals must not be negative");

  Thread* thread = Thread::current();
  HandleScope scope(thread);

  Tuple cellvars_tuple(&scope, tupleUnderlying(thread, cellvars));
  Tuple freevars_tuple(&scope, tupleUnderlying(thread, freevars));
  if (cellvars_tuple.length() == 0 && freevars_tuple.length() == 0) {
    flags |= Code::Flags::NOFREE;
  } else {
    flags &= ~Code::Flags::NOFREE;
  }

  if (kwonlyargcount == 0 && (flags & Code::Flags::NOFREE) &&
      !(flags & (Code::Flags::VARARGS | Code::Flags::VARKEYARGS))) {
    // Set up shortcut for detecting fast case for calls
    flags |= Code::Flags::SIMPLE_CALL;
  }

  Code result(&scope, heap()->create<RawCode>());
  result.setArgcount(argcount);
  result.setPosonlyargcount(posonlyargcount);
  result.setKwonlyargcount(kwonlyargcount);
  result.setNlocals(nlocals);
  result.setStacksize(stacksize);
  result.setFlags(flags);
  result.setCode(*code);
  result.setConsts(*consts);
  result.setNames(*names);
  result.setVarnames(*varnames);
  result.setFreevars(*freevars);
  result.setCellvars(*cellvars);
  result.setFilename(*filename);
  result.setName(*name);
  result.setFirstlineno(firstlineno);
  result.setLnotab(*lnotab);

  Tuple varnames_tuple(&scope, tupleUnderlying(thread, varnames));
  if (argcount > varnames_tuple.length() ||
      kwonlyargcount > varnames_tuple.length() ||
      result.totalArgs() > varnames_tuple.length()) {
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "code: varnames is too small");
  }

  strInternInTuple(thread, names);
  strInternInTuple(thread, varnames);
  strInternInTuple(thread, freevars);
  strInternInTuple(thread, cellvars);
  strInternConstants(thread, consts);

  // Create mapping between cells and arguments if needed
  if (result.numCellvars() > 0) {
    Tuple cell2arg(&scope, newTuple(result.numCellvars()));
    bool value_set = false;
    for (word i = 0; i < result.numCellvars(); i++) {
      for (word j = 0; j < result.totalArgs(); j++) {
        if (Tuple::cast(*cellvars).at(i) == Tuple::cast(*varnames).at(j)) {
          cell2arg.atPut(i, newInt(j));
          value_set = true;
        }
      }
    }
    if (value_set) result.setCell2arg(*cell2arg);
  }

  DCHECK(result.totalArgs() <= result.nlocals(), "invalid nlocals count");
  return *result;
}

RawObject Runtime::newBuiltinCode(word argcount, word posonlyargcount,
                                  word kwonlyargcount, word flags,
                                  Function::Entry entry,
                                  const Object& parameter_names,
                                  const Object& name_str) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Tuple empty_tuple(&scope, emptyTuple());
  Object empty_string(&scope, Str::empty());
  Object lnotab(&scope, Bytes::empty());
  word nlocals = argcount + kwonlyargcount +
                 ((flags & Code::Flags::VARARGS) != 0) +
                 ((flags & Code::Flags::VARKEYARGS) != 0);
  flags |= Code::Flags::OPTIMIZED | Code::Flags::NEWLOCALS;
  Object entry_ptr(&scope, newIntFromCPtr(bit_cast<void*>(entry)));
  return newCode(argcount, posonlyargcount, kwonlyargcount, nlocals,
                 /*stacksize=*/0, flags, entry_ptr, /*consts=*/empty_tuple,
                 /*names=*/empty_tuple,
                 /*varnames=*/parameter_names, /*freevars=*/empty_tuple,
                 /*cellvars=*/empty_tuple, /*filename=*/empty_string, name_str,
                 /*firstlineno=*/0, lnotab);
}

RawObject Runtime::newFunction(Thread* thread, const Object& name,
                               const Object& code, word flags, word argcount,
                               word total_args, word total_vars, word stacksize,
                               Function::Entry entry, Function::Entry entry_kw,
                               Function::Entry entry_ex) {
  DCHECK(isInstanceOfStr(*name), "expected str");

  HandleScope scope(thread);
  Function function(&scope, heap()->create<RawFunction>());
  function.setCode(*code);
  function.setFlags(flags);
  function.setArgcount(argcount);
  function.setTotalArgs(total_args);
  function.setTotalVars(total_vars);
  function.setStacksize(stacksize);
  function.setName(*name);
  function.setQualname(*name);
  function.setEntry(entry);
  function.setEntryKw(entry_kw);
  function.setEntryEx(entry_ex);
  return *function;
}

RawObject Runtime::newFunctionWithCode(Thread* thread, const Object& qualname,
                                       const Code& code,
                                       const Object& globals_dict) {
  HandleScope scope(thread);

  Function::Entry entry;
  Function::Entry entry_kw;
  Function::Entry entry_ex;
  word flags = code.flags();
  word stacksize = code.stacksize();
  if (!code.hasOptimizedAndNewLocals()) {
    // We do not support calling non-optimized functions directly. We only allow
    // them in Thread::exec() and Thread::runClassFunction().
    entry = unimplementedTrampoline;
    entry_kw = unimplementedTrampoline;
    entry_ex = unimplementedTrampoline;
  } else if (code.isNative()) {
    entry = builtinTrampoline;
    entry_kw = builtinTrampolineKw;
    entry_ex = builtinTrampolineEx;
    DCHECK(stacksize == 0, "expected zero stacksize");
  } else if (code.isGeneratorLike()) {
    if (code.hasFreevarsOrCellvars()) {
      entry = generatorClosureTrampoline;
      entry_kw = generatorClosureTrampolineKw;
      entry_ex = generatorClosureTrampolineEx;
    } else {
      entry = generatorTrampoline;
      entry_kw = generatorTrampolineKw;
      entry_ex = generatorTrampolineEx;
    }
    // HACK: Reserve one extra stack slot for the case where we need to unwrap a
    // bound method.
    stacksize++;
  } else {
    if (code.hasFreevarsOrCellvars()) {
      entry = interpreterClosureTrampoline;
      entry_kw = interpreterClosureTrampolineKw;
      entry_ex = interpreterClosureTrampolineEx;
    } else {
      entry = interpreterTrampoline;
      entry_kw = interpreterTrampolineKw;
      entry_ex = interpreterTrampolineEx;
    }
    flags |= Function::Flags::kInterpreted;
    // HACK: Reserve one extra stack slot for the case where we need to unwrap a
    // bound method.
    stacksize++;
  }
  Object name(&scope, code.name());
  word total_args = code.totalArgs();
  word total_vars =
      code.nlocals() - total_args + code.numCellvars() + code.numFreevars();

  Function function(&scope, newFunction(thread, name, code, flags,
                                        code.argcount(), total_args, total_vars,
                                        stacksize, entry, entry_kw, entry_ex));

  DCHECK(isInstanceOfStr(*qualname), "expected str");
  function.setQualname(*qualname);

  if (!globals_dict.isNoneType()) {
    Dict globals(&scope, *globals_dict);
    Object dunder_name(&scope, symbols()->at(SymbolId::kDunderName));
    Object value_cell(&scope, dictAt(thread, globals, dunder_name));
    if (value_cell.isValueCell()) {
      DCHECK(!ValueCell::cast(*value_cell).isUnbound(), "unbound globals");
      function.setModule(ValueCell::cast(*value_cell).value());
    }
    function.setGlobals(*globals);
  } else {
    DCHECK(code.isNative(), "Only native code may have no globals");
  }

  Object consts_obj(&scope, code.consts());
  if (consts_obj.isTuple()) {
    Tuple consts(&scope, *consts_obj);
    if (consts.length() >= 1 && consts.at(0).isStr()) {
      function.setDoc(consts.at(0));
    }
  }

  if (!code.isNative()) {
    Bytes bytecode(&scope, code.code());
    function.setRewrittenBytecode(mutableBytesFromBytes(thread, bytecode));
    function.setCaches(emptyTuple());
    function.setOriginalArguments(emptyTuple());
    if (isCacheEnabled()) {
      // TODO(T45382423): Move this into a separate function to be called by a
      // relevant opcode during opcode execution.
      rewriteBytecode(thread, function);
    }
  }
  return *function;
}

RawObject Runtime::newFunctionWithCustomEntry(
    Thread* thread, const Object& name, const Object& code,
    Function::Entry entry, Function::Entry entry_kw, Function::Entry entry_ex) {
  DCHECK(!code.isCode(), "Use newFunctionWithCode() for code objects");
  DCHECK(code.isInt(), "expected int");
  HandleScope scope(thread);
  Function function(&scope, newFunction(thread, name, code, /*flags=*/0,
                                        /*argcount=*/0, /*total_args=*/0,
                                        /*total_vars=*/0, /*stacksize=*/0,
                                        entry, entry_kw, entry_ex));
  return *function;
}

RawObject Runtime::newExceptionState() {
  return heap()->create<RawExceptionState>();
}

RawObject Runtime::newAsyncGenerator() {
  return heap()->create<RawAsyncGenerator>();
}

RawObject Runtime::newCoroutine() { return heap()->create<RawCoroutine>(); }

RawObject Runtime::newGenerator() { return heap()->create<RawGenerator>(); }

RawObject Runtime::newHeapFrame(const Function& function) {
  DCHECK(function.isGeneratorLike(), "expected a generator-like code object");

  HandleScope scope;
  word num_args = function.totalArgs();
  word num_vars = function.totalVars();
  word stacksize = function.stacksize();
  // +1 for the function pointer.
  word extra_words = num_args + num_vars + stacksize + 1;
  HeapFrame frame(
      &scope, heap()->createInstance(LayoutId::kHeapFrame,
                                     HeapFrame::numAttributes(extra_words)));
  frame.setMaxStackSize(stacksize);
  return *frame;
}

RawObject Runtime::newInstance(const Layout& layout) {
  // This takes into account the potential overflow pointer.
  word num_attrs = layout.instanceSize() / kPointerSize;
  RawObject object = heap()->createInstance(layout.id(), num_attrs);
  RawHeapObject instance = HeapObject::cast(object);
  // Set the overflow array
  instance.instanceVariableAtPut(layout.overflowOffset(), empty_tuple_);
  return instance;
}

RawObject Runtime::newQualname(Thread* thread, const Type& type,
                               SymbolId name) {
  HandleScope scope(thread);
  Str type_name(&scope, type.name());
  return newStrFromFmt("%S.%Y", &type_name, name);
}

void Runtime::typeAddBuiltinFunction(const Type& type, SymbolId name,
                                     Function::Entry entry) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Str qualname(&scope, newQualname(thread, type, name));
  Str name_str(&scope, symbols()->at(name));
  Tuple empty_tuple(&scope, emptyTuple());
  Code code(&scope, newBuiltinCode(/*argcount=*/0, /*posonlyargcount=*/0,
                                   /*kwonlyargcount=*/0,
                                   /*flags=*/0, entry,
                                   /*parameter_names=*/empty_tuple, name_str));

  Object globals(&scope, NoneType::object());
  Function function(&scope,
                    newFunctionWithCode(thread, qualname, code, globals));

  Dict dict(&scope, type.dict());
  typeDictAtPut(thread, dict, name_str, function);
}

RawObject Runtime::newList() {
  HandleScope scope;
  List result(&scope, heap()->create<RawList>());
  result.setNumItems(0);
  result.setItems(empty_tuple_);
  return *result;
}

RawObject Runtime::newListIterator(const Object& list) {
  HandleScope scope;
  ListIterator list_iterator(&scope, heap()->create<RawListIterator>());
  list_iterator.setIndex(0);
  list_iterator.setIterable(*list);
  return *list_iterator;
}

RawObject Runtime::newSeqIterator(const Object& sequence) {
  HandleScope scope;
  SeqIterator iter(&scope, heap()->create<RawSeqIterator>());
  iter.setIndex(0);
  iter.setIterable(*sequence);
  return *iter;
}

RawObject Runtime::newModule(const Object& name) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Module result(&scope, heap()->create<RawModule>());
  Dict dict(&scope, newDict());
  result.setDict(*dict);
  result.setName(*name);
  result.setDef(newIntFromCPtr(nullptr));
  Object key(&scope, symbols()->DunderName());
  moduleAtPut(thread, result, key, name);

  Object none(&scope, NoneType::object());
  Object doc_key(&scope, symbols()->DunderDoc());
  moduleAtPut(thread, result, doc_key, none);
  Object package_key(&scope, symbols()->DunderPackage());
  moduleAtPut(thread, result, package_key, none);
  Object loader_key(&scope, symbols()->DunderLoader());
  moduleAtPut(thread, result, loader_key, none);
  Object spec_key(&scope, symbols()->DunderSpec());
  moduleAtPut(thread, result, spec_key, none);

  return *result;
}

RawObject Runtime::newMemoryView(Thread* thread, const Object& buffer,
                                 word length, ReadOnly read_only) {
  HandleScope scope(thread);
  MemoryView result(&scope, heap()->create<RawMemoryView>());
  result.setBuffer(*buffer);
  result.setLength(length);
  result.setFormat(RawSmallStr::fromCodePoint('B'));
  result.setReadOnly(read_only == ReadOnly::ReadOnly);
  return *result;
}

RawObject Runtime::newMemoryViewFromCPtr(Thread* thread, void* ptr, word length,
                                         ReadOnly read_only) {
  HandleScope scope(thread);
  Object buffer(&scope, newIntFromCPtr(ptr));
  return newMemoryView(thread, buffer, length, read_only);
}

RawObject Runtime::newMutableBytesUninitialized(word size) {
  if (size == 0) {
    return empty_mutable_bytes_;
  }
  return heap()->createMutableBytes(size);
}

RawObject Runtime::mutableBytesFromBytes(Thread* thread, const Bytes& bytes) {
  HandleScope scope(thread);
  word len = bytes.length();
  MutableBytes mb(&scope, heap()->createMutableBytes(len));
  bytes.copyTo(reinterpret_cast<byte*>(mb.address()), len);
  return *mb;
}

RawObject Runtime::mutableBytesWith(word length, byte value) {
  if (length == 0) return empty_mutable_bytes_;
  DCHECK(length > 0, "invalid length %ld", length);
  HandleScope scope;
  MutableBytes result(&scope, heap()->createMutableBytes(length));
  std::memset(reinterpret_cast<byte*>(result.address()), value, length);
  return *result;
}

RawObject Runtime::newIntFromCPtr(void* ptr) {
  return newInt(reinterpret_cast<word>(ptr));
}

RawObject Runtime::emptyMutableBytes() { return empty_mutable_bytes_; }

RawObject Runtime::emptySlice() { return empty_slice_; }

RawObject Runtime::emptyTuple() { return empty_tuple_; }

RawObject Runtime::newTuple(word length) {
  if (length == 0) {
    return emptyTuple();
  }
  return heap()->createTuple(length);
}

RawObject Runtime::newInt(word value) {
  if (SmallInt::isValid(value)) {
    return SmallInt::fromWord(value);
  }
  uword digit[1] = {static_cast<uword>(value)};
  return newIntWithDigits(digit);
}

RawObject Runtime::newIntFromUnsigned(uword value) {
  if (static_cast<word>(value) >= 0 && SmallInt::isValid(value)) {
    return SmallInt::fromWord(value);
  }
  uword digits[] = {value, 0};
  View<uword> view(digits, digits[0] >> (kBitsPerWord - 1) ? 2 : 1);
  return newIntWithDigits(view);
}

RawObject Runtime::newFloat(double value) {
  return Float::cast(heap()->createFloat(value));
}

RawObject Runtime::newComplex(double real, double imag) {
  return Complex::cast(heap()->createComplex(real, imag));
}

RawObject Runtime::newIntWithDigits(View<uword> digits) {
  if (digits.length() == 0) {
    return SmallInt::fromWord(0);
  }
  if (digits.length() == 1) {
    word digit = static_cast<word>(digits.get(0));
    if (SmallInt::isValid(digit)) {
      return SmallInt::fromWord(digit);
    }
  }
  HandleScope scope;
  LargeInt result(&scope, heap()->createLargeInt(digits.length()));
  for (word i = 0; i < digits.length(); i++) {
    result.digitAtPut(i, digits.get(i));
  }
  DCHECK(result.isValid(), "Invalid digits");
  return *result;
}

RawObject Runtime::newProperty(const Object& getter, const Object& setter,
                               const Object& deleter) {
  HandleScope scope;
  Property new_prop(&scope, heap()->create<RawProperty>());
  new_prop.setGetter(*getter);
  new_prop.setSetter(*setter);
  new_prop.setDeleter(*deleter);
  return *new_prop;
}

RawObject Runtime::newRange(const Object& start, const Object& stop,
                            const Object& step) {
  HandleScope scope;
  Range result(&scope, heap()->create<RawRange>());
  result.setStart(*start);
  result.setStop(*stop);
  result.setStep(*step);
  return *result;
}

RawObject Runtime::newLongRangeIterator(const Int& start, const Int& stop,
                                        const Int& step) {
  HandleScope scope;
  LongRangeIterator result(&scope, heap()->create<RawLongRangeIterator>());
  result.setNext(*start);
  result.setStop(*stop);
  result.setStep(*step);
  return *result;
}

RawObject Runtime::newRangeIterator(word start, word step, word length) {
  HandleScope scope;
  RangeIterator result(&scope, heap()->create<RawRangeIterator>());
  result.setNext(start);
  result.setStep(step);
  result.setLength(length);
  return *result;
}

RawObject Runtime::newSetIterator(const Object& set) {
  HandleScope scope;
  SetIterator result(&scope, heap()->create<RawSetIterator>());
  result.setIterable(*set);
  result.setIndex(SetBase::Bucket::kFirst);
  result.setConsumedCount(0);
  return *result;
}

RawObject Runtime::newSlice(const Object& start, const Object& stop,
                            const Object& step) {
  if (start.isNoneType() && stop.isNoneType() && step.isNoneType()) {
    return emptySlice();
  }
  HandleScope scope;
  Slice slice(&scope, heap()->create<RawSlice>());
  slice.setStart(*start);
  slice.setStop(*stop);
  slice.setStep(*step);
  return *slice;
}

RawObject Runtime::newStaticMethod() {
  return heap()->create<RawStaticMethod>();
}

RawObject Runtime::newStrArray() {
  HandleScope scope;
  StrArray result(&scope, heap()->create<RawStrArray>());
  result.setItems(empty_mutable_bytes_);
  result.setNumItems(0);
  return *result;
}

RawObject Runtime::newStrFromByteArray(const ByteArray& array) {
  word length = array.numItems();
  if (length <= SmallStr::kMaxLength) {
    byte buffer[SmallStr::kMaxLength];
    array.copyTo(buffer, length);
    return SmallStr::fromBytes({buffer, length});
  }
  HandleScope scope;
  LargeStr result(&scope, heap()->createLargeStr(length));
  byte* dst = reinterpret_cast<byte*>(result.address());
  array.copyTo(dst, length);
  return *result;
}

RawObject Runtime::newStrFromCStr(const char* c_str) {
  word length = std::strlen(c_str);
  auto data = reinterpret_cast<const byte*>(c_str);
  return newStrWithAll(View<byte>(data, length));
}

RawObject Runtime::strFromStrArray(const StrArray& array) {
  word length = array.numItems();
  if (length <= SmallStr::kMaxLength) {
    byte buffer[SmallStr::kMaxLength];
    array.copyTo(buffer, length);
    return SmallStr::fromBytes({buffer, length});
  }
  HandleScope scope;
  LargeStr result(&scope, heap()->createLargeStr(length));
  array.copyTo(reinterpret_cast<byte*>(result.address()), length);
  return *result;
}

RawObject Runtime::strFormat(Thread* thread, char* dst, word size,
                             const Str& fmt, va_list args) {
  word dst_idx = 0;
  word len = 0;
  HandleScope scope(thread);
  DCHECK((dst == nullptr) == (size == 0), "dst must be null when size is 0");
  for (word fmt_idx = 0; fmt_idx < fmt.charLength(); fmt_idx++, len++) {
    if (fmt.charAt(fmt_idx) != '%') {
      if (dst != nullptr) {
        dst[dst_idx++] = fmt.charAt(fmt_idx);
      }
      continue;
    }
    if (++fmt_idx >= fmt.charLength()) {
      return thread->raiseWithFmt(LayoutId::kValueError, "Incomplete format");
    }
    switch (fmt.charAt(fmt_idx)) {
      case 'd': {
        int value = va_arg(args, int);
        if (dst == nullptr) {
          len--;
          len += snprintf(nullptr, 0, "%d", value);
        } else {
          dst_idx +=
              std::snprintf(&dst[dst_idx], size - dst_idx + 1, "%d", value);
        }
      } break;
      case 'g': {
        double value = va_arg(args, double);
        if (dst == nullptr) {
          len--;
          len += std::snprintf(nullptr, 0, "%g", value);
        } else {
          dst_idx +=
              std::snprintf(&dst[dst_idx], size - dst_idx + 1, "%g", value);
        }
      } break;
      case 's': {
        const char* value = va_arg(args, char*);
        if (dst == nullptr) {
          len--;
          len += std::strlen(value);
        } else {
          word length = std::strlen(value);
          std::memcpy(reinterpret_cast<byte*>(&dst[dst_idx]), value, length);
          dst_idx += length;
        }
      } break;
      case 'w': {
        word value = va_arg(args, word);
        if (dst == nullptr) {
          len--;
          len += std::snprintf(nullptr, 0, "%" PRIdPTR, value);
        } else {
          dst_idx += std::snprintf(&dst[dst_idx], size - dst_idx + 1,
                                   "%" PRIdPTR, value);
        }
      } break;
      case 'S': {
        Str value(&scope, **va_arg(args, Object*));
        word length = value.charLength();
        if (dst == nullptr) {
          len--;
          len += length;
        } else {
          value.copyTo(reinterpret_cast<byte*>(&dst[dst_idx]), length);
          dst_idx += length;
        }
      } break;
      case 'F': {
        Object obj(&scope, **va_arg(args, Object*));
        Function function(&scope, *obj);
        Str value(&scope, function.qualname());
        word length = value.charLength();
        if (dst == nullptr) {
          len--;
          len += length;
        } else {
          value.copyTo(reinterpret_cast<byte*>(&dst[dst_idx]), length);
          dst_idx += length;
        }
      } break;
      case 'T': {
        Object obj(&scope, **va_arg(args, Object*));
        Type type(&scope, typeOf(*obj));
        Str value(&scope, type.name());
        word length = value.charLength();
        if (dst == nullptr) {
          len--;
          len += length;
        } else {
          value.copyTo(reinterpret_cast<byte*>(&dst[dst_idx]), length);
          dst_idx += length;
        }
      } break;
      case 'Y': {
        SymbolId value = va_arg(args, SymbolId);
        Str value_str(&scope, symbols()->at(value));
        word length = value_str.charLength();
        if (dst == nullptr) {
          len--;
          len += length;
        } else {
          value_str.copyTo(reinterpret_cast<byte*>(&dst[dst_idx]), length);
          dst_idx += length;
        }
      } break;
      case '%':
        break;
      default:
        UNIMPLEMENTED("Unsupported format specifier");
    }
  }
  if (dst != nullptr) {
    dst[size] = '\0';
  }
  if (!SmallInt::isValid(len)) {
    return thread->raiseWithFmt(LayoutId::kOverflowError,
                                "Output of format string is too long");
  }
  return SmallInt::fromWord(len);
}

RawObject Runtime::newStrFromFmtV(Thread* thread, const char* fmt,
                                  va_list args) {
  va_list args_copy;
  va_copy(args_copy, args);
  HandleScope scope(thread);
  Str fmt_str(&scope, newStrFromCStr(fmt));
  Object out_len(&scope, strFormat(thread, nullptr, 0, fmt_str, args));
  if (out_len.isError()) return *out_len;
  word len = SmallInt::cast(*out_len).value();
  unique_c_ptr<char> dst(static_cast<char*>(std::malloc(len + 1)));
  CHECK(dst != nullptr, "Buffer allocation failure");
  strFormat(thread, dst.get(), len, fmt_str, args_copy);
  va_end(args_copy);
  return newStrFromCStr(dst.get());
}

RawObject Runtime::newStrFromFmt(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object result(&scope, newStrFromFmtV(Thread::current(), fmt, args));
  va_end(args);
  return *result;
}

RawObject Runtime::newStrFromUTF32(View<int32_t> code_units) {
  word size = 0;
  for (word i = 0; i < code_units.length(); ++i) {
    int32_t cp = code_units.get(i);
    if (cp <= kMaxASCII) {
      size += 1;
    } else if (cp < 0x0800) {
      size += 2;
    } else if (cp < 0x010000) {
      size += 3;
    } else {
      DCHECK(cp <= kMaxUnicode, "invalid codepoint");
      size += 4;
    }
  }
  if (size <= RawSmallStr::kMaxLength) {
    byte dst[SmallStr::kMaxLength];
    for (word i = 0, j = 0; i < code_units.length(); ++i) {
      RawStr src = Str::cast(SmallStr::fromCodePoint(code_units.get(i)));
      word num_bytes = src.charLength();
      src.copyTo(&dst[j], num_bytes);
      j += num_bytes;
    }
    return SmallStr::fromBytes(View<byte>(dst, size));
  }
  RawObject result = heap()->createLargeStr(size);
  DCHECK(!result.isError(), "failed to create large string");
  byte* dst = reinterpret_cast<byte*>(LargeStr::cast(result).address());
  if (code_units.length() == size) {
    // ASCII fastpath
    for (word i = 0; i < size; ++i) {
      dst[i] = code_units.get(i);
    }
    return result;
  }
  for (word i = 0, j = 0; i < code_units.length(); ++i) {
    RawStr src = Str::cast(SmallStr::fromCodePoint(code_units.get(i)));
    word num_bytes = src.charLength();
    src.copyTo(&dst[j], num_bytes);
    j += num_bytes;
  }
  return result;
}

RawObject Runtime::newStrWithAll(View<byte> code_units) {
  word length = code_units.length();
  if (length <= RawSmallStr::kMaxLength) {
    return SmallStr::fromBytes(code_units);
  }
  RawObject result = heap()->createLargeStr(length);
  DCHECK(!result.isError(), "failed to create large string");
  byte* dst = reinterpret_cast<byte*>(LargeStr::cast(result).address());
  const byte* src = code_units.data();
  memcpy(dst, src, length);
  return result;
}

RawObject Runtime::internStrFromCStr(Thread* thread, const char* c_str) {
  HandleScope scope(thread);
  // TODO(T29648342): Optimize lookup to avoid creating an intermediary Str
  Object str(&scope, newStrFromCStr(c_str));
  return internStr(thread, str);
}

RawObject Runtime::internStr(Thread* thread, const Object& str) {
  HandleScope scope(thread);
  Set set(&scope, interned());
  DCHECK(str.isStr(), "not a string");
  if (str.isSmallStr()) {
    return *str;
  }
  Object key_hash(&scope, hash(*str));
  return setAddWithHash(thread, set, str, key_hash);
}

bool Runtime::isInternedStr(Thread* thread, const Object& str) {
  if (str.isSmallStr()) {
    return true;
  }
  DCHECK(str.isLargeStr(), "expected small or large str");
  HandleScope scope(thread);
  Set set(&scope, interned());
  Tuple data(&scope, set.data());
  Object str_hash(&scope, hash(*str));
  word index = setLookup<SetLookupType::Lookup>(data, str, str_hash);
  if (index < 0) {
    return false;
  }
  return SetBase::Bucket::key(*data, index) == str;
}

RawObject Runtime::hash(RawObject object) {
  if (!object.isHeapObject()) {
    return immediateHash(object);
  }
  if (object.isLargeBytes() || object.isLargeStr()) {
    return valueHash(object);
  }
  return identityHash(object);
}

RawObject Runtime::immediateHash(RawObject object) {
  if (object.isSmallInt()) {
    return object;
  }
  if (object.isBool()) {
    return convertBoolToInt(object);
  }
  if (object.isSmallBytes() || object.isSmallStr()) {
    return SmallInt::fromWord(object.raw() >> Object::kImmediateTagBits);
  }
  return SmallInt::fromWord(object.raw());
}

// Xoroshiro128+
// http://xoroshiro.di.unimi.it/
uword Runtime::random() {
  const uint64_t s0 = random_state_[0];
  uint64_t s1 = random_state_[1];
  const uint64_t result = s0 + s1;
  s1 ^= s0;
  random_state_[0] = Utils::rotateLeft(s0, 55) ^ s1 ^ (s1 << 14);
  random_state_[1] = Utils::rotateLeft(s1, 36);
  return result;
}

void Runtime::setArgv(Thread* thread, int argc, const char** argv) {
  HandleScope scope(thread);
  List list(&scope, newList());
  CHECK(argc >= 1, "Unexpected argc");
  for (int i = 1; i < argc; i++) {  // skip program name (i.e. "python")
    Object arg_val(&scope, newStrFromCStr(argv[i]));
    listAdd(thread, list, arg_val);
  }

  Object module_name(&scope, symbols()->Sys());
  Module sys_module(&scope, findModule(module_name));
  Object argv_value(&scope, *list);
  moduleAtPutById(thread, sys_module, SymbolId::kArgv, argv_value);
}

bool Runtime::listEntryInsert(ListEntry* entry, ListEntry** root) {
  // If already tracked, do nothing.
  if (entry->prev != nullptr || entry->next != nullptr || entry == *root) {
    return false;
  }
  entry->prev = nullptr;
  entry->next = *root;
  if (*root != nullptr) {
    (*root)->prev = entry;
  }
  *root = entry;
  return true;
}

bool Runtime::listEntryRemove(ListEntry* entry, ListEntry** root) {
  // The node is the first node of the list.
  if (*root == entry) {
    *root = entry->next;
  } else if (entry->prev == nullptr && entry->next == nullptr) {
    // This is an already untracked object.
    return false;
  }
  if (entry->prev != nullptr) {
    entry->prev->next = entry->next;
  }
  if (entry->next != nullptr) {
    entry->next->prev = entry->prev;
  }
  entry->prev = nullptr;
  entry->next = nullptr;
  return true;
}

bool Runtime::trackObject(ListEntry* entry) {
  return listEntryInsert(entry, &tracked_objects_);
}

bool Runtime::untrackObject(ListEntry* entry) {
  return listEntryRemove(entry, &tracked_objects_);
}

bool Runtime::trackNativeObject(void* native) {
  // This is an already untracked object.
  if (reinterpret_cast<PyObject*>(native)->reference_ != nullptr) {
    return false;
  }
  NativeObjectNode* entry = reinterpret_cast<NativeObjectNode*>(
      std::malloc(sizeof(NativeObjectNode)));
  entry->prev = nullptr;
  entry->next = nullptr;
  entry->native_ptr = native;
  reinterpret_cast<PyObject*>(native)->reference_ = entry;
  return listEntryInsert(entry, &tracked_native_objects_);
}

bool Runtime::untrackNativeObject(void* native) {
  ListEntry* entry = reinterpret_cast<ListEntry*>(
      reinterpret_cast<PyObject*>(native)->reference_);
  if (!listEntryRemove(entry, &tracked_native_objects_)) {
    return false;
  }
  std::free(entry);
  return true;
}

RawObject Runtime::identityHash(RawObject object) {
  RawHeapObject src = HeapObject::cast(object);
  word code = src.header().hashCode();
  if (code == RawHeader::kUninitializedHash) {
    code = random() & RawHeader::kHashCodeMask;
    code = (code == RawHeader::kUninitializedHash) ? code + 1 : code;
    src.setHeader(src.header().withHashCode(code));
  }
  return SmallInt::fromWord(code);
}

word Runtime::siphash24(View<byte> array) {
  word result = 0;
  ::halfsiphash(array.data(), array.length(),
                reinterpret_cast<const uint8_t*>(hash_secret_),
                reinterpret_cast<uint8_t*>(&result), sizeof(result));
  return result;
}

RawObject Runtime::valueHash(RawObject object) {
  RawHeapObject src = HeapObject::cast(object);
  RawHeader header = src.header();
  word code = header.hashCode();
  if (code == RawHeader::kUninitializedHash) {
    word size = src.headerCountOrOverflow();
    code = siphash24(View<byte>(reinterpret_cast<byte*>(src.address()), size));
    code &= RawHeader::kHashCodeMask;
    code = (code == RawHeader::kUninitializedHash) ? code + 1 : code;
    src.setHeader(header.withHashCode(code));
    DCHECK(code == src.header().hashCode(), "hash failure");
  }
  return SmallInt::fromWord(code);
}

void Runtime::initializeTypes() {
  initializeLayouts();
  initializeHeapTypes();
  initializeImmediateTypes();
}

void Runtime::initializeLayouts() {
  HandleScope scope;
  Tuple array(&scope, newTuple(256));
  List list(&scope, newList());
  list.setItems(*array);
  const word allocated = static_cast<word>(LayoutId::kLastBuiltinId) + 1;
  CHECK(allocated < array.length(), "bad allocation %ld", allocated);
  list.setNumItems(allocated);
  layouts_ = *list;
}

RawObject Runtime::createMro(const Layout& subclass_layout,
                             LayoutId superclass_id) {
  HandleScope scope;
  CHECK(subclass_layout.describedType().isType(),
        "subclass layout must have a described class");
  Type superclass(&scope, typeAt(superclass_id));
  Tuple src(&scope, superclass.mro());
  Tuple dst(&scope, newTuple(1 + src.length()));
  dst.atPut(0, subclass_layout.describedType());
  for (word i = 0; i < src.length(); i++) {
    dst.atPut(1 + i, src.at(i));
  }
  return *dst;
}

void Runtime::initializeHeapTypes() {
  ObjectBuiltins::initialize(this);

  // Runtime-internal classes.
  addEmptyBuiltinType(SymbolId::kExceptionState, LayoutId::kExceptionState,
                      LayoutId::kObject);
  addEmptyBuiltinType(SymbolId::kUnderMutableBytes, LayoutId::kMutableBytes,
                      LayoutId::kObject);
  addEmptyBuiltinType(SymbolId::kUnderWeakLink, LayoutId::kWeakLink,
                      LayoutId::kObject);
  StrArrayBuiltins::initialize(this);

  // Abstract classes.
  BytesBuiltins::initialize(this);
  IntBuiltins::initialize(this);
  StrBuiltins::initialize(this);

  // Exception hierarchy.
  initializeExceptionTypes();

  // Concrete classes.
  AsyncGeneratorBuiltins::initialize(this);
  ByteArrayBuiltins::initialize(this);
  ByteArrayIteratorBuiltins::initialize(this);
  BytesIteratorBuiltins::initialize(this);
  ClassMethodBuiltins::initialize(this);
  addEmptyBuiltinType(SymbolId::kCode, LayoutId::kCode, LayoutId::kObject);
  CodeBuiltins::initialize(this);
  ComplexBuiltins::initialize(this);
  CoroutineBuiltins::initialize(this);
  DictBuiltins::initialize(this);
  DictItemsBuiltins::initialize(this);
  DictItemIteratorBuiltins::initialize(this);
  DictKeysBuiltins::initialize(this);
  DictKeyIteratorBuiltins::initialize(this);
  DictValuesBuiltins::initialize(this);
  DictValueIteratorBuiltins::initialize(this);
  addEmptyBuiltinType(SymbolId::kEllipsis, LayoutId::kEllipsis,
                      LayoutId::kObject);
  FloatBuiltins::initialize(this);
  addEmptyBuiltinType(SymbolId::kFrame, LayoutId::kHeapFrame,
                      LayoutId::kObject);
  FrozenSetBuiltins::initialize(this);
  FunctionBuiltins::initialize(this);
  GeneratorBuiltins::initialize(this);
  addEmptyBuiltinType(SymbolId::kLayout, LayoutId::kLayout, LayoutId::kObject);
  LargeBytesBuiltins::initialize(this);
  LargeIntBuiltins::initialize(this);
  LargeStrBuiltins::initialize(this);
  ListBuiltins::initialize(this);
  ListIteratorBuiltins::initialize(this);
  LongRangeIteratorBuiltins::initialize(this);
  BoundMethodBuiltins::initialize(this);
  MemoryViewBuiltins::initialize(this);
  ModuleBuiltins::initialize(this);
  addEmptyBuiltinType(SymbolId::kNotImplementedType,
                      LayoutId::kNotImplementedType, LayoutId::kObject);
  TupleBuiltins::initialize(this);
  TupleIteratorBuiltins::initialize(this);
  addEmptyBuiltinType(SymbolId::kUnderUnbound, LayoutId::kUnbound,
                      LayoutId::kObject);
  PropertyBuiltins::initialize(this);
  RangeBuiltins::initialize(this);
  RangeIteratorBuiltins::initialize(this);
  RefBuiltins::initialize(this);
  SetBuiltins::initialize(this);
  SeqIteratorBuiltins::initialize(this);
  SetIteratorBuiltins::initialize(this);
  SliceBuiltins::initialize(this);
  StrIteratorBuiltins::initialize(this);
  StaticMethodBuiltins::initialize(this);
  SuperBuiltins::initialize(this);
  addEmptyBuiltinType(SymbolId::kTraceback, LayoutId::kTraceback,
                      LayoutId::kObject);
  TypeBuiltins::initialize(this);
  addEmptyBuiltinType(SymbolId::kValueCell, LayoutId::kValueCell,
                      LayoutId::kObject);

  // IO types
  UnderIOBaseBuiltins::initialize(this);
  UnderRawIOBaseBuiltins::initialize(this);
  UnderBufferedIOBaseBuiltins::initialize(this);
  BytesIOBuiltins::initialize(this);
}

void Runtime::initializeExceptionTypes() {
  BaseExceptionBuiltins::initialize(this);

  // BaseException subclasses
  addEmptyBuiltinType(SymbolId::kException, LayoutId::kException,
                      LayoutId::kBaseException);
  addEmptyBuiltinType(SymbolId::kKeyboardInterrupt,
                      LayoutId::kKeyboardInterrupt, LayoutId::kBaseException);
  addEmptyBuiltinType(SymbolId::kGeneratorExit, LayoutId::kGeneratorExit,
                      LayoutId::kBaseException);
  SystemExitBuiltins::initialize(this);

  // Exception subclasses
  addEmptyBuiltinType(SymbolId::kArithmeticError, LayoutId::kArithmeticError,
                      LayoutId::kException);
  addEmptyBuiltinType(SymbolId::kAssertionError, LayoutId::kAssertionError,
                      LayoutId::kException);
  addEmptyBuiltinType(SymbolId::kAttributeError, LayoutId::kAttributeError,
                      LayoutId::kException);
  addEmptyBuiltinType(SymbolId::kBufferError, LayoutId::kBufferError,
                      LayoutId::kException);
  addEmptyBuiltinType(SymbolId::kEOFError, LayoutId::kEOFError,
                      LayoutId::kException);
  ImportErrorBuiltins::initialize(this);
  addEmptyBuiltinType(SymbolId::kLookupError, LayoutId::kLookupError,
                      LayoutId::kException);
  addEmptyBuiltinType(SymbolId::kMemoryError, LayoutId::kMemoryError,
                      LayoutId::kException);
  addEmptyBuiltinType(SymbolId::kNameError, LayoutId::kNameError,
                      LayoutId::kException);
  addEmptyBuiltinType(SymbolId::kOSError, LayoutId::kOSError,
                      LayoutId::kException);
  addEmptyBuiltinType(SymbolId::kReferenceError, LayoutId::kReferenceError,
                      LayoutId::kException);
  addEmptyBuiltinType(SymbolId::kRuntimeError, LayoutId::kRuntimeError,
                      LayoutId::kException);
  StopIterationBuiltins::initialize(this);
  addEmptyBuiltinType(SymbolId::kStopAsyncIteration,
                      LayoutId::kStopAsyncIteration, LayoutId::kException);
  addEmptyBuiltinType(SymbolId::kSyntaxError, LayoutId::kSyntaxError,
                      LayoutId::kException);
  addEmptyBuiltinType(SymbolId::kSystemError, LayoutId::kSystemError,
                      LayoutId::kException);
  addEmptyBuiltinType(SymbolId::kTypeError, LayoutId::kTypeError,
                      LayoutId::kException);
  addEmptyBuiltinType(SymbolId::kValueError, LayoutId::kValueError,
                      LayoutId::kException);
  addEmptyBuiltinType(SymbolId::kWarning, LayoutId::kWarning,
                      LayoutId::kException);

  // ArithmeticError subclasses
  addEmptyBuiltinType(SymbolId::kFloatingPointError,
                      LayoutId::kFloatingPointError,
                      LayoutId::kArithmeticError);
  addEmptyBuiltinType(SymbolId::kOverflowError, LayoutId::kOverflowError,
                      LayoutId::kArithmeticError);
  addEmptyBuiltinType(SymbolId::kZeroDivisionError,
                      LayoutId::kZeroDivisionError, LayoutId::kArithmeticError);

  // ImportError subclasses
  addEmptyBuiltinType(SymbolId::kModuleNotFoundError,
                      LayoutId::kModuleNotFoundError, LayoutId::kImportError);

  // LookupError subclasses
  addEmptyBuiltinType(SymbolId::kIndexError, LayoutId::kIndexError,
                      LayoutId::kLookupError);
  addEmptyBuiltinType(SymbolId::kKeyError, LayoutId::kKeyError,
                      LayoutId::kLookupError);

  // NameError subclasses
  addEmptyBuiltinType(SymbolId::kUnboundLocalError,
                      LayoutId::kUnboundLocalError, LayoutId::kNameError);

  // OSError subclasses
  addEmptyBuiltinType(SymbolId::kBlockingIOError, LayoutId::kBlockingIOError,
                      LayoutId::kOSError);
  addEmptyBuiltinType(SymbolId::kChildProcessError,
                      LayoutId::kChildProcessError, LayoutId::kOSError);
  addEmptyBuiltinType(SymbolId::kConnectionError, LayoutId::kConnectionError,
                      LayoutId::kOSError);
  addEmptyBuiltinType(SymbolId::kFileExistsError, LayoutId::kFileExistsError,
                      LayoutId::kOSError);
  addEmptyBuiltinType(SymbolId::kFileNotFoundError,
                      LayoutId::kFileNotFoundError, LayoutId::kOSError);
  addEmptyBuiltinType(SymbolId::kInterruptedError, LayoutId::kInterruptedError,
                      LayoutId::kOSError);
  addEmptyBuiltinType(SymbolId::kIsADirectoryError,
                      LayoutId::kIsADirectoryError, LayoutId::kOSError);
  addEmptyBuiltinType(SymbolId::kNotADirectoryError,
                      LayoutId::kNotADirectoryError, LayoutId::kOSError);
  addEmptyBuiltinType(SymbolId::kPermissionError, LayoutId::kPermissionError,
                      LayoutId::kOSError);
  addEmptyBuiltinType(SymbolId::kProcessLookupError,
                      LayoutId::kProcessLookupError, LayoutId::kOSError);
  addEmptyBuiltinType(SymbolId::kTimeoutError, LayoutId::kTimeoutError,
                      LayoutId::kOSError);

  // ConnectionError subclasses
  addEmptyBuiltinType(SymbolId::kBrokenPipeError, LayoutId::kBrokenPipeError,
                      LayoutId::kConnectionError);
  addEmptyBuiltinType(SymbolId::kConnectionAbortedError,
                      LayoutId::kConnectionAbortedError,
                      LayoutId::kConnectionError);
  addEmptyBuiltinType(SymbolId::kConnectionRefusedError,
                      LayoutId::kConnectionRefusedError,
                      LayoutId::kConnectionError);
  addEmptyBuiltinType(SymbolId::kConnectionResetError,
                      LayoutId::kConnectionResetError,
                      LayoutId::kConnectionError);

  // RuntimeError subclasses
  addEmptyBuiltinType(SymbolId::kNotImplementedError,
                      LayoutId::kNotImplementedError, LayoutId::kRuntimeError);
  addEmptyBuiltinType(SymbolId::kRecursionError, LayoutId::kRecursionError,
                      LayoutId::kRuntimeError);

  // SyntaxError subclasses
  addEmptyBuiltinType(SymbolId::kIndentationError, LayoutId::kIndentationError,
                      LayoutId::kSyntaxError);

  // IndentationError subclasses
  addEmptyBuiltinType(SymbolId::kTabError, LayoutId::kTabError,
                      LayoutId::kIndentationError);

  // ValueError subclasses
  UnicodeErrorBuiltins::initialize(this);

  // UnicodeError subclasses
  UnicodeDecodeErrorBuiltins::initialize(this);
  UnicodeEncodeErrorBuiltins::initialize(this);
  UnicodeTranslateErrorBuiltins::initialize(this);

  // Warning subclasses
  addEmptyBuiltinType(SymbolId::kUserWarning, LayoutId::kUserWarning,
                      LayoutId::kWarning);
  addEmptyBuiltinType(SymbolId::kDeprecationWarning,
                      LayoutId::kDeprecationWarning, LayoutId::kWarning);
  addEmptyBuiltinType(SymbolId::kPendingDeprecationWarning,
                      LayoutId::kPendingDeprecationWarning, LayoutId::kWarning);
  addEmptyBuiltinType(SymbolId::kSyntaxWarning, LayoutId::kSyntaxWarning,
                      LayoutId::kWarning);
  addEmptyBuiltinType(SymbolId::kRuntimeWarning, LayoutId::kRuntimeWarning,
                      LayoutId::kWarning);
  addEmptyBuiltinType(SymbolId::kFutureWarning, LayoutId::kFutureWarning,
                      LayoutId::kWarning);
  addEmptyBuiltinType(SymbolId::kImportWarning, LayoutId::kImportWarning,
                      LayoutId::kWarning);
  addEmptyBuiltinType(SymbolId::kUnicodeWarning, LayoutId::kUnicodeWarning,
                      LayoutId::kWarning);
  addEmptyBuiltinType(SymbolId::kBytesWarning, LayoutId::kBytesWarning,
                      LayoutId::kWarning);
  addEmptyBuiltinType(SymbolId::kResourceWarning, LayoutId::kResourceWarning,
                      LayoutId::kWarning);
}

void Runtime::initializeImmediateTypes() {
  BoolBuiltins::initialize(this);
  NoneBuiltins::initialize(this);
  SmallBytesBuiltins::initialize(this);
  SmallStrBuiltins::initialize(this);
  SmallIntBuiltins::initialize(this);
}

void Runtime::collectGarbage() {
  bool run_callback = callbacks_ == NoneType::object();
  RawObject cb = Scavenger(this).scavenge();
  callbacks_ = WeakRef::spliceQueue(callbacks_, cb);
  if (run_callback) {
    processCallbacks();
  }
}

void Runtime::processCallbacks() {
  Thread* thread = Thread::current();
  Frame* frame = thread->currentFrame();
  HandleScope scope(thread);
  Object saved_type(&scope, thread->pendingExceptionType());
  Object saved_value(&scope, thread->pendingExceptionValue());
  Object saved_traceback(&scope, thread->pendingExceptionTraceback());
  thread->clearPendingException();

  while (callbacks_ != NoneType::object()) {
    Object weak(&scope, WeakRef::dequeueReference(&callbacks_));
    Object callback(&scope, WeakRef::cast(*weak).callback());
    Interpreter::callMethod1(thread, frame, callback, weak);
    thread->ignorePendingException();
    WeakRef::cast(*weak).setCallback(NoneType::object());
  }

  thread->setPendingExceptionType(*saved_type);
  thread->setPendingExceptionValue(*saved_value);
  thread->setPendingExceptionTraceback(*saved_traceback);
}

RawObject Runtime::findOrCreateImportlibModule(Thread* thread) {
  HandleScope scope(thread);
  Object importlib_obj(&scope, findModuleById(SymbolId::kUnderFrozenImportlib));
  // We may need to load and create `_frozen_importlib` if it doesn't exist.
  if (importlib_obj.isNoneType()) {
    createImportlibModule(thread);
    importlib_obj = findModuleById(SymbolId::kUnderFrozenImportlib);
  }
  return *importlib_obj;
}

RawObject Runtime::findOrCreateMainModule() {
  HandleScope scope;
  Object main(&scope, findModuleById(SymbolId::kDunderMain));
  if (main.isNoneType()) {
    main = createMainModule();
  }
  return *main;
}

RawObject Runtime::executeFrozenModule(const char* buffer,
                                       const Module& module) {
  HandleScope scope;
  Marshal::Reader reader(&scope, this, buffer);
  reader.readLong();
  reader.readLong();
  reader.readLong();
  Code code(&scope, reader.readObject());
  return executeModule(code, module);
}

RawObject Runtime::executeModule(const Code& code, const Module& module) {
  HandleScope scope;
  DCHECK(code.argcount() == 0, "invalid argcount %ld", code.argcount());
  Dict globals(&scope, module.dict());
  return Thread::current()->exec(code, globals, globals);
}

RawObject Runtime::printTraceback(Thread* /* thread */,
                                  const Object& /* file */) {
  // TODO(T42602699): Replace this with an actual traceback printer
  // TODO(T42602545): Write to given file object
  Utils::printTracebackToStderr();
  return NoneType::object();
}

RawObject Runtime::importModuleFromCode(const Code& code, const Object& name) {
  HandleScope scope;
  Object cached_module(&scope, findModule(name));
  if (!cached_module.isNoneType()) {
    return *cached_module;
  }

  Module module(&scope, newModule(name));
  addModule(module);
  Object result(&scope, executeModule(code, module));
  if (result.isError()) return *result;
  return *module;
}

void Runtime::initializeThreads() {
  auto main_thread = new Thread(Thread::kDefaultStackSize);
  main_thread->setCaughtExceptionState(heap()->create<RawExceptionState>());
  threads_ = main_thread;
  main_thread->setRuntime(this);
  Thread::setCurrentThread(main_thread);
}

void Runtime::initializePrimitiveInstances() {
  empty_tuple_ = heap()->createTuple(0);
  empty_frozen_set_ = newFrozenSet();
  empty_mutable_bytes_ = heap()->createMutableBytes(0);
  empty_slice_ = heap()->create<RawSlice>();
  ellipsis_ = heap()->createEllipsis();
  callbacks_ = NoneType::object();
}

void Runtime::initializeImplicitBases() {
  DCHECK(!implicit_bases_.isTuple(), "implicit bases already initialized");
  implicit_bases_ = heap()->createTuple(1);
  Tuple::cast(implicit_bases_).atPut(0, typeAt(LayoutId::kObject));
}

void Runtime::initializeInterned() { interned_ = newSet(); }

void Runtime::initializeRandom() {
  uword random_state[2];
  uword hash_secret[2];
  // TODO(T43142858) Replace getenv with a configuration system.
  const char* hashseed = std::getenv("PYTHONHASHSEED");
  if (hashseed == nullptr || std::strcmp(hashseed, "random") == 0) {
    OS::secureRandom(reinterpret_cast<byte*>(&random_state),
                     sizeof(random_state));
    OS::secureRandom(reinterpret_cast<byte*>(&hash_secret),
                     sizeof(hash_secret));
  } else {
    char* endptr;
    unsigned long seed = std::strtoul(hashseed, &endptr, 10);
    if (*endptr != '\0' || seed > 4294967295 ||
        (seed == ULONG_MAX && errno == ERANGE)) {
      std::fprintf(stderr,
                   "Fatal Python error: PYTHONHASHSEED must be "
                   "\"random\" or an integer in range [0; 4294967295]");
      std::exit(EXIT_FAILURE);
    }
    // Splitmix64 as suggested by http://http://xoshiro.di.unimi.it.
    uword state = static_cast<uword>(seed);
    auto next = [&state]() {
      uword z = (state += 0x9e3779b97f4a7c15);
      z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
      z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
      return z ^ (z >> 31);
    };
    random_state[0] = next();
    random_state[1] = next();
    hash_secret[0] = next();
    hash_secret[1] = next();
  }
  seedRandom(random_state, hash_secret);
}

void Runtime::initializeSymbols() {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  symbols_ = new Symbols(this);
  for (int i = 0; i < static_cast<int>(SymbolId::kMaxId); i++) {
    SymbolId id = static_cast<SymbolId>(i);
    Object symbol(&scope, symbols()->at(id));
    internStr(thread, symbol);
  }
}

void Runtime::visitRoots(PointerVisitor* visitor) {
  visitRuntimeRoots(visitor);
  visitThreadRoots(visitor);
}

void Runtime::visitRuntimeRoots(PointerVisitor* visitor) {
  // Visit layouts
  visitor->visitPointer(&layouts_);

  // Visit internal types that are not described by a layout
  visitor->visitPointer(&large_bytes_);
  visitor->visitPointer(&large_int_);
  visitor->visitPointer(&large_str_);
  visitor->visitPointer(&small_bytes_);
  visitor->visitPointer(&small_int_);
  visitor->visitPointer(&small_str_);

  // Visit instances
  visitor->visitPointer(&build_class_);
  visitor->visitPointer(&display_hook_);
  visitor->visitPointer(&dunder_import_);
  visitor->visitPointer(&ellipsis_);
  visitor->visitPointer(&empty_frozen_set_);
  visitor->visitPointer(&empty_mutable_bytes_);
  visitor->visitPointer(&empty_tuple_);
  visitor->visitPointer(&implicit_bases_);
  visitor->visitPointer(&object_dunder_getattribute_);
  visitor->visitPointer(&object_dunder_init_);
  visitor->visitPointer(&object_dunder_new_);
  visitor->visitPointer(&object_dunder_setattr_);
  visitor->visitPointer(&sys_stderr_);
  visitor->visitPointer(&sys_stdout_);

  // Visit interned strings.
  visitor->visitPointer(&interned_);

  // Visit canonical empty slice.
  visitor->visitPointer(&empty_slice_);

  // Visit modules
  visitor->visitPointer(&modules_);

  // Visit C-API data.
  visitor->visitPointer(&api_handles_);
  ApiHandle::visitReferences(apiHandles(), visitor);
  visitor->visitPointer(&api_caches_);

  // Visit symbols
  symbols_->visit(visitor);

  // Visit GC callbacks
  visitor->visitPointer(&callbacks_);
}

void Runtime::visitThreadRoots(PointerVisitor* visitor) {
  for (Thread* thread = threads_; thread != nullptr; thread = thread->next()) {
    thread->visitRoots(visitor);
  }
}

void Runtime::addModule(const Module& module) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Dict dict(&scope, modules());
  Object key(&scope, module.name());
  Object value(&scope, *module);
  dictAtPut(thread, dict, key, value);
}

RawObject Runtime::findModule(const Object& name) {
  DCHECK(name.isStr(), "name not a string");

  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Dict dict(&scope, modules());
  RawObject value = dictAt(thread, dict, name);
  if (value.isError()) {
    return NoneType::object();
  }
  return value;
}

RawObject Runtime::findModuleById(SymbolId name) {
  HandleScope scope;
  Str name_str(&scope, symbols()->at(name));
  return findModule(name_str);
}

RawObject Runtime::lookupNameInModule(Thread* thread, SymbolId module_name,
                                      SymbolId name) {
  HandleScope scope(thread);
  Object module_obj(&scope, findModuleById(module_name));
  DCHECK(module_obj.isModule(),
         "The given module '%s' doesn't exist in modules dict",
         symbols()->predefinedSymbolAt(module_name));
  Module module(&scope, *module_obj);
  return moduleAtById(thread, module, name);
}

// TODO(emacs): Move these names into the modules themselves, so there is only
// once source of truth.
const ModuleInitializer Runtime::kBuiltinModules[] = {
    {SymbolId::kUnderCodecs, &UnderCodecsModule::initialize},
    {SymbolId::kUnderImp, &UnderImpModule::initialize},
    {SymbolId::kUnderIo, &UnderIoModule::initialize},
    {SymbolId::kUnderStrMod, &UnderStrModModule::initialize},
    {SymbolId::kMarshal, &MarshalModule::initialize},
    {SymbolId::kUnderWarnings, &UnderWarningsModule::initialize},
    {SymbolId::kUnderWeakRef, &UnderWeakrefModule::initialize},
    {SymbolId::kOperator, &OperatorModule::initialize},
    {SymbolId::kWarnings, &WarningsModule::initialize},
    {SymbolId::kSentinelId, nullptr},
};

void Runtime::initializeModules() {
  Thread* thread = Thread::current();
  modules_ = newDict();
  createEmptyBuiltinsModule(thread);
  createUnderBuiltinsModule(thread);
  createBuiltinsModule(thread);
  createSysModule(thread);
  for (word i = 0; kBuiltinModules[i].name != SymbolId::kSentinelId; i++) {
    kBuiltinModules[i].create_module(thread);
  }
}

void Runtime::initializeApiData() {
  api_handles_ = newDict();
  api_caches_ = newDict();
}

RawObject Runtime::concreteTypeAt(LayoutId layout_id) {
  switch (layout_id) {
    case LayoutId::kLargeBytes:
      return large_bytes_;
    case LayoutId::kLargeInt:
      return large_int_;
    case LayoutId::kLargeStr:
      return large_str_;
    case LayoutId::kSmallBytes:
      return small_bytes_;
    case LayoutId::kSmallInt:
      return small_int_;
    case LayoutId::kSmallStr:
      return small_str_;
    default:
      return Layout::cast(layoutAt(layout_id)).describedType();
  }
}

void Runtime::setLargeBytesType(const Type& type) { large_bytes_ = *type; }

void Runtime::setLargeIntType(const Type& type) { large_int_ = *type; }

void Runtime::setLargeStrType(const Type& type) { large_str_ = *type; }

void Runtime::setSmallBytesType(const Type& type) { small_bytes_ = *type; }

void Runtime::setSmallIntType(const Type& type) { small_int_ = *type; }

void Runtime::setSmallStrType(const Type& type) { small_str_ = *type; }

void Runtime::layoutAtPut(LayoutId layout_id, RawObject object) {
  List::cast(layouts_).atPut(static_cast<word>(layout_id), object);
}

RawObject Runtime::typeAt(LayoutId layout_id) {
  return Layout::cast(layoutAt(layout_id)).describedType();
}

RawObject Runtime::typeDictAt(Thread* thread, const Dict& dict,
                              const Object& key) {
  HandleScope scope(thread);
  Object value(&scope, dictAt(thread, dict, key));
  DCHECK(value.isErrorNotFound() || value.isValueCell(),
         "type dictionaries must return either ErrorNotFound or ValueCell");
  if (value.isErrorNotFound() || ValueCell::cast(*value).isPlaceholder()) {
    return Error::notFound();
  }
  return ValueCell::cast(*value).value();
}

RawObject Runtime::typeDictAtPut(Thread* thread, const Dict& dict,
                                 const Object& key, const Object& value) {
  return dictAtPutInValueCell(thread, dict, key, value);
}

LayoutId Runtime::reserveLayoutId(Thread* thread) {
  HandleScope scope(thread);
  List list(&scope, layouts_);
  Object value(&scope, NoneType::object());
  word result = list.numItems();
  DCHECK(result <= RawHeader::kMaxLayoutId,
         "exceeded layout id space in header word");
  listAdd(thread, list, value);
  return static_cast<LayoutId>(result);
}

SymbolId Runtime::binaryOperationSelector(Interpreter::BinaryOp op) {
  return kBinaryOperationSelector[static_cast<int>(op)];
}

SymbolId Runtime::swappedBinaryOperationSelector(Interpreter::BinaryOp op) {
  return kSwappedBinaryOperationSelector[static_cast<int>(op)];
}

SymbolId Runtime::inplaceOperationSelector(Interpreter::BinaryOp op) {
  DCHECK(op != Interpreter::BinaryOp::DIVMOD,
         "DIVMOD is not a valid inplace op");
  return kInplaceOperationSelector[static_cast<int>(op)];
}

SymbolId Runtime::comparisonSelector(CompareOp op) {
  DCHECK(op >= CompareOp::LT, "invalid compare op");
  DCHECK(op <= CompareOp::GE, "invalid compare op");
  return kComparisonSelector[op];
}

SymbolId Runtime::swappedComparisonSelector(CompareOp op) {
  DCHECK(op >= CompareOp::LT, "invalid compare op");
  DCHECK(op <= CompareOp::GE, "invalid compare op");
  CompareOp swapped_op = kSwappedCompareOp[op];
  return comparisonSelector(swapped_op);
}

RawObject Runtime::moduleAddBuiltinFunction(const Module& module, SymbolId name,
                                            Function::Entry entry) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Str name_str(&scope, symbols()->at(name));
  Tuple empty_tuple(&scope, emptyTuple());
  Code code(&scope, newBuiltinCode(/*argcount=*/0, /*posonlyargcount=*/0,
                                   /*kwonlyargcount=*/0,
                                   /*flags=*/0, entry,
                                   /*parameter_names=*/empty_tuple, name_str));
  Object globals(&scope, NoneType::object());
  Function function(&scope,
                    newFunctionWithCode(thread, name_str, code, globals));
  return moduleAtPut(thread, module, name_str, function);
}

void Runtime::moduleAddBuiltinType(const Module& module, SymbolId name,
                                   LayoutId layout_id) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object value(&scope, typeAt(layout_id));
  moduleAtPutById(thread, module, name, value);
}

void Runtime::moduleImportAllFrom(const Dict& dict, const Module& module) {
  Thread* thread = Thread::current();
  HandleScope scope;
  Dict module_dict(&scope, module.dict());
  Tuple buckets(&scope, module_dict.data());
  for (word i = Dict::Bucket::kFirst; nextModuleDictItem(*buckets, &i);) {
    CHECK(Dict::Bucket::key(*buckets, i).isStr(), "Symbol is not a String");
    Str symbol_name(&scope, Dict::Bucket::key(*buckets, i));
    // Load all the symbols not starting with '_'
    if (symbol_name.charAt(0) != '_') {
      Object value(&scope, moduleAt(thread, module, symbol_name));
      DCHECK(!value.isErrorNotFound(), "value must not be ErrorNotFound");
      dictAtPutInValueCell(thread, dict, symbol_name, value);
    }
  }
}

void Runtime::createBuiltinsModule(Thread* thread) {
  HandleScope scope(thread);
  // Find the module object created by Runtime::createEmptyBuiltinsModule()
  Module module(&scope, findModuleById(SymbolId::kBuiltins));
  for (word i = 0;
       BuiltinsModule::kBuiltinMethods[i].name != SymbolId::kSentinelId; i++) {
    moduleAddBuiltinFunction(module, BuiltinsModule::kBuiltinMethods[i].name,
                             BuiltinsModule::kBuiltinMethods[i].address);
  }
  for (word i = 0;
       BuiltinsModule::kBuiltinTypes[i].name != SymbolId::kSentinelId; i++) {
    moduleAddBuiltinType(module, BuiltinsModule::kBuiltinTypes[i].name,
                         BuiltinsModule::kBuiltinTypes[i].type);
  }

  moduleAddBuiltinFunction(module, SymbolId::kDunderBuildClass,
                           BuiltinsModule::dunderBuildClass);
  Dict module_dict(&scope, module.dict());
  Object dunder_build_class_name(&scope, symbols()->DunderBuildClass());
  build_class_ =
      thread->runtime()->dictAt(thread, module_dict, dunder_build_class_name);

  // Add module variables
  {
    Object dunder_debug(&scope, Bool::falseObj());
    moduleAtPutById(thread, module, SymbolId::kDunderDebug, dunder_debug);

    Object false_obj(&scope, Bool::falseObj());
    moduleAtPutById(thread, module, SymbolId::kFalse, false_obj);

    Object none(&scope, NoneType::object());
    moduleAtPutById(thread, module, SymbolId::kNone, none);

    Object not_implemented(&scope, NotImplementedType::object());
    moduleAtPutById(thread, module, SymbolId::kNotImplemented, not_implemented);

    Object true_obj(&scope, Bool::trueObj());
    moduleAtPutById(thread, module, SymbolId::kTrue, true_obj);
  }

  {
    // Manually import all of the functions and types in the _builtins module.
    Module under_builtins(&scope, findModuleById(SymbolId::kUnderBuiltins));
    Object key(&scope, Unbound::object());
    Object value(&scope, Unbound::object());
    for (word i = 0;
         UnderBuiltinsModule::kBuiltinMethods[i].name != SymbolId::kSentinelId;
         i++) {
      key = symbols()->at(UnderBuiltinsModule::kBuiltinMethods[i].name);
      value = moduleAt(thread, under_builtins, key);
      dictAtPutInValueCell(thread, module_dict, key, value);
    }
    for (word i = 0;
         UnderBuiltinsModule::kBuiltinTypes[i].name != SymbolId::kSentinelId;
         i++) {
      key = symbols()->at(UnderBuiltinsModule::kBuiltinTypes[i].name);
      value = moduleAt(thread, under_builtins, key);
      dictAtPutInValueCell(thread, module_dict, key, value);
    }
    key = symbols()->UnderPatch();
    value = moduleAt(thread, under_builtins, key);
    dictAtPutInValueCell(thread, module_dict, key, value);
    key = symbols()->UnderUnbound();
    value = moduleAt(thread, under_builtins, key);
    dictAtPutInValueCell(thread, module_dict, key, value);
  }

  // Add and execute builtins module.
  CHECK(!executeFrozenModule(BuiltinsModule::kFrozenData, module).isError(),
        "Failed to initialize builtins module");

  // TODO(T39575976): Create a consistent way to hide internal names
  // such as "module" or "function"
  Object dunder_import_name(&scope, symbols()->DunderImport());
  dunder_import_ =
      thread->runtime()->dictAt(thread, module_dict, dunder_import_name);

  Type object(&scope, typeAt(LayoutId::kObject));
  Dict object_dict(&scope, object.dict());
  Object dunder_getattribute_name(&scope, symbols()->DunderGetattribute());
  object_dunder_getattribute_ =
      typeDictAt(thread, object_dict, dunder_getattribute_name);
  Object dunder_init_name(&scope, symbols()->DunderInit());
  object_dunder_init_ = typeDictAt(thread, object_dict, dunder_init_name);
  Object dunder_new_name(&scope, symbols()->DunderNew());
  object_dunder_new_ = typeDictAt(thread, object_dict, dunder_new_name);
  Object dunder_setattr_name(&scope, symbols()->DunderSetattr());
  object_dunder_setattr_ = typeDictAt(thread, object_dict, dunder_setattr_name);

  // Mark functions that have an intrinsic implementation.
  for (word i = 0; BuiltinsModule::kIntrinsicIds[i] != SymbolId::kSentinelId;
       i++) {
    SymbolId intrinsic_id = BuiltinsModule::kIntrinsicIds[i];
    Function::cast(moduleAtById(thread, module, intrinsic_id))
        .setIntrinsicId(static_cast<word>(intrinsic_id));
  }
}

void Runtime::createEmptyBuiltinsModule(Thread* thread) {
  HandleScope scope(thread);

  Str name(&scope, symbols()->Builtins());
  Module builtins(&scope, newModule(name));
  addModule(builtins);
}

void Runtime::createImportlibModule(Thread* thread) {
  HandleScope scope(thread);

  // CPython's freezing tool creates the following mapping:
  // `_frozen_importlib`: importlib/_bootstrap.py frozen bytes
  // `_frozen_importlib_external`: importlib/_external_bootstrap.py frozen bytes
  // This replicates that mapping for compatibility

  // Run _bootstrap.py
  Str importlib_name(&scope, symbols()->UnderFrozenImportlib());
  Module importlib(&scope, newModule(importlib_name));
  CHECK(!executeFrozenModule(kUnderBootstrapModuleData, importlib).isError(),
        "Failed to initialize _bootstrap module");
  addModule(importlib);

  // Run _bootstrap_external.py
  Str importlib_external_name(&scope,
                              symbols()->UnderFrozenImportlibExternal());
  Module importlib_external(&scope, newModule(importlib_external_name));
  moduleAtPutById(thread, importlib_external, SymbolId::kUnderBootstrap,
                  importlib);
  CHECK(!executeFrozenModule(kUnderBootstrapUnderExternalModuleData,
                             importlib_external)
             .isError(),
        "Failed to initialize _bootstrap_external module");
  addModule(importlib_external);

  // Run _bootstrap._install(sys, _imp)
  Module sys_module(&scope, findModuleById(SymbolId::kSys));
  Module imp_module(&scope, findModuleById(SymbolId::kUnderImp));
  CHECK(!thread
             ->invokeFunction2(SymbolId::kUnderFrozenImportlib,
                               SymbolId::kUnderInstall, sys_module, imp_module)
             .isError(),
        "Failed to run _bootstrap._install");
}

void Runtime::createSysModule(Thread* thread) {
  HandleScope scope(thread);
  Str name_str(&scope, symbols()->Sys());
  Module module(&scope, newModule(name_str));
  for (word i = 0; SysModule::kBuiltinMethods[i].name != SymbolId::kSentinelId;
       i++) {
    moduleAddBuiltinFunction(module, SysModule::kBuiltinMethods[i].name,
                             SysModule::kBuiltinMethods[i].address);
  }

  Object modules(&scope, modules_);
  moduleAtPutById(thread, module, SymbolId::kModules, modules);

  // Fill in sys...
  Object platform(&scope, newStrFromCStr(OS::name()));
  moduleAtPutById(thread, module, SymbolId::kPlatform, platform);

  Object stderr_fd_val(&scope, SmallInt::fromWord(kStderrFd));
  moduleAtPutById(thread, module, SymbolId::kUnderStderrFd, stderr_fd_val);
  Object stdout_fd_val(&scope, SmallInt::fromWord(kStdoutFd));
  moduleAtPutById(thread, module, SymbolId::kUnderStdoutFd, stdout_fd_val);

  // TODO(T42692043): This awkwardness should go away once we freeze the
  // standard library into the binary and/or support PYTHONPATH.
  Object base_dir(&scope, newStrFromCStr(PYRO_BASEDIR));
  moduleAtPutById(thread, module, SymbolId::kUnderBaseDir, base_dir);

  Object byteorder(
      &scope,
      newStrFromCStr(endian::native == endian::little ? "little" : "big"));
  moduleAtPutById(thread, module, SymbolId::kByteorder, byteorder);

  unique_c_ptr<char> executable_path(OS::executablePath());
  Object executable(&scope, newStrFromCStr(executable_path.get()));
  moduleAtPutById(thread, module, SymbolId::kExecutable, executable);

  // maxsize is defined as the largest supported length of containers which
  // would be `SmallInt::kMaxValue`. However in practice it is used to
  // determine the size of a machine word which is kMaxWord.
  Object maxsize(&scope, newInt(kMaxWord));
  moduleAtPutById(thread, module, SymbolId::kMaxsize, maxsize);

  Object maxunicode(&scope, newInt(kMaxUnicode));
  moduleAtPutById(thread, module, SymbolId::kMaxunicode, maxunicode);

  // Count the number of modules and create a tuple
  uword num_external_modules = 0;
  while (_PyImport_Inittab[num_external_modules].name != nullptr) {
    num_external_modules++;
  }
  uword num_builtin_modules = 2;
  for (int i = 0; kBuiltinModules[i].name != SymbolId::kSentinelId; i++) {
    num_builtin_modules++;
  }

  uword num_modules = num_builtin_modules + num_external_modules;
  Tuple builtins_tuple(&scope, newTuple(num_modules));

  // Add all the available builtin modules
  builtins_tuple.atPut(0, symbols()->Builtins());
  builtins_tuple.atPut(1, symbols()->Sys());
  for (uword i = 2; i < num_builtin_modules; i++) {
    Object module_name(&scope, symbols()->at(kBuiltinModules[i - 2].name));
    builtins_tuple.atPut(i, *module_name);
  }

  // Add all the available extension builtin modules
  for (int i = 0; _PyImport_Inittab[i].name != nullptr; i++) {
    Object module_name(&scope, newStrFromCStr(_PyImport_Inittab[i].name));
    builtins_tuple.atPut(num_builtin_modules + i, *module_name);
  }

  // Create builtin_module_names tuple
  Object builtins(&scope, *builtins_tuple);
  moduleAtPutById(thread, module, SymbolId::kBuiltinModuleNames, builtins);
  // Add and execute sys module.
  addModule(module);
  CHECK(!executeFrozenModule(SysModule::kFrozenData, module).isError(),
        "Failed to initialize sys module");

  Dict module_dict(&scope, module.dict());
  Object stderr_name(&scope, symbols()->Stderr());
  sys_stderr_ = dictAt(thread, module_dict, stderr_name);
  CHECK(!sys_stderr_.isError(), "sys.stderr not found");
  Object stdout_name(&scope, symbols()->Stdout());
  sys_stdout_ = dictAt(thread, module_dict, stdout_name);
  CHECK(!sys_stdout_.isError(), "sys.stdout not found");
  Object display_hook_name(&scope, symbols()->Displayhook());
  display_hook_ = dictAt(thread, module_dict, display_hook_name);
  CHECK(!display_hook_.isError(), "sys.displayhook not found");
}

void Runtime::createUnderBuiltinsModule(Thread* thread) {
  HandleScope scope(thread);
  Str name_str(&scope, symbols()->UnderBuiltins());
  Module module(&scope, newModule(name_str));
  for (word i = 0;
       UnderBuiltinsModule::kBuiltinMethods[i].name != SymbolId::kSentinelId;
       i++) {
    moduleAddBuiltinFunction(module,
                             UnderBuiltinsModule::kBuiltinMethods[i].name,
                             UnderBuiltinsModule::kBuiltinMethods[i].address);
  }
  for (word i = 0;
       UnderBuiltinsModule::kBuiltinTypes[i].name != SymbolId::kSentinelId;
       i++) {
    moduleAddBuiltinType(module, UnderBuiltinsModule::kBuiltinTypes[i].name,
                         UnderBuiltinsModule::kBuiltinTypes[i].type);
  }

  // We have to patch _patch manually.
  {
    Tuple parameters(&scope, newTuple(1));
    parameters.atPut(0, newStrFromCStr("function"));
    Object name(&scope, symbols()->UnderPatch());
    Code code(&scope, newBuiltinCode(/*argcount=*/1, /*posonlyargcount=*/0,
                                     /*kwonlyargcount=*/0, /*flags=*/0,
                                     UnderBuiltinsModule::underPatch,
                                     parameters, name));
    Dict globals(&scope, module.dict());
    Function under_patch(&scope,
                         newFunctionWithCode(thread, name, code, globals));
    moduleAtPut(thread, module, name, under_patch);
  }

  Object unbound_value(&scope, Unbound::object());
  moduleAtPutById(thread, module, SymbolId::kUnderUnbound, unbound_value);

  // Mark functions that have an intrinsic implementation.
  for (word i = 0;
       UnderBuiltinsModule::kIntrinsicIds[i] != SymbolId::kSentinelId; i++) {
    SymbolId intrinsic_id = UnderBuiltinsModule::kIntrinsicIds[i];
    Function::cast(moduleAtById(thread, module, intrinsic_id))
        .setIntrinsicId(static_cast<word>(intrinsic_id));
  }

  // Add _builtins module.
  addModule(module);
  CHECK(
      !executeFrozenModule(UnderBuiltinsModule::kFrozenData, module).isError(),
      "Failed to initialize _builtins module");
}

RawObject Runtime::createMainModule() {
  HandleScope scope;
  Object name(&scope, symbols()->DunderMain());
  Module module(&scope, newModule(name));

  // Fill in __main__...

  addModule(module);

  return *module;
}

word Runtime::newCapacity(word curr_capacity, word min_capacity) {
  word new_capacity = (curr_capacity < kInitialEnsuredCapacity)
                          ? kInitialEnsuredCapacity
                          : curr_capacity + (curr_capacity >> 1);
  if (new_capacity < min_capacity) {
    return min_capacity;
  }
  return Utils::minimum(new_capacity, SmallInt::kMaxValue);
}

// ByteArray

void Runtime::byteArrayEnsureCapacity(Thread* thread, const ByteArray& array,
                                      word min_capacity) {
  DCHECK_BOUND(min_capacity, SmallInt::kMaxValue);
  word curr_capacity = array.capacity();
  if (min_capacity <= curr_capacity) return;
  word new_capacity = newCapacity(curr_capacity, min_capacity);
  HandleScope scope(thread);
  MutableBytes old_bytes(&scope, array.bytes());
  MutableBytes new_bytes(&scope, newMutableBytesUninitialized(new_capacity));
  byte* dst = reinterpret_cast<byte*>(new_bytes.address());
  word old_length = array.numItems();
  old_bytes.copyTo(dst, old_length);
  std::memset(dst + old_length, 0, new_capacity - old_length);
  array.setBytes(*new_bytes);
}

void Runtime::byteArrayExtend(Thread* thread, const ByteArray& array,
                              View<byte> view) {
  word length = view.length();
  if (length == 0) return;
  word num_items = array.numItems();
  word new_length = num_items + length;
  byteArrayEnsureCapacity(thread, array, new_length);
  byte* dst =
      reinterpret_cast<byte*>(MutableBytes::cast(array.bytes()).address());
  std::memcpy(dst + num_items, view.data(), view.length());
  array.setNumItems(new_length);
}

void Runtime::byteArrayIadd(Thread* thread, const ByteArray& array,
                            const Bytes& bytes, word length) {
  DCHECK_BOUND(length, bytes.length());
  if (length == 0) return;
  word num_items = array.numItems();
  word new_length = num_items + length;
  byteArrayEnsureCapacity(thread, array, new_length);
  MutableBytes::cast(array.bytes()).replaceFromWith(num_items, *bytes, length);
  array.setNumItems(new_length);
}

// Bytes

RawObject Runtime::bytesConcat(Thread* thread, const Bytes& self,
                               const Bytes& other) {
  word self_len = self.length();
  word other_len = other.length();
  word len = self_len + other_len;
  if (len <= SmallBytes::kMaxLength) {
    byte buffer[SmallBytes::kMaxLength];
    self.copyTo(buffer, self_len);
    other.copyTo(buffer + self_len, other_len);
    return SmallBytes::fromBytes({buffer, len});
  }
  HandleScope scope(thread);
  MutableBytes result(&scope, newMutableBytesUninitialized(len));
  result.replaceFromWith(0, *self, self_len);
  result.replaceFromWith(self_len, *other, other_len);
  return result.becomeImmutable();
}

RawObject Runtime::bytesCopyWithSize(Thread* thread, const Bytes& original,
                                     word new_length) {
  DCHECK(new_length > 0, "length must be positive");
  word old_length = original.length();
  if (new_length <= SmallBytes::kMaxLength) {
    byte buffer[SmallBytes::kMaxLength];
    original.copyTo(buffer, Utils::minimum(old_length, new_length));
    return SmallBytes::fromBytes({buffer, new_length});
  }
  HandleScope scope(thread);
  MutableBytes copy(&scope, newMutableBytesUninitialized(new_length));
  byte* dst = reinterpret_cast<byte*>(copy.address());
  if (old_length < new_length) {
    original.copyTo(dst, old_length);
    std::memset(dst + old_length, 0, new_length - old_length);
  } else {
    original.copyTo(dst, new_length);
  }
  return copy.becomeImmutable();
}

RawObject Runtime::bytesEndsWith(const Bytes& self, word self_len,
                                 const Bytes& suffix, word suffix_len,
                                 word start, word end) {
  DCHECK_BOUND(self_len, self.length());
  DCHECK_BOUND(suffix_len, suffix.length());
  Slice::adjustSearchIndices(&start, &end, self_len);
  if (end - start < suffix_len || start > self_len) {
    return Bool::falseObj();
  }
  for (word i = end - suffix_len, j = 0; i < end; i++, j++) {
    if (self.byteAt(i) != suffix.byteAt(j)) {
      return Bool::falseObj();
    }
  }
  return Bool::trueObj();
}

RawObject Runtime::bytesFromTuple(Thread* thread, const Tuple& items,
                                  word length) {
  DCHECK_BOUND(length, items.length());
  HandleScope scope(thread);
  MutableBytes result(&scope, empty_mutable_bytes_);
  byte buffer[SmallBytes::kMaxLength];
  byte* dst;
  if (length <= SmallBytes::kMaxLength) {
    dst = buffer;
  } else {
    result = newMutableBytesUninitialized(length);
    dst = reinterpret_cast<byte*>(MutableBytes::cast(*result).address());
  }
  for (word idx = 0; idx < length; idx++) {
    Object item(&scope, items.at(idx));
    if (!isInstanceOfInt(*item)) {
      // escape into slow path
      return NoneType::object();
    }
    Int index(&scope, intUnderlying(thread, item));
    OptInt<byte> current_byte = index.asInt<byte>();
    if (current_byte.error == CastError::None) {
      dst[idx] = current_byte.value;
    } else {
      return thread->raiseWithFmt(LayoutId::kValueError,
                                  "bytes must be in range(0, 256)");
    }
  }
  return length <= SmallBytes::kMaxLength
             ? SmallBytes::fromBytes({buffer, length})
             : result.becomeImmutable();
}

RawObject Runtime::bytesJoin(Thread* thread, const Bytes& sep, word sep_length,
                             const Tuple& src, word src_length) {
  DCHECK_BOUND(src_length, src.length());
  bool is_mutable = sep.isMutableBytes();
  if (src_length == 0) {
    if (is_mutable) {
      return empty_mutable_bytes_;
    }
    return Bytes::empty();
  }
  HandleScope scope(thread);

  // first pass to accumulate length and check types
  word result_length = sep_length * (src_length - 1);
  Object item(&scope, Unbound::object());
  for (word index = 0; index < src_length; index++) {
    item = src.at(index);
    if (isInstanceOfBytes(*item)) {
      Bytes bytes(&scope, bytesUnderlying(thread, item));
      result_length += bytes.length();
    } else if (isInstanceOfByteArray(*item)) {
      ByteArray array(&scope, *item);
      result_length += array.numItems();
    } else {
      return thread->raiseWithFmt(
          LayoutId::kTypeError,
          "sequence item %w: expected a bytes-like object, %T found", index,
          &item);
    }
  }

  // second pass to accumulate concatenation
  MutableBytes result(&scope, empty_mutable_bytes_);
  byte buffer[SmallBytes::kMaxLength];
  byte* dst = nullptr;
  bool is_small_bytes = result_length <= SmallBytes::kMaxLength && !is_mutable;
  if (is_small_bytes) {
    dst = buffer;
  } else {
    result = newMutableBytesUninitialized(result_length);
    dst = reinterpret_cast<byte*>(MutableBytes::cast(*result).address());
  }
  const byte* const end = dst + result_length;
  for (word src_index = 0; src_index < src_length; src_index++) {
    if (src_index > 0) {
      sep.copyTo(dst, sep_length);
      dst += sep_length;
    }
    item = src.at(src_index);
    Bytes bytes(&scope, Bytes::empty());
    word length;
    if (isInstanceOfBytes(*item)) {
      bytes = bytesUnderlying(thread, item);
      length = bytes.length();
    } else {
      DCHECK(isInstanceOfByteArray(*item), "source is not bytes-like");
      ByteArray array(&scope, *item);
      bytes = array.bytes();
      length = array.numItems();
    }
    bytes.copyTo(dst, length);
    dst += length;
  }
  DCHECK(dst == end, "unexpected number of bytes written");
  return is_small_bytes ? SmallBytes::fromBytes({buffer, result_length})
                        : (is_mutable ? *result : result.becomeImmutable());
}

RawObject Runtime::bytesRepeat(Thread* thread, const Bytes& source, word length,
                               word count) {
  DCHECK(length > 0, "length should be positive");
  DCHECK(count > 0, "count should be positive");
  DCHECK_BOUND(length, source.length());
  DCHECK_BOUND(count, kMaxWord / length);
  bool is_mutable = source.isMutableBytes();
  if (length == 1) {
    byte item = source.byteAt(0);
    return is_mutable ? mutableBytesWith(count, item) : newBytes(count, item);
  }
  word new_length = length * count;
  if (!is_mutable && new_length <= SmallBytes::kMaxLength) {
    byte buffer[SmallBytes::kMaxLength];
    byte* dst = buffer;
    for (word i = 0; i < count; i++, dst += length) {
      source.copyTo(dst, length);
    }
    return SmallBytes::fromBytes({buffer, new_length});
  }
  HandleScope scope(thread);
  MutableBytes result(&scope, newMutableBytesUninitialized(new_length));
  for (word i = 0; i < count * length; i += length) {
    result.replaceFromWith(i, *source, length);
  }
  return is_mutable ? *result : result.becomeImmutable();
}

RawObject Runtime::bytesSlice(Thread* thread, const Bytes& self, word start,
                              word stop, word step) {
  word length = Slice::length(start, stop, step);
  if (length <= SmallBytes::kMaxLength) {
    byte buffer[SmallBytes::kMaxLength];
    for (word i = 0, j = start; i < length; i++, j += step) {
      buffer[i] = self.byteAt(j);
    }
    return SmallBytes::fromBytes({buffer, length});
  }
  HandleScope scope(thread);
  MutableBytes result(&scope, newMutableBytesUninitialized(length));
  {
    byte* dst = reinterpret_cast<byte*>(result.address());
    for (word i = 0, j = start; i < length; i++, j += step) {
      dst[i] = self.byteAt(j);
    }
  }
  return result.becomeImmutable();
}

RawObject Runtime::bytesSubseq(Thread* thread, const Bytes& self, word start,
                               word length) {
  DCHECK_BOUND(start, self.length());
  DCHECK_BOUND(length, self.length() - start);
  if (length <= SmallBytes::kMaxLength) {
    byte buffer[SmallBytes::kMaxLength];
    for (word i = length - 1; i >= 0; i--) {
      buffer[i] = self.byteAt(start + i);
    }
    return SmallBytes::fromBytes({buffer, length});
  }
  HandleScope scope(thread);
  MutableBytes copy(&scope, newMutableBytesUninitialized(length));
  {
    byte* dst = reinterpret_cast<byte*>(copy.address());
    const byte* src =
        reinterpret_cast<byte*>(HeapObject::cast(*self).address());
    std::memcpy(dst, src + start, length);
  }
  return copy.becomeImmutable();
}

RawObject Runtime::bytesTranslate(Thread* thread, const Bytes& self,
                                  word length, const Bytes& table,
                                  word table_len, const Bytes& del,
                                  word del_len) {
  DCHECK_BOUND(length, self.length());
  DCHECK_BOUND(del_len, del.length());
  // calculate mapping table
  byte new_byte[BytesBuiltins::kTranslationTableLength];
  if (table == Bytes::empty()) {
    for (word i = 0; i < BytesBuiltins::kTranslationTableLength; i++) {
      new_byte[i] = i;
    }
  } else {
    DCHECK_BOUND(table_len, table.length());
    DCHECK(table_len == BytesBuiltins::kTranslationTableLength,
           "translation table must map every possible byte value");
    for (word i = 0; i < BytesBuiltins::kTranslationTableLength; i++) {
      new_byte[i] = table.byteAt(i);
    }
  }
  // make initial pass to calculate length
  bool delete_byte[BytesBuiltins::kTranslationTableLength] = {};
  for (word i = 0; i < del_len; i++) {
    delete_byte[del.byteAt(i)] = true;
  }
  word new_length = length;
  for (word i = 0; i < length; i++) {
    if (delete_byte[self.byteAt(i)]) {
      new_length--;
    }
  }
  // replace or delete each byte
  bool is_mutable = self.isMutableBytes();
  if (new_length <= SmallBytes::kMaxLength && !is_mutable) {
    byte buffer[SmallBytes::kMaxLength];
    for (word i = 0, j = 0; j < new_length; i++) {
      DCHECK(i < length, "reached end of self before finishing translation");
      byte current = self.byteAt(i);
      if (!delete_byte[current]) {
        buffer[j++] = new_byte[current];
      }
    }
    return SmallBytes::fromBytes({buffer, new_length});
  }
  HandleScope scope(thread);
  MutableBytes result(&scope, newMutableBytesUninitialized(new_length));
  for (word i = 0, j = 0; j < new_length; i++) {
    DCHECK(i < length, "reached end of self before finishing translation");
    byte current = self.byteAt(i);
    if (!delete_byte[current]) {
      result.byteAtPut(j++, new_byte[current]);
    }
  }
  return is_mutable ? *result : result.becomeImmutable();
}

// List

void Runtime::listEnsureCapacity(Thread* thread, const List& list,
                                 word min_capacity) {
  DCHECK_BOUND(min_capacity, SmallInt::kMaxValue);
  word curr_capacity = list.capacity();
  if (min_capacity <= curr_capacity) return;
  word new_capacity = newCapacity(curr_capacity, min_capacity);
  HandleScope scope(thread);
  Tuple old_array(&scope, list.items());
  Tuple new_array(&scope, newTuple(new_capacity));
  old_array.copyTo(*new_array);
  list.setItems(*new_array);
}

void Runtime::listAdd(Thread* thread, const List& list, const Object& value) {
  word index = list.numItems();
  listEnsureCapacity(thread, list, index + 1);
  list.setNumItems(index + 1);
  list.atPut(index, *value);
}

// Dict

RawObject Runtime::newDict() {
  HandleScope scope;
  Dict result(&scope, heap()->create<RawDict>());
  result.setNumItems(0);
  result.setData(empty_tuple_);
  result.setNumUsableItems(0);
  return *result;
}

RawObject Runtime::newDictWithSize(word initial_size) {
  HandleScope scope;
  // TODO(jeethu): initialSize should be scaled up by a load factor.
  word initial_capacity = Utils::maximum(
      static_cast<word>(kInitialDictCapacity),
      Utils::nextPowerOfTwo(initial_size) * Runtime::kDictGrowthFactor);
  Tuple array(&scope, newTuple(initial_capacity * Dict::Bucket::kNumPointers));
  Dict result(&scope, newDict());
  result.setNumItems(0);
  result.setData(*array);
  result.resetNumUsableItems();
  return *result;
}

bool Runtime::dictHasEmptyItem(const Tuple& data) {
  for (word index = 0; index < data.length();
       index += Dict::Bucket::kNumPointers) {
    if (Dict::Bucket::isEmpty(*data, index)) {
      return true;
    }
  }
  return false;
}

void Runtime::dictAtPutWithHash(Thread* thread, const Dict& dict,
                                const Object& key, const Object& value,
                                const Object& key_hash) {
  // TODO(T44245141): Move initialization of an empty dict to
  // dictEnsureCapacity.
  if (dict.capacity() == 0) {
    dict.setData(
        newTuple(Runtime::kInitialDictCapacity * Dict::Bucket::kNumPointers));
    dict.resetNumUsableItems();
  }
  HandleScope scope(thread);
  Tuple data(&scope, dict.data());
  word index = -1;
  bool found = dictLookup(data, key, key_hash, &index, RawObject::equals);
  DCHECK(index != -1, "invalid index %ld", index);
  bool empty_slot = Dict::Bucket::isEmpty(*data, index);
  Dict::Bucket::set(*data, index, *key_hash, *key, *value);
  if (found) {
    return;
  }
  dict.setNumItems(dict.numItems() + 1);
  if (empty_slot) {
    dict.decrementNumUsableItems();
    dictEnsureCapacity(thread, dict);
  }
  DCHECK(dictHasEmptyItem(data), "dict must have at least an empty item");
}

void Runtime::dictAtPut(Thread* thread, const Dict& dict, const Object& key,
                        const Object& value) {
  HandleScope scope(thread);
  Object key_hash(&scope, hash(*key));
  dictAtPutWithHash(thread, dict, key, value, key_hash);
}

void Runtime::dictEnsureCapacity(Thread* thread, const Dict& dict) {
  // TODO(T44245141): Move initialization of an empty dict here.
  DCHECK(dict.capacity() > 0 && Utils::isPowerOfTwo(dict.capacity()),
         "dict capacity must be power of two, greater than zero");
  if (dict.hasUsableItems()) {
    return;
  }
  // TODO(T44247845): Handle overflow here.
  word new_capacity = dict.capacity() * kDictGrowthFactor;
  HandleScope scope(thread);
  Tuple data(&scope, dict.data());
  Tuple new_data(&scope, newTuple(new_capacity * Dict::Bucket::kNumPointers));
  // Re-insert items
  for (word i = Dict::Bucket::kFirst; Dict::Bucket::nextItem(*data, &i);) {
    Object key(&scope, Dict::Bucket::key(*data, i));
    Object hash(&scope, Dict::Bucket::hash(*data, i));
    word index = -1;
    dictLookup(new_data, key, hash, &index, RawObject::equals);
    DCHECK(index != -1, "invalid index %ld", index);
    Dict::Bucket::set(*new_data, index, *hash, *key,
                      Dict::Bucket::value(*data, i));
  }
  dict.setData(*new_data);
  dict.resetNumUsableItems();
}

RawObject Runtime::dictAtWithHash(Thread* thread, const Dict& dict,
                                  const Object& key, const Object& key_hash) {
  HandleScope scope(thread);
  Tuple data(&scope, dict.data());
  word index = -1;
  bool found = dictLookup(data, key, key_hash, &index, RawObject::equals);
  if (found) {
    return Dict::Bucket::value(*data, index);
  }
  return Error::notFound();
}

RawObject Runtime::dictAt(Thread* thread, const Dict& dict, const Object& key) {
  HandleScope scope(thread);
  Object key_hash(&scope, hash(*key));
  return dictAtWithHash(thread, dict, key, key_hash);
}

RawObject Runtime::dictAtIfAbsentPut(Thread* thread, const Dict& dict,
                                     const Object& key,
                                     Callback<RawObject>* thunk) {
  // TODO(T44245141): Move initialization of an empty dict to
  // dictEnsureCapacity.
  if (dict.capacity() == 0) {
    dict.setData(
        newTuple(Runtime::kInitialDictCapacity * Dict::Bucket::kNumPointers));
    dict.resetNumUsableItems();
  }
  HandleScope scope(thread);
  Tuple data(&scope, dict.data());
  word index = -1;
  Object key_hash(&scope, hash(*key));
  bool found = dictLookup(data, key, key_hash, &index, RawObject::equals);
  DCHECK(index != -1, "invalid index %ld", index);
  if (found) {
    return Dict::Bucket::value(*data, index);
  }
  bool empty_slot = Dict::Bucket::isEmpty(*data, index);
  Object value(&scope, thunk->call());
  Dict::Bucket::set(*data, index, *key_hash, *key, *value);
  dict.setNumItems(dict.numItems() + 1);
  if (empty_slot) {
    DCHECK(dict.numUsableItems() > 0, "dict.numIsableItems() must be positive");
    dict.decrementNumUsableItems();
    dictEnsureCapacity(thread, dict);
  }
  DCHECK(dictHasEmptyItem(data), "dict must have at least an empty item");
  return *value;
}

RawObject Runtime::dictAtPutInValueCell(Thread* thread, const Dict& dict,
                                        const Object& key,
                                        const Object& value) {
  RawObject result =
      dictAtIfAbsentPut(thread, dict, key, newValueCellCallback());
  ValueCell::cast(result).setValue(*value);
  return result;
}

bool Runtime::dictIncludes(Thread* thread, const Dict& dict,
                           const Object& key) {
  HandleScope scope;
  // TODO(T36757907): Check if key is hashable
  Object key_hash(&scope, hash(*key));
  return dictIncludesWithHash(thread, dict, key, key_hash);
}

bool Runtime::dictIncludesWithHash(Thread* thread, const Dict& dict,
                                   const Object& key, const Object& key_hash) {
  HandleScope scope(thread);
  Tuple data(&scope, dict.data());
  word ignore;
  return dictLookup(data, key, key_hash, &ignore, RawObject::equals);
}

RawObject Runtime::dictRemove(Thread* thread, const Dict& dict,
                              const Object& key) {
  HandleScope scope(thread);
  Object key_hash(&scope, hash(*key));
  return dictRemoveWithHash(thread, dict, key, key_hash);
}

RawObject Runtime::dictRemoveWithHash(Thread* thread, const Dict& dict,
                                      const Object& key,
                                      const Object& key_hash) {
  HandleScope scope(thread);
  Tuple data(&scope, dict.data());
  word index = -1;
  Object result(&scope, Error::notFound());
  bool found = dictLookup(data, key, key_hash, &index, RawObject::equals);
  if (found) {
    result = Dict::Bucket::value(*data, index);
    Dict::Bucket::setTombstone(*data, index);
    dict.setNumItems(dict.numItems() - 1);
  }
  return *result;
}

bool Runtime::dictLookup(const Tuple& data, const Object& key,
                         const Object& key_hash, word* index, DictEq pred) {
  if (data.length() == 0) {
    *index = -1;
    return false;
  }
  DCHECK(dictHasEmptyItem(data),
         "dict must be non-empty and have at least an empty item to guarantee "
         "termination of lookup");
  word bucket_mask;
  // hash value left shifted per probing to use different bits
  // for probing.
  uword perturb;
  word current = Dict::Bucket::bucket(*data, *key_hash, &bucket_mask, &perturb);
  word current_index = current * Dict::Bucket::kNumPointers;
  word next_free_index = -1;
  for (;;) {
    if (Dict::Bucket::isEmpty(*data, current_index)) {
      if (next_free_index == -1) {
        next_free_index = current_index;
      }
      *index = next_free_index;
      return false;
    }
    if (Dict::Bucket::isTombstone(*data, current_index)) {
      if (next_free_index == -1) {
        next_free_index = current_index;
      }
    } else if (Dict::Bucket::hasKey(*data, current_index, *key, pred)) {
      *index = current_index;
      return true;
    }
    current = Dict::Bucket::nextBucket(current, bucket_mask, &perturb);
    current_index = current * Dict::Bucket::kNumPointers;
  }
}

RawObject Runtime::dictItems(Thread* thread, const Dict& dict) {
  HandleScope scope(thread);
  Tuple data(&scope, dict.data());
  Tuple items(&scope, newTuple(dict.numItems()));
  word num_items = 0;
  for (word i = Dict::Bucket::kFirst; Dict::Bucket::nextItem(*data, &i);) {
    Tuple kvpair(&scope, newTuple(2));
    kvpair.atPut(0, Dict::Bucket::key(*data, i));
    kvpair.atPut(1, Dict::Bucket::value(*data, i));
    items.atPut(num_items++, *kvpair);
  }
  return *items;
}

RawObject Runtime::dictKeys(Thread* thread, const Dict& dict) {
  HandleScope scope(thread);
  Tuple data(&scope, dict.data());
  Tuple keys(&scope, newTuple(dict.numItems()));
  word num_keys = 0;
  for (word i = Dict::Bucket::kFirst; Dict::Bucket::nextItem(*data, &i);) {
    DCHECK(num_keys < keys.length(), "%ld ! < %ld", num_keys, keys.length());
    keys.atPut(num_keys, Dict::Bucket::key(*data, i));
    num_keys++;
  }
  return *keys;
}

RawObject Runtime::dictValues(Thread* thread, const Dict& dict) {
  HandleScope scope(thread);
  Tuple data(&scope, dict.data());
  Tuple values(&scope, newTuple(dict.numItems()));
  word num_values = 0;
  for (word i = Dict::Bucket::kFirst; Dict::Bucket::nextItem(*data, &i);) {
    values.atPut(num_values++, Dict::Bucket::value(*data, i));
  }
  return *values;
}

// DictItemIterator

RawObject Runtime::newDictItemIterator(Thread* thread, const Dict& dict) {
  HandleScope scope(thread);
  DictItemIterator result(&scope, heap()->create<RawDictItemIterator>());
  result.setIndex(Dict::Bucket::kFirst);
  result.setIterable(*dict);
  result.setNumFound(0);
  return *result;
}

// DictItems

RawObject Runtime::newDictItems(Thread* thread, const Dict& dict) {
  HandleScope scope(thread);
  DictItems result(&scope, heap()->create<RawDictItems>());
  result.setDict(*dict);
  return *result;
}

// DictKeyIterator

RawObject Runtime::newDictKeyIterator(Thread* thread, const Dict& dict) {
  HandleScope scope(thread);
  DictKeyIterator result(&scope, heap()->create<RawDictKeyIterator>());
  result.setIndex(Dict::Bucket::kFirst);
  result.setIterable(*dict);
  result.setNumFound(0);
  return *result;
}

// DictKeys

RawObject Runtime::newDictKeys(Thread* thread, const Dict& dict) {
  HandleScope scope(thread);
  DictKeys result(&scope, heap()->create<RawDictKeys>());
  result.setDict(*dict);
  return *result;
}

// DictValueIterator

RawObject Runtime::newDictValueIterator(Thread* thread, const Dict& dict) {
  HandleScope scope(thread);
  DictValueIterator result(&scope, heap()->create<RawDictValueIterator>());
  result.setIndex(Dict::Bucket::kFirst);
  result.setIterable(*dict);
  result.setNumFound(0);
  return *result;
}

// DictValues

RawObject Runtime::newDictValues(Thread* thread, const Dict& dict) {
  HandleScope scope(thread);
  DictValues result(&scope, heap()->create<RawDictValues>());
  result.setDict(*dict);
  return *result;
}

// Set

RawObject Runtime::newSet() {
  HandleScope scope;
  Set result(&scope, heap()->create<RawSet>());
  result.setNumItems(0);
  result.setData(empty_tuple_);
  return *result;
}

RawObject Runtime::newFrozenSet() {
  HandleScope scope;
  FrozenSet result(&scope, heap()->create<RawFrozenSet>());
  result.setNumItems(0);
  result.setData(empty_tuple_);
  return *result;
}

template <SetLookupType type>
word Runtime::setLookup(const Tuple& data, const Object& key,
                        const Object& key_hash) {
  word start = SetBase::Bucket::getIndex(*data, *key_hash);
  word current = start;
  word next_free_index = -1;

  // TODO(mpage) - Quadratic probing?
  word length = data.length();
  if (length == 0) {
    return -1;
  }

  do {
    if (SetBase::Bucket::hasKey(*data, current, *key)) {
      return current;
    }
    if (next_free_index == -1 && SetBase::Bucket::isTombstone(*data, current)) {
      if (type == SetLookupType::Insertion) {
        return current;
      }
      next_free_index = current;
    } else if (SetBase::Bucket::isEmpty(*data, current)) {
      if (next_free_index == -1) {
        next_free_index = current;
      }
      break;
    }
    current = (current + SetBase::Bucket::kNumPointers) & (length - 1);
  } while (current != start);

  if (type == SetLookupType::Insertion) {
    return next_free_index;
  }
  return -1;
}

RawTuple Runtime::setGrow(Thread* thread, const Tuple& data) {
  HandleScope scope(thread);
  word new_length = data.length() * kSetGrowthFactor;
  if (new_length == 0) {
    new_length = kInitialSetCapacity * SetBase::Bucket::kNumPointers;
  }
  Tuple new_data(&scope, newTuple(new_length));
  // Re-insert items
  for (word i = SetBase::Bucket::kFirst;
       SetBase::Bucket::nextItem(*data, &i);) {
    Object key(&scope, SetBase::Bucket::key(*data, i));
    Object hash(&scope, SetBase::Bucket::hash(*data, i));
    word index = setLookup<SetLookupType::Insertion>(new_data, key, hash);
    DCHECK(index != -1, "unexpected index %ld", index);
    SetBase::Bucket::set(*new_data, index, *hash, *key);
  }
  return *new_data;
}

RawObject Runtime::setAddWithHash(Thread* thread, const SetBase& set,
                                  const Object& value, const Object& key_hash) {
  HandleScope scope(thread);
  Tuple data(&scope, set.data());
  word index = setLookup<SetLookupType::Lookup>(data, value, key_hash);
  if (index != -1) {
    return SetBase::Bucket::key(*data, index);
  }
  Tuple new_data(&scope, *data);
  if (data.length() == 0 || set.numItems() >= data.length() / 2) {
    new_data = setGrow(thread, data);
  }
  index = setLookup<SetLookupType::Insertion>(new_data, value, key_hash);
  DCHECK(index != -1, "unexpected index %ld", index);
  set.setData(*new_data);
  SetBase::Bucket::set(*new_data, index, *key_hash, *value);
  set.setNumItems(set.numItems() + 1);
  return *value;
}

RawObject Runtime::setAdd(Thread* thread, const SetBase& set,
                          const Object& value) {
  HandleScope scope(thread);
  Object key_hash(&scope, hash(*value));
  return setAddWithHash(thread, set, value, key_hash);
}

bool Runtime::setIncludes(Thread* thread, const SetBase& set,
                          const Object& value) {
  HandleScope scope(thread);
  Tuple data(&scope, set.data());
  Object key_hash(&scope, hash(*value));
  return setLookup<SetLookupType::Lookup>(data, value, key_hash) != -1;
}

RawObject Runtime::setIntersection(Thread* thread, const SetBase& set,
                                   const Object& iterable) {
  HandleScope scope(thread);
  SetBase dst(&scope, isInstanceOfSet(*set) ? Runtime::newSet()
                                            : Runtime::newFrozenSet());
  Object key(&scope, NoneType::object());
  Object key_hash(&scope, NoneType::object());
  // Special case for sets
  if (isInstanceOfSetBase(*iterable)) {
    SetBase self(&scope, *set);
    SetBase other(&scope, *iterable);
    if (set.numItems() == 0 || other.numItems() == 0) {
      return *dst;
    }
    // Iterate over the smaller set
    if (set.numItems() > other.numItems()) {
      self = *iterable;
      other = *set;
    }
    Tuple data(&scope, self.data());
    Tuple other_data(&scope, other.data());
    for (word i = SetBase::Bucket::kFirst;
         SetBase::Bucket::nextItem(*data, &i);) {
      key = SetBase::Bucket::key(*data, i);
      key_hash = SetBase::Bucket::hash(*data, i);
      if (setLookup<SetLookupType::Lookup>(other_data, key, key_hash) != -1) {
        setAddWithHash(thread, dst, key, key_hash);
      }
    }
    return *dst;
  }
  // Generic case
  Object iter_method(
      &scope, Interpreter::lookupMethod(thread, thread->currentFrame(),
                                        iterable, SymbolId::kDunderIter));
  if (iter_method.isError()) {
    return thread->raiseWithFmt(LayoutId::kTypeError, "object is not iterable");
  }
  Object iterator(&scope,
                  Interpreter::callMethod1(thread, thread->currentFrame(),
                                           iter_method, iterable));
  if (iterator.isError()) {
    return thread->raiseWithFmt(LayoutId::kTypeError, "object is not iterable");
  }
  Object next_method(
      &scope, Interpreter::lookupMethod(thread, thread->currentFrame(),
                                        iterator, SymbolId::kDunderNext));
  if (next_method.isError()) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "iter() returned a non-iterator");
  }
  if (set.numItems() == 0) {
    return *dst;
  }
  Tuple data(&scope, set.data());
  for (;;) {
    key = Interpreter::callMethod1(thread, thread->currentFrame(), next_method,
                                   iterator);
    if (key.isError()) {
      if (thread->clearPendingStopIteration()) break;
      return *key;
    }
    key_hash = hash(*key);
    if (setLookup<SetLookupType::Lookup>(data, key, key_hash) != -1) {
      setAddWithHash(thread, dst, key, key_hash);
    }
  }
  return *dst;
}

bool Runtime::setRemove(Thread* thread, const Set& set, const Object& value) {
  HandleScope scope(thread);
  Tuple data(&scope, set.data());
  Object key_hash(&scope, hash(*value));
  // TODO(T36757907): Raise TypeError if key is unhashable
  word index = setLookup<SetLookupType::Lookup>(data, value, key_hash);
  if (index != -1) {
    SetBase::Bucket::setTombstone(*data, index);
    set.setNumItems(set.numItems() - 1);
    return true;
  }
  return false;
}

RawObject Runtime::setUpdate(Thread* thread, const SetBase& dst,
                             const Object& iterable) {
  HandleScope scope(thread);
  Object elt(&scope, NoneType::object());
  // Special case for lists
  if (iterable.isList()) {
    List src(&scope, *iterable);
    for (word i = 0; i < src.numItems(); i++) {
      elt = src.at(i);
      setAdd(thread, dst, elt);
    }
    return *dst;
  }
  // Special case for lists iterators
  if (iterable.isListIterator()) {
    ListIterator list_iter(&scope, *iterable);
    List src(&scope, list_iter.iterable());
    for (word i = 0; i < src.numItems(); i++) {
      elt = src.at(i);
      setAdd(thread, dst, elt);
    }
  }
  // Special case for tuples
  if (iterable.isTuple()) {
    Tuple tuple(&scope, *iterable);
    if (tuple.length() > 0) {
      for (word i = 0; i < tuple.length(); i++) {
        elt = tuple.at(i);
        setAdd(thread, dst, elt);
      }
    }
    return *dst;
  }
  // Special case for built-in set types
  if (isInstanceOfSetBase(*iterable)) {
    SetBase src(&scope, *iterable);
    Tuple data(&scope, src.data());
    if (src.numItems() > 0) {
      Object hash(&scope, NoneType::object());
      for (word i = SetBase::Bucket::kFirst;
           SetBase::Bucket::nextItem(*data, &i);) {
        elt = SetBase::Bucket::key(*data, i);
        // take hash from data to avoid recomputing it.
        hash = SetBase::Bucket::hash(*data, i);
        setAddWithHash(thread, dst, elt, hash);
      }
    }
    return *dst;
  }
  // Special case for dicts
  if (iterable.isDict()) {
    Dict dict(&scope, *iterable);
    if (dict.numItems() > 0) {
      Tuple keys(&scope, dictKeys(thread, dict));
      Object value(&scope, NoneType::object());
      for (word i = 0; i < keys.length(); i++) {
        value = keys.at(i);
        setAdd(thread, dst, value);
      }
    }
    return *dst;
  }
  // Generic case
  Object iter_method(
      &scope, Interpreter::lookupMethod(thread, thread->currentFrame(),
                                        iterable, SymbolId::kDunderIter));
  if (iter_method.isError()) {
    return thread->raiseWithFmt(LayoutId::kTypeError, "object is not iterable");
  }
  Object iterator(&scope,
                  Interpreter::callMethod1(thread, thread->currentFrame(),
                                           iter_method, iterable));
  if (iterator.isError()) {
    return thread->raiseWithFmt(LayoutId::kTypeError, "object is not iterable");
  }
  Object next_method(
      &scope, Interpreter::lookupMethod(thread, thread->currentFrame(),
                                        iterator, SymbolId::kDunderNext));
  if (next_method.isError()) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "iter() returned a non-iterator");
  }
  Object value(&scope, NoneType::object());
  for (;;) {
    value = Interpreter::callMethod1(thread, thread->currentFrame(),
                                     next_method, iterator);
    if (value.isError()) {
      if (thread->clearPendingStopIteration()) break;
      return *value;
    }
    setAdd(thread, dst, value);
  }
  return *dst;
}

RawObject Runtime::tupleSubseq(Thread* thread, const Tuple& self, word start,
                               word length) {
  DCHECK_BOUND(start, self.length());
  DCHECK_BOUND(length, self.length() - start);
  if (length == 0) return empty_tuple_;
  HandleScope scope(thread);
  Tuple result(&scope, newTuple(length));
  for (word i = 0; i < length; i++) {
    result.atPut(i, self.at(i + start));
  }
  return *result;
}

// Push a new Frame below caller_frame, and copy a HeapFrame into it. Stack
// overflow checks must be done by the caller. Returns a pointer to the new
// stack Frame.
static Frame* copyHeapFrameToStackFrame(const HeapFrame& heap_frame,
                                        Frame* caller_frame) {
  auto src_base = reinterpret_cast<RawObject*>(heap_frame.address() +
                                               RawHeapFrame::kFrameOffset);
  word frame_words = heap_frame.numFrameWords();
  RawObject* dest_base = caller_frame->valueStackTop() - frame_words;
  std::memcpy(dest_base, src_base, frame_words * kPointerSize);

  auto live_frame =
      reinterpret_cast<Frame*>(dest_base + heap_frame.maxStackSize());
  live_frame->unstashInternalPointers(Function::cast(heap_frame.function()));
  return live_frame;
}

RawObject Runtime::genSend(Thread* thread, const GeneratorBase& gen,
                           const Object& value) {
  HandleScope scope(thread);
  HeapFrame heap_frame(&scope, gen.heapFrame());
  thread->checkStackOverflow(heap_frame.numFrameWords() * kPointerSize);
  Frame* live_frame =
      copyHeapFrameToStackFrame(heap_frame, thread->currentFrame());
  if (live_frame->virtualPC() != 0) {
    live_frame->pushValue(*value);
  }
  thread->linkFrame(live_frame);

  // TODO(T38009294): Improve the compiler to avoid this exception state
  // overhead on every generator entry.
  ExceptionState exc_state(&scope, gen.exceptionState());
  exc_state.setPrevious(thread->caughtExceptionState());
  thread->setCaughtExceptionState(*exc_state);

  Object result(&scope, Interpreter::execute(thread));
  thread->setCaughtExceptionState(exc_state.previous());
  exc_state.setPrevious(NoneType::object());
  return *result;
}

// Copy a Frame from the stack back into a HeapFrame.
static void copyStackFrameToHeapFrame(Frame* live_frame,
                                      const HeapFrame& heap_frame) {
  auto dest_base = reinterpret_cast<RawObject*>(heap_frame.address() +
                                                RawHeapFrame::kFrameOffset);
  RawObject* src_base =
      live_frame->valueStackBase() - heap_frame.maxStackSize();
  std::memcpy(dest_base, src_base, heap_frame.numFrameWords() * kPointerSize);
  heap_frame.stashInternalPointers(live_frame);
}

void Runtime::genSave(Thread* thread, const GeneratorBase& gen) {
  HandleScope scope(thread);
  HeapFrame heap_frame(&scope, gen.heapFrame());
  Frame* live_frame = thread->currentFrame();
  DCHECK(live_frame->valueStackSize() <= heap_frame.maxStackSize(),
         "not enough space in RawGeneratorBase to save live stack");
  copyStackFrameToHeapFrame(live_frame, heap_frame);
}

RawObject Runtime::newValueCell() { return heap()->create<RawValueCell>(); }

RawObject Runtime::newWeakLink(Thread* thread, const Object& referent,
                               const Object& prev, const Object& next) {
  HandleScope scope(thread);
  WeakLink link(&scope, heap()->create<RawWeakLink>());
  link.setReferent(*referent);
  link.setCallback(NoneType::object());
  link.setPrev(*prev);
  link.setNext(*next);
  return *link;
}

RawObject Runtime::newWeakRef(Thread* thread, const Object& referent,
                              const Object& callback) {
  HandleScope scope(thread);
  WeakRef ref(&scope, heap()->create<RawWeakRef>());
  ref.setReferent(*referent);
  ref.setCallback(*callback);
  return *ref;
}

void Runtime::collectAttributes(const Code& code, const Dict& attributes) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Bytes bc(&scope, code.code());
  Tuple names(&scope, code.names());

  word len = bc.length();
  for (word i = 0; i < len - 3; i += 2) {
    // If the current instruction is EXTENDED_ARG we must skip it and the next
    // instruction.
    if (bc.byteAt(i) == Bytecode::EXTENDED_ARG) {
      i += 2;
      continue;
    }
    // Check for LOAD_FAST 0 (self)
    if (bc.byteAt(i) != Bytecode::LOAD_FAST || bc.byteAt(i + 1) != 0) {
      continue;
    }
    // Followed by a STORE_ATTR
    if (bc.byteAt(i + 2) != Bytecode::STORE_ATTR) {
      continue;
    }
    word name_index = bc.byteAt(i + 3);
    Object name(&scope, names.at(name_index));
    dictAtPut(thread, attributes, name, name);
  }
}

RawObject Runtime::classConstructor(const Type& type) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Dict type_dict(&scope, type.dict());
  Object init(&scope, symbols()->DunderInit());
  RawObject value = dictAt(thread, type_dict, init);
  if (value.isError()) {
    return NoneType::object();
  }
  return ValueCell::cast(value).value();
}

RawObject Runtime::computeInitialLayout(Thread* thread, const Type& type,
                                        LayoutId base_layout_id) {
  HandleScope scope(thread);
  // Create the layout
  LayoutId layout_id = reserveLayoutId(thread);
  Layout layout(&scope, layoutCreateSubclassWithBuiltins(
                            layout_id, base_layout_id,
                            View<BuiltinAttribute>(nullptr, 0)));

  Tuple mro(&scope, type.mro());
  Dict attrs(&scope, newDict());

  // Collect set of in-object attributes by scanning the __init__ method of
  // each class in the MRO
  for (word i = 0; i < mro.length(); i++) {
    Type mro_type(&scope, mro.at(i));
    Object maybe_init(&scope, classConstructor(mro_type));
    if (!maybe_init.isFunction()) {
      continue;
    }
    Function init(&scope, *maybe_init);
    RawObject maybe_code = init.code();
    if (!maybe_code.isCode()) {
      continue;  // native trampoline
    }
    Code code(&scope, maybe_code);
    if (code.code().isSmallInt()) {
      continue;  // builtin trampoline
    }
    collectAttributes(code, attrs);
  }

  layout.setNumInObjectAttributes(layout.numInObjectAttributes() +
                                  attrs.numItems());
  layoutAtPut(layout_id, *layout);

  return *layout;
}

RawObject Runtime::attributeAt(Thread* thread, const Object& object,
                               const Object& name_str) {
  DCHECK(isInstanceOfStr(*name_str), "name must be a str subclass");
  HandleScope scope(thread);
  Object result(&scope, thread->invokeMethod2(
                            object, SymbolId::kDunderGetattribute, name_str));
  if (!result.isError()) {
    return *result;
  }
  Type pending_exception_type(&scope, thread->pendingExceptionType());
  Type attribute_error(&scope, typeAt(LayoutId::kAttributeError));
  if (!givenExceptionMatches(thread, pending_exception_type, attribute_error)) {
    return *result;
  }

  // Save the attribute error and clear it then attempt to call `__getattr__`.
  Object saved_type(&scope, thread->pendingExceptionType());
  Object saved_value(&scope, thread->pendingExceptionValue());
  Object saved_traceback(&scope, thread->pendingExceptionTraceback());
  thread->clearPendingException();
  result = thread->invokeMethod2(object, SymbolId::kDunderGetattr, name_str);
  if (result.isErrorNotFound()) {
    thread->setPendingExceptionType(*saved_type);
    thread->setPendingExceptionValue(*saved_value);
    thread->setPendingExceptionTraceback(*saved_traceback);
    return Error::exception();
  }
  return *result;
}

RawObject Runtime::attributeAtById(Thread* thread, const Object& receiver,
                                   SymbolId id) {
  HandleScope scope(thread);
  Object name_str(&scope, symbols()->at(id));
  return attributeAt(thread, receiver, name_str);
}

RawObject Runtime::attributeAtByCStr(Thread* thread, const Object& receiver,
                                     const char* name) {
  HandleScope scope(thread);
  Object name_str(&scope, internStrFromCStr(thread, name));
  return attributeAt(thread, receiver, name_str);
}

RawObject Runtime::attributeDel(Thread* thread, const Object& receiver,
                                const Object& name) {
  HandleScope scope(thread);
  // If present, __delattr__ overrides all attribute deletion logic.
  Type type(&scope, typeOf(*receiver));
  Object dunder_delattr(
      &scope, typeLookupSymbolInMro(thread, type, SymbolId::kDunderDelattr));
  RawObject result = NoneType::object();
  if (!dunder_delattr.isError()) {
    result = Interpreter::callMethod2(thread, thread->currentFrame(),
                                      dunder_delattr, receiver, name);
  } else if (isInstanceOfType(*receiver)) {
    result = classDelAttr(thread, receiver, name);
  } else if (receiver.isModule()) {
    result = moduleDelAttr(thread, receiver, name);
  } else {
    result = instanceDelAttr(thread, receiver, name);
  }

  return result;
}

RawObject Runtime::strConcat(Thread* thread, const Str& left,
                             const Str& right) {
  HandleScope scope(thread);
  word left_len = left.charLength();
  word right_len = right.charLength();
  word result_len = left_len + right_len;
  // Small result
  if (result_len <= RawSmallStr::kMaxLength) {
    byte buffer[RawSmallStr::kMaxLength];
    left.copyTo(buffer, left_len);
    right.copyTo(buffer + left_len, right_len);
    return SmallStr::fromBytes(View<byte>(buffer, result_len));
  }
  // Large result
  LargeStr result(&scope, heap()->createLargeStr(result_len));
  left.copyTo(reinterpret_cast<byte*>(result.address()), left_len);
  right.copyTo(reinterpret_cast<byte*>(result.address() + left_len), right_len);
  return *result;
}

RawObject Runtime::strJoin(Thread* thread, const Str& sep, const Tuple& items,
                           word allocated) {
  HandleScope scope(thread);
  word result_len = 0;
  Object elt(&scope, NoneType::object());
  Str str(&scope, Str::empty());
  for (word i = 0; i < allocated; ++i) {
    elt = items.at(i);
    if (!isInstanceOfStr(*elt)) {
      return thread->raiseWithFmt(
          LayoutId::kTypeError,
          "sequence item %w: expected str instance, %T found", i, &elt);
    }
    str = strUnderlying(thread, elt);
    result_len += str.charLength();
  }
  if (allocated > 1) {
    result_len += sep.charLength() * (allocated - 1);
  }
  // Small result
  if (result_len <= RawSmallStr::kMaxLength) {
    byte buffer[RawSmallStr::kMaxLength];
    for (word i = 0, offset = 0; i < allocated; ++i) {
      elt = items.at(i);
      str = strUnderlying(thread, elt);
      word str_len = str.charLength();
      str.copyTo(&buffer[offset], str_len);
      offset += str_len;
      if ((i + 1) < allocated) {
        word sep_len = sep.charLength();
        sep.copyTo(&buffer[offset], sep_len);
        offset += sep.charLength();
      }
    }
    return SmallStr::fromBytes(View<byte>(buffer, result_len));
  }
  // Large result
  LargeStr result(&scope, heap()->createLargeStr(result_len));
  for (word i = 0, offset = 0; i < allocated; ++i) {
    elt = items.at(i);
    str = strUnderlying(thread, elt);
    word str_len = str.charLength();
    str.copyTo(reinterpret_cast<byte*>(result.address() + offset), str_len);
    offset += str_len;
    if ((i + 1) < allocated) {
      word sep_len = sep.charLength();
      sep.copyTo(reinterpret_cast<byte*>(result.address() + offset), sep_len);
      offset += sep_len;
    }
  }
  return *result;
}

RawObject Runtime::strRepeat(Thread* thread, const Str& str, word count) {
  DCHECK(count > 0, "count should be positive");
  byte buffer[SmallStr::kMaxLength];
  word length = str.charLength();
  DCHECK(length > 0, "length should be positive");
  DCHECK_BOUND(count, SmallInt::kMaxValue / length);
  word new_length = length * count;
  if (new_length <= SmallStr::kMaxLength) {
    // SmallStr result
    for (word i = 0; i < new_length; i++) {
      buffer[i] = str.charAt(i % length);
    }
    return SmallStr::fromBytes(View<byte>(buffer, new_length));
  }
  // LargeStr result
  HandleScope scope(thread);
  LargeStr result(&scope, heap()->createLargeStr(new_length));
  const byte* src;
  if (length <= SmallStr::kMaxLength) {
    // SmallStr original
    str.copyTo(buffer, length);
    src = buffer;
  } else {
    // LargeStr original
    LargeStr source(&scope, *str);
    src = reinterpret_cast<byte*>(source.address());
  }
  byte* dst = reinterpret_cast<byte*>(result.address());
  for (word i = 0; i < count; i++, dst += length) {
    std::memcpy(dst, src, length);
  }
  return *result;
}

RawObject Runtime::strSubstr(Thread* thread, const Str& str, word start,
                             word length) {
  DCHECK(start >= 0, "from should be > 0");
  if (length <= 0) {
    return Str::empty();
  }
  word str_len = str.charLength();
  DCHECK(start + length <= str_len, "overflow");
  if (start == 0 && length == str_len) {
    return *str;
  }
  // SmallStr result
  if (length <= RawSmallStr::kMaxLength) {
    byte buffer[RawSmallStr::kMaxLength];
    for (word i = 0; i < length; i++) {
      buffer[i] = str.charAt(start + i);
    }
    return SmallStr::fromBytes(View<byte>(buffer, length));
  }
  // LargeStr result
  HandleScope scope(thread);
  LargeStr source(&scope, *str);
  LargeStr result(&scope, heap()->createLargeStr(length));
  std::memcpy(reinterpret_cast<void*>(result.address()),
              reinterpret_cast<void*>(source.address() + start), length);
  return *result;
}

// StrArray

void Runtime::strArrayAddASCII(Thread* thread, const StrArray& array,
                               byte code_point) {
  DCHECK(code_point <= kMaxASCII, "can only add ASCII in strArrayAddASCII");
  word num_items = array.numItems();
  word new_length = num_items + 1;
  strArrayEnsureCapacity(thread, array, new_length);
  array.setNumItems(new_length);
  MutableBytes::cast(array.items()).byteAtPut(num_items, code_point);
}

void Runtime::strArrayAddStr(Thread* thread, const StrArray& array,
                             const Str& str) {
  word length = str.charLength();
  if (length == 0) return;
  word num_items = array.numItems();
  word new_length = length + num_items;
  strArrayEnsureCapacity(thread, array, new_length);
  byte* dst =
      reinterpret_cast<byte*>(MutableBytes::cast(array.items()).address());
  str.copyTo(dst + num_items, length);
  array.setNumItems(new_length);
}

void Runtime::strArrayEnsureCapacity(Thread* thread, const StrArray& array,
                                     word min_capacity) {
  DCHECK_BOUND(min_capacity, SmallInt::kMaxValue);
  word curr_capacity = array.capacity();
  if (min_capacity <= curr_capacity) return;
  word new_capacity = newCapacity(curr_capacity, min_capacity);
  HandleScope scope(thread);
  MutableBytes new_bytes(&scope, heap()->createMutableBytes(new_capacity));
  byte* dst = reinterpret_cast<byte*>(new_bytes.address());
  word old_length = array.numItems();
  array.copyTo(dst, old_length);
  std::memset(dst + old_length, 0, new_capacity - old_length);
  array.setItems(*new_bytes);
}

// See https://github.com/python/cpython/blob/master/Objects/lnotab_notes.txt
// for details about the line number table format
word Runtime::codeOffsetToLineNum(Thread* thread, const Code& code,
                                  word offset) {
  HandleScope scope(thread);
  Bytes table(&scope, code.lnotab());
  word line = code.firstlineno();
  word cur_offset = 0;
  for (word i = 0; i < table.length(); i += 2) {
    cur_offset += table.byteAt(i);
    if (cur_offset > offset) {
      break;
    }
    line += static_cast<int8_t>(table.byteAt(i + 1));
  }
  return line;
}

bool Runtime::isSubclass(const Type& subclass, const Type& superclass) {
  HandleScope scope;
  Tuple mro(&scope, subclass.mro());
  for (word i = 0; i < mro.length(); i++) {
    if (mro.at(i) == *superclass) {
      return true;
    }
  }
  return false;
}

RawObject Runtime::newClassMethod() { return heap()->create<RawClassMethod>(); }

RawObject Runtime::newSuper() { return heap()->create<RawSuper>(); }

RawObject Runtime::newStrIterator(const Object& str) {
  HandleScope scope;
  StrIterator result(&scope, heap()->create<RawStrIterator>());
  result.setIndex(0);
  result.setIterable(*str);
  return *result;
}

RawObject Runtime::newTupleIterator(const Tuple& tuple, word length) {
  HandleScope scope;
  TupleIterator result(&scope, heap()->create<RawTupleIterator>());
  result.setIndex(0);
  result.setIterable(*tuple);
  result.setTupleLength(length);
  return *result;
}

RawObject Runtime::emptyFrozenSet() { return empty_frozen_set_; }

RawObject Runtime::computeBuiltinBase(Thread* thread, const Type& type) {
  // The base class can only be one of the builtin bases including object.
  // We use the first non-object builtin base if any, throw if multiple.
  HandleScope scope(thread);
  Tuple mro(&scope, type.mro());
  Type object_type(&scope, typeAt(LayoutId::kObject));
  Type candidate(&scope, *object_type);
  // Skip itself since builtin class won't go through this.
  DCHECK(*type == mro.at(0) && type.instanceLayout().isNoneType(),
         "type's layout should not be set at this point");
  for (word i = 1; i < mro.length(); i++) {
    Type mro_type(&scope, mro.at(i));
    if (!mro_type.isBuiltin()) {
      continue;
    }
    Type builtin_base(&scope, typeAt(mro_type.builtinBase()));
    if (*candidate == *object_type) {
      candidate = *mro_type;
    } else if (isSubclass(candidate, builtin_base)) {
      continue;
    } else if (isSubclass(builtin_base, candidate)) {
      candidate = *builtin_base;
    } else {
      return thread->raiseWithFmt(
          LayoutId::kTypeError,
          "multiple bases have instance lay-out conflict");
    }
  }
  return *candidate;
}

bool Runtime::layoutHasDictOverflow(const Layout& layout) {
  // SmallInt -> offset of the dict attribute on the object
  return layout.overflowAttributes().isSmallInt();
}

RawObject Runtime::layoutGetOverflowDict(Thread* thread,
                                         const HeapObject& instance,
                                         const Layout& layout) {
  DCHECK(layout.hasDictOverflow(), "layout must have dict overflow");
  word offset = SmallInt::cast(layout.overflowAttributes()).value();
  HandleScope scope(thread);
  if (instance.instanceVariableAt(offset).isNoneType()) {
    // Lazily initialize the dict
    instance.instanceVariableAtPut(offset, newDict());
  }
  Object overflow(&scope, instance.instanceVariableAt(offset));
  DCHECK(overflow.isDict(), "layout dict overflow must be dict");
  return *overflow;
}

RawObject Runtime::instanceDel(Thread* thread, const HeapObject& instance,
                               const Object& name) {
  HandleScope scope(thread);

  // Make the attribute invisible
  Layout old_layout(&scope, layoutAt(instance.layoutId()));
  Object result(&scope, layoutDeleteAttribute(thread, old_layout, name));
  if (result.isError()) return *result;
  LayoutId new_layout_id = Layout::cast(*result).id();
  instance.setHeader(instance.header().withLayoutId(new_layout_id));

  // Remove the reference to the attribute value from the instance
  AttributeInfo info;
  bool found = layoutFindAttribute(thread, old_layout, name, &info);
  CHECK(found, "couldn't find attribute");

  if (info.isReadOnly()) {
    return thread->raiseWithFmt(LayoutId::kAttributeError,
                                "'%S' attribute is read-only", &name);
  }

  if (info.isInObject()) {
    instance.instanceVariableAtPut(info.offset(), NoneType::object());
  } else {
    Tuple overflow(&scope,
                   instance.instanceVariableAt(old_layout.overflowOffset()));
    overflow.atPut(info.offset(), NoneType::object());
  }

  return NoneType::object();
}

RawObject Runtime::layoutFollowEdge(const List& edges, const Object& label) {
  DCHECK(edges.numItems() % 2 == 0,
         "edges must contain an even number of elements");
  for (word i = 0; i < edges.numItems(); i++) {
    if (edges.at(i) == *label) {
      return edges.at(i + 1);
    }
  }
  return Error::notFound();
}

void Runtime::layoutAddEdge(Thread* thread, const List& edges,
                            const Object& label, const Object& layout) {
  DCHECK(edges.numItems() % 2 == 0,
         "edges must contain an even number of elements");
  listAdd(thread, edges, label);
  listAdd(thread, edges, layout);
}

bool Runtime::layoutFindAttribute(Thread* thread, const Layout& layout,
                                  const Object& name, AttributeInfo* info) {
  HandleScope scope(thread);
  Object iname(&scope, internStr(thread, name));

  // Check in-object attributes
  Tuple in_object(&scope, layout.inObjectAttributes());
  for (word i = 0; i < in_object.length(); i++) {
    Tuple entry(&scope, in_object.at(i));
    if (entry.at(0) == *iname) {
      *info = AttributeInfo(entry.at(1));
      return true;
    }
  }

  // Check overflow attributes
  if (layout.isSealed()) {
    return false;
  }
  // There is an overflow dict; don't try and read the tuple
  if (layout.hasDictOverflow()) {
    return false;
  }
  Tuple overflow(&scope, layout.overflowAttributes());
  for (word i = 0; i < overflow.length(); i++) {
    Tuple entry(&scope, overflow.at(i));
    if (entry.at(0) == *iname) {
      *info = AttributeInfo(entry.at(1));
      return true;
    }
  }

  return false;
}

RawObject Runtime::layoutCreateEmpty(Thread* thread) {
  HandleScope scope(thread);
  Layout result(&scope, newLayout());
  result.setId(reserveLayoutId(thread));
  layoutAtPut(result.id(), *result);
  return *result;
}

RawObject Runtime::layoutCreateChild(Thread* thread, const Layout& layout) {
  HandleScope scope(thread);
  Layout new_layout(&scope, newLayout());
  new_layout.setId(reserveLayoutId(thread));
  new_layout.setDescribedType(layout.describedType());
  new_layout.setNumInObjectAttributes(layout.numInObjectAttributes());
  new_layout.setInObjectAttributes(layout.inObjectAttributes());
  new_layout.setOverflowAttributes(layout.overflowAttributes());
  layoutAtPut(new_layout.id(), *new_layout);
  return *new_layout;
}

RawObject Runtime::layoutAddAttributeEntry(Thread* thread, const Tuple& entries,
                                           const Object& name,
                                           AttributeInfo info) {
  HandleScope scope(thread);
  Tuple new_entries(&scope, newTuple(entries.length() + 1));
  entries.copyTo(*new_entries);

  Tuple entry(&scope, newTuple(2));
  entry.atPut(0, *name);
  entry.atPut(1, info.asSmallInt());
  new_entries.atPut(entries.length(), *entry);

  return *new_entries;
}

RawObject Runtime::layoutAddAttribute(Thread* thread, const Layout& layout,
                                      const Object& name, word flags) {
  HandleScope scope(thread);
  Object iname(&scope, internStr(thread, name));

  // Check if a edge for the attribute addition already exists
  List edges(&scope, layout.additions());
  RawObject result = layoutFollowEdge(edges, iname);
  if (!result.isError()) {
    return result;
  }

  // Create a new layout and figure out where to place the attribute
  Layout new_layout(&scope, layoutCreateChild(thread, layout));
  Tuple inobject(&scope, layout.inObjectAttributes());
  if (inobject.length() < layout.numInObjectAttributes()) {
    AttributeInfo info(inobject.length() * kPointerSize,
                       flags | AttributeFlags::kInObject);
    new_layout.setInObjectAttributes(
        layoutAddAttributeEntry(thread, inobject, name, info));
  } else {
    Tuple overflow(&scope, layout.overflowAttributes());
    AttributeInfo info(overflow.length(), flags);
    new_layout.setOverflowAttributes(
        layoutAddAttributeEntry(thread, overflow, name, info));
  }

  // Add the edge to the existing layout
  Object value(&scope, *new_layout);
  layoutAddEdge(thread, edges, iname, value);

  return *new_layout;
}

RawObject Runtime::layoutDeleteAttribute(Thread* thread, const Layout& layout,
                                         const Object& name) {
  HandleScope scope(thread);

  // See if the attribute exists
  AttributeInfo info;
  if (!layoutFindAttribute(thread, layout, name, &info)) {
    return Error::notFound();
  }

  // Check if an edge exists for removing the attribute
  Object iname(&scope, internStr(thread, name));
  List edges(&scope, layout.deletions());
  RawObject next_layout = layoutFollowEdge(edges, iname);
  if (!next_layout.isError()) {
    return next_layout;
  }

  // No edge was found, create a new layout and add an edge
  Layout new_layout(&scope, layoutCreateChild(thread, layout));
  if (info.isInObject()) {
    // The attribute to be deleted was an in-object attribute, mark it as
    // deleted
    Tuple old_inobject(&scope, layout.inObjectAttributes());
    Tuple new_inobject(&scope, newTuple(old_inobject.length()));
    for (word i = 0; i < old_inobject.length(); i++) {
      Tuple entry(&scope, old_inobject.at(i));
      if (entry.at(0) == *iname) {
        entry = newTuple(2);
        entry.atPut(0, NoneType::object());
        entry.atPut(1, AttributeInfo(0, AttributeFlags::kDeleted).asSmallInt());
      }
      new_inobject.atPut(i, *entry);
    }
    new_layout.setInObjectAttributes(*new_inobject);
  } else {
    // The attribute to be deleted was an overflow attribute, omit it from the
    // new overflow array
    Tuple old_overflow(&scope, layout.overflowAttributes());
    Tuple new_overflow(&scope, newTuple(old_overflow.length() - 1));
    bool is_deleted = false;
    for (word i = 0, j = 0; i < old_overflow.length(); i++) {
      Tuple entry(&scope, old_overflow.at(i));
      if (entry.at(0) == *iname) {
        is_deleted = true;
        continue;
      }
      if (is_deleted) {
        // Need to shift everything down by 1 once we've deleted the attribute
        entry = newTuple(2);
        entry.atPut(0, Tuple::cast(old_overflow.at(i)).at(0));
        entry.atPut(1, AttributeInfo(j, info.flags()).asSmallInt());
      }
      new_overflow.atPut(j, *entry);
      j++;
    }
    new_layout.setOverflowAttributes(*new_overflow);
  }

  // Add the edge to the existing layout
  Object value(&scope, *new_layout);
  layoutAddEdge(thread, edges, iname, value);

  return *new_layout;
}

void Runtime::freeApiHandles() {
  // Dealloc the Module handles first as they are the handle roots
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Dict modules(&scope, modules_);
  Tuple modules_buckets(&scope, modules.data());
  for (word i = Dict::Bucket::kFirst;
       Dict::Bucket::nextItem(*modules_buckets, &i);) {
    Module module(&scope, Dict::Bucket::value(*modules_buckets, i));
    Object module_def(&scope, module.def());
    if (module_def.isInt() && Int::cast(*module_def).asCPtr() != nullptr) {
      auto def =
          reinterpret_cast<PyModuleDef*>(Int::cast(module.def()).asCPtr());
      ApiHandle* handle = ApiHandle::borrowedReference(thread, *module);
      if (def->m_free != nullptr) def->m_free(handle);
      handle->dispose();
    }
  }

  // Cleanly free all the handles that have a reference count of zero.
  // These can be safely deallocated as they are not referenced by any other
  // native object or handle.
  Dict dict(&scope, apiHandles());
  Tuple buckets(&scope, dict.data());
  for (word i = Dict::Bucket::kFirst; Dict::Bucket::nextItem(*buckets, &i);) {
    Object key(&scope, Dict::Bucket::key(*buckets, i));
    ApiHandle* handle = ApiHandle::borrowedReference(thread, *key);
    if (ApiHandle::nativeRefcnt(handle) == 0) handle->dispose();
  }

  // Finally, skip trying to cleanly deallocate the object. Just free
  // the memory without calling the deallocation functions.
  for (word i = Dict::Bucket::kFirst; Dict::Bucket::nextItem(*buckets, &i);) {
    Object key(&scope, Dict::Bucket::key(*buckets, i));
    ApiHandle::borrowedReference(thread, *key)->dispose();
  }
  while (tracked_native_objects_ != nullptr) {
    NativeObjectNode* entry =
        static_cast<NativeObjectNode*>(tracked_native_objects_);
    void* native = entry->native_ptr;
    untrackNativeObject(native);
    std::free(native);
  }
}

RawObject Runtime::iteratorLengthHint(Thread* thread, const Object& iterator) {
  HandleScope scope(thread);
  Object length_hint_method(
      &scope, Interpreter::lookupMethod(thread, thread->currentFrame(),
                                        iterator, SymbolId::kDunderLengthHint));
  if (length_hint_method.isError()) {
    return *length_hint_method;
  }
  Object result(&scope, Interpreter::callMethod1(thread, thread->currentFrame(),
                                                 length_hint_method, iterator));
  if (result.isError()) {
    return *result;
  }
  if (!result.isSmallInt()) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "__length_hint__ returned non-integer value");
  }
  return *result;
}

RawObject Runtime::bytesToInt(Thread* thread, const Bytes& bytes,
                              endian endianness, bool is_signed) {
  word length = bytes.length();
  DCHECK(length <= kMaxWord - kWordSize, "huge length will overflow");
  if (length == 0) {
    return SmallInt::fromWord(0);
  }

  // Positive numbers that end up having the highest bit of their highest digit
  // set need an extra zero digit.
  byte high_byte = bytes.byteAt(endianness == endian::big ? 0 : length - 1);
  bool high_bit = high_byte & (1 << (kBitsPerByte - 1));
  bool extra_digit = high_bit && !is_signed && length % kWordSize == 0;
  word num_digits = (length + (kWordSize - 1)) / kWordSize + extra_digit;
  HandleScope scope(thread);
  LargeInt result(&scope, heap()->createLargeInt(num_digits));

  byte sign_extension = (is_signed && high_bit) ? kMaxByte : 0;
  if (endianness == endian::little && endian::native == endian::little) {
    result.copyFrom(*bytes, sign_extension);
  } else {
    for (word d = 0; d < num_digits; ++d) {
      uword digit = 0;
      for (int w = 0; w < kWordSize; ++w) {
        word idx = d * kWordSize + w;
        byte b;
        if (idx >= length) {
          b = sign_extension;
        } else {
          b = bytes.byteAt(endianness == endian::big ? length - idx - 1 : idx);
        }
        digit |= static_cast<uword>(b) << (w * kBitsPerByte);
      }
      result.digitAtPut(d, digit);
    }
  }
  return normalizeLargeInt(thread, result);
}

RawObject Runtime::normalizeLargeInt(Thread* thread,
                                     const LargeInt& large_int) {
  word shrink_to_digits = large_int.numDigits();
  for (word digit = large_int.digitAt(shrink_to_digits - 1), next_digit;
       shrink_to_digits > 1; --shrink_to_digits, digit = next_digit) {
    next_digit = large_int.digitAt(shrink_to_digits - 2);
    // break if we have neither a redundant sign-extension nor a redundnant
    // zero-extension.
    if ((digit != -1 || next_digit >= 0) && (digit != 0 || next_digit < 0)) {
      break;
    }
  }
  if (shrink_to_digits == 1 && SmallInt::isValid(large_int.digitAt(0))) {
    return SmallInt::fromWord(large_int.digitAt(0));
  }
  if (shrink_to_digits == large_int.numDigits()) {
    return *large_int;
  }

  // Shrink.  Future Optimization: Shrink in-place instead of copying.
  HandleScope scope(thread);
  LargeInt result(&scope, heap()->createLargeInt(shrink_to_digits));
  for (word i = 0; i < shrink_to_digits; ++i) {
    result.digitAtPut(i, large_int.digitAt(i));
  }
  return *result;
}

static uword addWithCarry(uword x, uword y, uword carry_in, uword* carry_out) {
  DCHECK(carry_in <= 1, "carry must be 0 or 1");
  uword sum;
  uword carry0 = __builtin_add_overflow(x, y, &sum);
  uword carry1 = __builtin_add_overflow(sum, carry_in, &sum);
  *carry_out = carry0 | carry1;
  return sum;
}

RawObject Runtime::intAdd(Thread* thread, const Int& left, const Int& right) {
  if (left.isSmallInt() && right.isSmallInt()) {
    // Take a shortcut because we know the result fits in a word.
    word left_digit = SmallInt::cast(*left).value();
    word right_digit = SmallInt::cast(*right).value();
    return newInt(left_digit + right_digit);
  }

  HandleScope scope(thread);
  word left_digits = left.numDigits();
  word right_digits = right.numDigits();
  Int longer(&scope, left_digits > right_digits ? *left : *right);
  Int shorter(&scope, left_digits <= right_digits ? *left : *right);

  word shorter_digits = shorter.numDigits();
  word longer_digits = longer.numDigits();
  word result_digits = longer_digits + 1;
  LargeInt result(&scope, heap()->createLargeInt(result_digits));
  uword carry = 0;
  for (word i = 0; i < shorter_digits; i++) {
    uword sum =
        addWithCarry(longer.digitAt(i), shorter.digitAt(i), carry, &carry);
    result.digitAtPut(i, sum);
  }
  uword shorter_sign_extension = shorter.isNegative() ? kMaxUword : 0;
  for (word i = shorter_digits; i < longer_digits; i++) {
    uword sum =
        addWithCarry(longer.digitAt(i), shorter_sign_extension, carry, &carry);
    result.digitAtPut(i, sum);
  }
  uword longer_sign_extension = longer.isNegative() ? kMaxUword : 0;
  uword high_digit = longer_sign_extension + shorter_sign_extension + carry;
  result.digitAtPut(result_digits - 1, high_digit);
  return normalizeLargeInt(thread, result);
}

RawObject Runtime::intBinaryAnd(Thread* thread, const Int& left,
                                const Int& right) {
  word left_digits = left.numDigits();
  word right_digits = right.numDigits();
  if (left_digits == 1 && right_digits == 1) {
    return newInt(left.asWord() & right.asWord());
  }

  HandleScope scope(thread);
  Int longer(&scope, left_digits > right_digits ? *left : *right);
  Int shorter(&scope, left_digits <= right_digits ? *left : *right);

  word num_digits = longer.numDigits();
  LargeInt result(&scope, heap()->createLargeInt(num_digits));
  for (word i = 0, e = shorter.numDigits(); i < e; ++i) {
    result.digitAtPut(i, longer.digitAt(i) & shorter.digitAt(i));
  }
  if (shorter.isNegative()) {
    for (word i = shorter.numDigits(); i < num_digits; ++i) {
      result.digitAtPut(i, longer.digitAt(i));
    }
  } else {
    for (word i = shorter.numDigits(); i < num_digits; ++i) {
      result.digitAtPut(i, 0);
    }
  }
  return normalizeLargeInt(thread, result);
}

RawObject Runtime::intInvert(Thread* thread, const Int& value) {
  word num_digits = value.numDigits();
  if (num_digits == 1) {
    word value_word = value.asWord();
    return newInt(~value_word);
  }
  HandleScope scope(thread);
  LargeInt large_int(&scope, *value);
  LargeInt result(&scope, heap()->createLargeInt(num_digits));
  for (word i = 0; i < num_digits; ++i) {
    uword digit = large_int.digitAt(i);
    result.digitAtPut(i, ~digit);
  }
  DCHECK(result.isValid(), "valid large integer");
  return *result;
}

RawObject Runtime::intNegate(Thread* thread, const Int& value) {
  word num_digits = value.numDigits();
  if (num_digits == 1) {
    word value_word = value.asWord();
    // Negating kMinWord results in a number with two digits.
    if (value_word == kMinWord) {
      const uword min_word[] = {static_cast<uword>(kMinWord), 0};
      return newIntWithDigits(min_word);
    }
    return newInt(-value_word);
  }

  HandleScope scope(thread);
  LargeInt large_int(&scope, *value);

  auto digits_zero = [&](word up_to_digit) {
    for (word i = 0; i < up_to_digit; i++) {
      if (large_int.digitAt(i) != 0) {
        return false;
      }
    }
    return true;
  };

  // The result of negating a number like `digits == {0, 0, ..., 0x800000.. }`
  // needs an extra digit.
  uword highest_digit = large_int.digitAt(num_digits - 1);
  if (highest_digit == static_cast<uword>(kMinWord) &&
      digits_zero(num_digits - 1)) {
    LargeInt result(&scope, heap()->createLargeInt(num_digits + 1));
    for (word i = 0; i < num_digits; i++) {
      result.digitAtPut(i, large_int.digitAt(i));
    }
    result.digitAtPut(num_digits, 0);
    DCHECK(result.isValid(), "Invalid LargeInt");
    return *result;
  }
  // The result of negating a number like `digits == {0, 0, ..., 0x800000.., 0}`
  // is one digit shorter.
  if (highest_digit == 0 &&
      large_int.digitAt(num_digits - 2) == static_cast<uword>(kMinWord) &&
      digits_zero(num_digits - 2)) {
    LargeInt result(&scope, heap()->createLargeInt(num_digits - 1));
    for (word i = 0; i < num_digits - 1; i++) {
      result.digitAtPut(i, large_int.digitAt(i));
    }
    DCHECK(result.isValid(), "Invalid LargeInt");
    return *result;
  }

  LargeInt result(&scope, heap()->createLargeInt(num_digits));
  word carry = 1;
  for (word i = 0; i < num_digits; i++) {
    uword digit = large_int.digitAt(i);
    static_assert(sizeof(digit) == sizeof(long), "invalid builtin size");
    carry = __builtin_uaddl_overflow(~digit, carry, &digit);
    result.digitAtPut(i, digit);
  }
  DCHECK(carry == 0, "Carry should be zero");
  DCHECK(result.isValid(), "Invalid LargeInt");
  return *result;
}

// The division algorithm operates on half words. This is because to implement
// multiword division we require a doubleword division operation such as
// (`uint128_t / uint64_t -> uint128_t`). Such an operation does not exist on
// most architectures (x86_64 only has `uint128_t / uint64_t -> uint64_t`,
// aarch64 only `uint64_t / uint64_t -> uint64_t`). Instead we perform the
// algorithm on half words and use a `uint64_t / uint32_t -> uint64_t` division.
// This is easier and faster than trying to emulate a doubleword division.
typedef uint32_t halfuword;
static_assert(sizeof(halfuword) == sizeof(uword) / 2, "halfuword size");

const int kBitsPerHalfWord = kBitsPerByte * sizeof(halfuword);

static void halvesInvert(halfuword* halves, word num_halves) {
  for (word i = 0; i < num_halves; i++) {
    halves[i] = ~halves[i];
  }
}

static void halvesNegate(halfuword* halves, word num_halves) {
  uword carry = 1;
  for (word i = 0; i < num_halves; i++) {
    halfuword half = uword{~halves[i]} + carry;
    halves[i] = half;
    carry &= (half == 0);
  }
  DCHECK(carry == 0, "overflow");
}

static halfuword halvesAdd(halfuword* dest, const halfuword* src,
                           word num_halves) {
  halfuword carry = 0;
  for (word i = 0; i < num_halves; i++) {
    uword sum = uword{dest[i]} + src[i] + carry;
    dest[i] = static_cast<halfuword>(sum);
    carry = sum >> kBitsPerHalfWord;
  }
  return carry;
}

static void halvesIncrement(halfuword* halves, word num_halves,
                            bool overflow_ok) {
  for (word i = 0; i < num_halves; i++) {
    halfuword half = halves[i] + 1;
    halves[i] = half;
    // We are done if there was no overflow.
    if (half != 0) break;
    DCHECK(overflow_ok || i < num_halves - 1, "overflow");
  }
}

static void halvesFromIntMagnitude(halfuword* halves, const Int& number) {
  word num_digits = number.numDigits();
  for (word i = 0; i < num_digits; i++) {
    uword digit = number.digitAt(i);
    halves[i * 2] = static_cast<halfuword>(digit);
    halves[i * 2 + 1] = digit >> kBitsPerHalfWord;
  }
  if (number.isNegative()) {
    halvesNegate(halves, num_digits * 2);
  }
}

// Given an array of size `num_halves` checks how many items at the end of the
// array is zero and returns a reduced length without them. Put another way:
// It drops leading zeros from an arbitrary precision little endian number.
static word halvesNormalize(halfuword* halves, word num_halves) {
  while (halves[num_halves - 1] == 0) {
    num_halves--;
    DCHECK(num_halves > 0, "must not have every digit zero");
  }
  return num_halves;
}

static void halvesDecrement(halfuword* halves, word num_halves) {
  DCHECK(num_halves > 0, "must have at least one half");
  for (word i = 0; i < num_halves; i++) {
    halfuword half = halves[i] - 1;
    halves[i] = half;
    // We are done if there is no borrow left.
    if (half != ~halfuword{0}) return;
  }
  // Must only be used in situations that cannot underflow.
  UNREACHABLE("underflow");
}

static void halvesShiftLeft(halfuword* halves, word num_halves, word shift) {
  DCHECK(shift < kBitsPerHalfWord, "must not shift more than a halfuword");
  if (shift == 0) return;

  halfuword prev = 0;
  for (word i = 0; i < num_halves; i++) {
    halfuword half = halves[i];
    halves[i] = (half << shift) | (prev >> (kBitsPerHalfWord - shift));
    prev = half;
  }
  DCHECK(prev >> (kBitsPerHalfWord - shift) == 0, "must not overflow");
}

static void halvesShiftRight(halfuword* halves, word num_halves, word shift) {
  DCHECK(shift < kBitsPerHalfWord, "must not shift more than a halfuword");
  if (shift == 0) return;

  halfuword prev = 0;
  for (word i = num_halves - 1; i >= 0; i--) {
    halfuword half = halves[i];
    halves[i] = (half >> shift) | (prev << (kBitsPerHalfWord - shift));
    prev = half;
  }
}

static RawObject largeIntFromHalves(Thread* thread, const halfuword* halves,
                                    word num_halves) {
  DCHECK(num_halves % 2 == 0, "must have even number of halves");
  word digits = num_halves / 2;
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  LargeInt result(&scope, runtime->heap()->createLargeInt(digits));
  for (word i = 0; i < digits; i++) {
    uword digit =
        halves[i * 2] | (uword{halves[i * 2 + 1]} << kBitsPerHalfWord);
    result.digitAtPut(i, digit);
  }
  return runtime->normalizeLargeInt(thread, result);
}

// Compute quotient and modulo of dividing a large integer through a divisor
// whose magnitude fits in a `halfuword`.
static void divideModuloSingleHalfDivisor(Thread* thread, const Int& dividend,
                                          word divisor, Object* quotient,
                                          Object* modulo) {
  DCHECK(divisor >= 0 ? static_cast<halfuword>(divisor) == divisor
                      : static_cast<halfuword>(-divisor) == -divisor,
         "divisor magnitude fits in half word");

  word dividend_digits = dividend.numDigits();
  bool same_sign = dividend.isNegative() == (divisor < 0);
  halfuword divisor_half = divisor < 0 ? -divisor : divisor;
  uword result_halves = dividend_digits * 2;
  std::unique_ptr<halfuword[]> result(new halfuword[result_halves]);
  halvesFromIntMagnitude(result.get(), dividend);
  if (!same_sign) {
    halvesDecrement(result.get(), result_halves);
  }
  word significant_result_halves = halvesNormalize(result.get(), result_halves);

  halfuword remainder = 0;
  for (word i = significant_result_halves - 1; i >= 0; i--) {
    uword digit = (uword{remainder} << kBitsPerHalfWord) | result[i];
    result[i] = digit / divisor_half;
    remainder = digit % divisor_half;
    // Note that the division result fits into a halfuword, because the upper
    // half is the remainder from last round and therefore smaller than
    // `divisor_half`.
  }

  Runtime* runtime = thread->runtime();
  if (quotient) {
    if (!same_sign) {
      // Compute `-1 - quotient == -1 + (~quotient + 1) == ~quotient`.
      halvesInvert(result.get(), result_halves);
    }

    *quotient = largeIntFromHalves(thread, result.get(), result_halves);
  }
  if (modulo) {
    word modulo_word;
    if (same_sign) {
      modulo_word = remainder;
    } else {
      modulo_word = -static_cast<word>(remainder) + divisor_half - 1;
    }
    if (divisor < 0) {
      modulo_word = -modulo_word;
    }
    *modulo = runtime->newInt(modulo_word);
  }
}

// Perform unsigned integer division with multi-half dividend and divisor.
static void unsignedDivideRemainder(halfuword* result, word result_halves,
                                    halfuword* dividend,
                                    const halfuword* divisor,
                                    word divisor_halves) {
  // See Hackers Delight 9-2 "Multiword Division" and Knuth TAOCP volume 2,
  // 4.3.1 for this algorithm.
  DCHECK(divisor_halves > 1, "need at least 2 divisor halves");
  // Expects the divisor to be normalized by left shifting until the highest bit
  // is set. This ensures that the guess performed in each iteration step is off
  // by no more than 2 (see Knuth for details and a proof).
  DCHECK((divisor[divisor_halves - 1] & (1 << (kBitsPerHalfWord - 1))) != 0,
         "need normalized divisor");

  // Performs some arithmetic with no more than half the bits of a `uword`.
  const uword half_mask = (uword{1} << kBitsPerHalfWord) - 1;

  for (word r = result_halves - 1; r >= 0; r--) {
    // Take the two highest words of the dividend. We implicitly have
    // `dividend_halves = result_halves + divisor_halves - 1` (the actual
    // dividend array is guaranteed to have at least one more half filled with
    // zero on top for the first round). Since the dividend shrinks by 1 half
    // each round, the two highest digits can be found starting at
    // `r + divisor_halves - 1`.
    uword dividend_high_word =
        (uword{dividend[r + divisor_halves]} << kBitsPerHalfWord) |
        uword{dividend[r + divisor_halves - 1]};
    uword divisor_half = divisor[divisor_halves - 1];

    // Guess this result half by dividing the two highest dividend halves.
    // The guess gets us close: `guess_quot - 2 <= quot <= guess_quot`.
    uword guess_quot = dividend_high_word / divisor_half;
    uword guess_remainder = dividend_high_word % divisor_half;

    // Iterate until the guess is exact.
    while (guess_quot > half_mask ||
           guess_quot * divisor[divisor_halves - 2] >
               ((guess_remainder << kBitsPerHalfWord) |
                dividend[r + divisor_halves - 2])) {
      guess_quot--;
      guess_remainder += divisor_half;
      if (guess_remainder > half_mask) break;
    }

    // Multiply and subtract from dividend.
    uword borrow = 0;
    for (word d = 0; d < divisor_halves; d++) {
      uword product = guess_quot * divisor[d];
      word diff =
          static_cast<word>(dividend[d + r]) - borrow - (product & half_mask);
      dividend[d + r] = static_cast<halfuword>(diff);
      borrow = (product >> kBitsPerHalfWord) - (diff >> kBitsPerHalfWord);
    }
    word diff = static_cast<word>(dividend[r + divisor_halves]) - borrow;
    dividend[r + divisor_halves] = static_cast<halfuword>(diff);

    // If we subtracted too much, add back.
    if (diff < 0) {
      guess_quot--;
      halfuword carry = halvesAdd(&dividend[r], divisor, divisor_halves);
      dividend[r + divisor_halves] += carry;
    }

    result[r] = guess_quot;
  }
}

// Like Runtime::intDivideModulo() but specifically for the case of the
// divisor's magnitued being bigger than the dividend's.
static void divideWithBiggerDivisor(Thread* thread, const Int& dividend,
                                    const Int& divisor, Object* quotient,
                                    Object* modulo) {
  if (dividend.isZero()) {
    if (quotient != nullptr) *quotient = SmallInt::fromWord(0);
    if (modulo != nullptr) *modulo = SmallInt::fromWord(0);
    return;
  }
  bool same_sign = dividend.isNegative() == divisor.isNegative();
  if (quotient != nullptr) {
    *quotient = SmallInt::fromWord(same_sign ? 0 : -1);
  }
  if (modulo != nullptr) {
    if (!same_sign) {
      *modulo = thread->runtime()->intAdd(thread, divisor, dividend);
    } else if (dividend.isBool()) {
      *modulo = convertBoolToInt(*dividend);
    } else {
      *modulo = *dividend;
    }
  }
}

bool Runtime::intDivideModulo(Thread* thread, const Int& dividend,
                              const Int& divisor, Object* quotient,
                              Object* modulo) {
  // Some notes for understanding this code:
  // - This is built around an unsigned division algorithm in
  //   `unsignedDivideRemainder()`.
  // - Remember that this implements floor div and modulo which is different
  //   from C++ giving you truncated div and remainder when operands are
  //   negative.
  // - To build a signed floor division from an unsigned division primitive we
  //   use the following formula when the sign of dividend and divisor differs:
  //     floor_div = -1 - (abs(dividend) - 1) / abs(divisor)
  //     modulo    = divisor_sign *
  //                 (abs(divisor) - 1 - (abs(dividend) - 1) % abs(divisor))

  // TODO(matthiasb): Optimization idea: Fuse the independent operations/loops
  // on arrays of `halfuword`s to reduce the number of passes over the data.

  word divisor_digits = divisor.numDigits();
  word dividend_digits = dividend.numDigits();
  bool same_sign = dividend.isNegative() == divisor.isNegative();
  if (divisor_digits == 1) {
    word divisor_word = divisor.asWord();
    if (divisor_word == 0) {
      return false;
    }
    // Handle -1 as a special case because for dividend being the smallest
    // negative number possible for the amount of digits and `divisor == -1`
    // produces a result that is bigger than the input.
    if (divisor_word == -1) {
      if (quotient != nullptr) *quotient = intNegate(thread, dividend);
      if (modulo != nullptr) *modulo = SmallInt::fromWord(0);
      return true;
    }
    if (dividend_digits == 1) {
      word dividend_word = dividend.asWord();
      word quotient_word = dividend_word / divisor_word;
      word modulo_word = dividend_word % divisor_word;
      if (!same_sign && modulo_word) {
        DCHECK(quotient_word > kMinWord, "underflow");
        quotient_word--;
        modulo_word += divisor_word;
      }
      if (quotient != nullptr) *quotient = newInt(quotient_word);
      if (modulo != nullptr) *modulo = newInt(modulo_word);
      return true;
    }

    // Handle the case where `abs(divisor)` fits in single half word.
    // This helps performance and simplifies `unsignedDivideRemainder()` because
    // it can assume to have at least 2 divisor half words.
    word max_half_uword = (word{1} << kBitsPerHalfWord) - 1;
    if (-max_half_uword <= divisor_word && divisor_word <= max_half_uword) {
      divideModuloSingleHalfDivisor(thread, dividend, divisor_word, quotient,
                                    modulo);
      return true;
    }
  }

  if (divisor_digits > dividend_digits) {
    divideWithBiggerDivisor(thread, dividend, divisor, quotient, modulo);
    return true;
  }

  // Convert divisor to `halfuword`s. Normalize by left shifting until the
  // highest bit (of the highest half) is set as required by
  // `unsignedDivideRemainder()`. We count the non-zero halves in the
  // `significant_xxx_halves` variables.
  word divisor_halves = divisor_digits * 2;
  std::unique_ptr<halfuword[]> divisor_n(new halfuword[divisor_halves]);
  halvesFromIntMagnitude(divisor_n.get(), divisor);
  word significant_divisor_halves =
      halvesNormalize(divisor_n.get(), divisor_halves);
  static_assert(sizeof(divisor_n[0]) == sizeof(unsigned),
                "choose right builtin");
  int shift = __builtin_clz(divisor_n[significant_divisor_halves - 1]);
  halvesShiftLeft(divisor_n.get(), significant_divisor_halves, shift);

  // Convert dividend to `halfuword`s and shift by the same amount we used for
  // the divisor. We reserve 1 extra half so we can save a bounds check in
  // `unsignedDivideRemainder()` because `dividend_halves` will still be valid
  // to access at index `significant_divisor_halves`.
  word dividend_halves = (dividend_digits + 1) * 2;
  std::unique_ptr<halfuword[]> dividend_n(new halfuword[dividend_halves]);
  halvesFromIntMagnitude(dividend_n.get(), dividend);
  dividend_n[dividend_halves - 1] = 0;
  dividend_n[dividend_halves - 2] = 0;
  if (!same_sign) {
    halvesDecrement(dividend_n.get(), dividend_halves);
  }
  halvesShiftLeft(dividend_n.get(), dividend_halves, shift);
  word significant_dividend_halves =
      halvesNormalize(dividend_n.get(), dividend_halves);

  // Handle special case of divisor being bigger than the dividend.
  if (significant_divisor_halves > significant_dividend_halves ||
      (significant_divisor_halves == significant_dividend_halves &&
       divisor_n[significant_divisor_halves - 1] >
           dividend_n[significant_divisor_halves - 1])) {
    divideWithBiggerDivisor(thread, dividend, divisor, quotient, modulo);
    return true;
  }

  // Allocate storage for result. Make sure we have an even number of halves.
  word result_halves = (dividend_halves - divisor_halves + 2) & ~1;
  DCHECK(result_halves % 2 == 0, "even number of halves");
  std::unique_ptr<halfuword[]> result(new halfuword[result_halves]);
  word significant_result_halves =
      significant_dividend_halves - significant_divisor_halves + 1;
  DCHECK(significant_result_halves <= result_halves, "no overflow");

  unsignedDivideRemainder(result.get(), significant_result_halves,
                          dividend_n.get(), divisor_n.get(),
                          significant_divisor_halves);

  // TODO(matthiasb): We copy the data in result[] to a new LargeInt,
  // normalizeLargeInt will probably just copy it again. Should we normalize on
  // result[]? Can we do it without duplicating the normalization code?

  if (quotient != nullptr) {
    for (word i = significant_result_halves; i < result_halves; i++) {
      result[i] = 0;
    }
    if (!same_sign) {
      // Compute `-1 - quotient == -1 + (~quotient + 1) == ~quotient`.
      halvesInvert(result.get(), result_halves);
    }

    *quotient = largeIntFromHalves(thread, result.get(), result_halves);
  }
  if (modulo != nullptr) {
    // `dividend` contains the remainder now. First revert normalization shift.
    halvesShiftRight(dividend_n.get(), significant_dividend_halves, shift);
    if (!same_sign) {
      // Revert divisor shift.
      halvesShiftRight(divisor_n.get(), significant_divisor_halves, shift);
      // Compute `-remainder + divisor - 1`.
      halvesNegate(dividend_n.get(), dividend_halves);
      halfuword carry = halvesAdd(dividend_n.get(), divisor_n.get(),
                                  significant_divisor_halves);
      DCHECK(carry <= 1, "carry <= 1");
      if (carry) {
        halvesIncrement(dividend_n.get() + significant_divisor_halves,
                        dividend_halves - significant_divisor_halves, true);
      }

      halvesDecrement(dividend_n.get(), dividend_halves);
    }
    if (divisor.isNegative()) {
      halvesNegate(dividend_n.get(), dividend_halves);
    }

    *modulo = largeIntFromHalves(thread, dividend_n.get(), dividend_halves);
  }

  return true;
}

static uword subtractWithBorrow(uword x, uword y, uword borrow_in,
                                uword* borrow_out) {
  DCHECK(borrow_in <= 1, "borrow must be 0 or 1");
  uword difference;
  uword borrow0 = __builtin_sub_overflow(x, y, &difference);
  uword borrow1 = __builtin_sub_overflow(difference, borrow_in, &difference);
  *borrow_out = borrow0 | borrow1;
  return difference;
}

static void fullMultiply(uword x, uword y, uword* result_low,
                         uword* result_high) {
  static_assert(sizeof(uword) == 8, "assuming uword is 64bit");
  auto result = __extension__ static_cast<unsigned __int128>(x) * y;
  *result_low = static_cast<uword>(result);
  *result_high = static_cast<uword>(result >> 64);
}

RawObject Runtime::intMultiply(Thread* thread, const Int& left,
                               const Int& right) {
  // See also Hackers Delight Chapter 8 Multiplication.
  word left_digits = left.numDigits();
  word right_digits = right.numDigits();
  if (left_digits == 1 && right_digits == 1) {
    word left_digit = static_cast<word>(left.digitAt(0));
    word right_digit = static_cast<word>(right.digitAt(0));
    word result;
    if (!__builtin_mul_overflow(left_digit, right_digit, &result)) {
      return newInt(result);
    }
  }

  HandleScope scope(thread);
  word result_digits = left.numDigits() + right.numDigits();
  LargeInt result(&scope, heap()->createLargeInt(result_digits));

  for (word i = 0; i < result_digits; i++) {
    result.digitAtPut(i, 0);
  }

  // Perform an unsigned multiplication.
  for (word l = 0; l < left_digits; l++) {
    uword digit_left = left.digitAt(l);
    uword carry = 0;
    for (word r = 0; r < right_digits; r++) {
      uword digit_right = right.digitAt(r);
      uword result_digit = result.digitAt(l + r);

      uword product_low;
      uword product_high;
      fullMultiply(digit_left, digit_right, &product_low, &product_high);
      uword carry0;
      uword sum0 = addWithCarry(result_digit, product_low, 0, &carry0);
      uword carry1;
      uword sum1 = addWithCarry(sum0, carry, 0, &carry1);
      result.digitAtPut(l + r, sum1);
      // Note that this cannot overflow: Even with digit_left and digit_right
      // being kMaxUword the result is something like 0xfff...e0000...1, so
      // carry1 will be zero in these cases where the high word is close to
      // overflow.
      carry = product_high + carry0 + carry1;
    }
    result.digitAtPut(l + right_digits, carry);
  }

  // Correct for `left` signedness:
  // Interpreting a negative number as unsigned means we are off by
  // `2**num_bits` (i.e. for a single byte `-3 = 0b11111101` gets interpreted
  // as 253, which is off by `256 = 253 - -3 = 2**8`).
  // Hence if we interpreted a negative `left` as unsigned, the multiplication
  // result will be off by `right * 2**left_num_bits`. We can correct that by
  // subtracting `right << left_num_bits`.
  if (left.isNegative()) {
    uword borrow = 0;
    for (word r = 0; r < right_digits; r++) {
      uword right_digit = right.digitAt(r);
      uword result_digit = result.digitAt(r + left_digits);
      uword difference =
          subtractWithBorrow(result_digit, right_digit, borrow, &borrow);
      result.digitAtPut(r + left_digits, difference);
    }
  }
  // Correct for `right` signedness, analogous to the `left` correction.
  if (right.isNegative()) {
    uword borrow = 0;
    for (word l = 0; l < left_digits; l++) {
      uword left_digit = left.digitAt(l);
      uword result_digit = result.digitAt(l + right_digits);
      uword difference =
          subtractWithBorrow(result_digit, left_digit, borrow, &borrow);
      result.digitAtPut(l + right_digits, difference);
    }
  }

  return normalizeLargeInt(thread, result);
}

RawObject Runtime::intBinaryOr(Thread* thread, const Int& left,
                               const Int& right) {
  word left_digits = left.numDigits();
  word right_digits = right.numDigits();
  if (left_digits == 1 && right_digits == 1) {
    return newInt(left.asWord() | right.asWord());
  }

  HandleScope scope(thread);
  Int longer(&scope, left_digits > right_digits ? *left : *right);
  Int shorter(&scope, left_digits <= right_digits ? *left : *right);
  word num_digits = longer.numDigits();
  LargeInt result(&scope, heap()->createLargeInt(num_digits));
  for (word i = 0, e = shorter.numDigits(); i < e; ++i) {
    result.digitAtPut(i, longer.digitAt(i) | shorter.digitAt(i));
  }
  if (shorter.isNegative()) {
    for (word i = shorter.numDigits(); i < num_digits; ++i) {
      result.digitAtPut(i, kMaxUword);
    }
  } else {
    for (word i = shorter.numDigits(); i < num_digits; ++i) {
      result.digitAtPut(i, longer.digitAt(i));
    }
  }
  return normalizeLargeInt(thread, result);
}

RawObject Runtime::intBinaryRshift(Thread* thread, const Int& num,
                                   const Int& amount) {
  DCHECK(!amount.isNegative(), "shift amount must be positive");
  if (num.numDigits() == 1) {
    if (amount.numDigits() > 1) {
      return SmallInt::fromWord(0);
    }
    word amount_word = amount.asWord();
    if (amount_word >= kBitsPerWord) {
      return SmallInt::fromWord(0);
    }
    word num_word = num.asWord();
    return newInt(num_word >> amount_word);
  }

  word amount_digits = amount.numDigits();
  uword digit0 = amount.digitAt(0);
  word shift_words = digit0 / kBitsPerWord;
  word shift_bits = digit0 % kBitsPerWord;
  if (amount_digits > 1) {
    // It is impossible to create a LargeInt so big that a two-digit amount
    // would result in a non-zero result.
    if (amount_digits > 2) {
      return SmallInt::fromWord(0);
    }
    uword digit1 = amount.digitAt(1);
    // Must fit in a word and be positive.
    if (digit1 / kBitsPerWord / 2 != 0) {
      return SmallInt::fromWord(0);
    }
    shift_words |= digit1 * (kMaxUword / kBitsPerWord + 1);
  }

  word result_digits = num.numDigits() - shift_words;
  if (result_digits < 0) {
    return SmallInt::fromWord(0);
  }
  if (shift_bits == 0 && shift_words == 0) {
    return *num;
  }
  HandleScope scope(thread);
  LargeInt result(&scope, heap()->createLargeInt(result_digits));
  if (shift_bits == 0) {
    for (word i = 0; i < result_digits; i++) {
      result.digitAtPut(i, num.digitAt(shift_words + i));
    }
  } else {
    uword prev = num.isNegative() ? kMaxUword : 0;
    word prev_shift = kBitsPerWord - shift_bits;
    for (word i = result_digits - 1; i >= 0; i--) {
      uword digit = num.digitAt(shift_words + i);
      uword result_digit = prev << prev_shift | digit >> shift_bits;
      result.digitAtPut(i, result_digit);
      prev = digit;
    }
  }
  return normalizeLargeInt(thread, result);
}

RawObject Runtime::intBinaryLshift(Thread* thread, const Int& num,
                                   const Int& amount) {
  DCHECK(!amount.isNegative(), "shift amount must be non-negative");

  word num_digits = num.numDigits();
  if (num_digits == 1 && num.asWord() == 0) return SmallInt::fromWord(0);
  CHECK(amount.numDigits() == 1, "lshift result is too large");

  word amount_word = amount.asWord();
  if (amount_word == 0) {
    if (num.isBool()) {
      return convertBoolToInt(*num);
    }
    return *num;
  }

  word shift_bits = amount_word % kBitsPerWord;
  word shift_words = amount_word / kBitsPerWord;
  word high_digit = num.digitAt(num.numDigits() - 1);

  // check if high digit overflows when shifted - if we need an extra digit
  word bit_length =
      Utils::highestBit(high_digit >= 0 ? high_digit : ~high_digit);
  bool overflow = bit_length + shift_bits >= kBitsPerWord;

  // check if result fits into one word
  word result_digits = num_digits + shift_words + overflow;
  if (result_digits == 1) {
    return newInt(high_digit << shift_bits);
  }

  // allocate large int and zero-initialize low digits
  HandleScope scope(thread);
  LargeInt result(&scope, heap()->createLargeInt(result_digits));
  for (word i = 0; i < shift_words; i++) {
    result.digitAtPut(i, 0);
  }

  // iterate over digits of num and handle carrying
  if (shift_bits == 0) {
    for (word i = 0; i < num_digits; i++) {
      result.digitAtPut(i + shift_words, num.digitAt(i));
    }
    DCHECK(!overflow, "overflow must be false with shift_bits==0");
  } else {
    word right_shift = kBitsPerWord - shift_bits;
    uword prev = 0;
    for (word i = 0; i < num_digits; i++) {
      uword digit = num.digitAt(i);
      uword result_digit = (digit << shift_bits) | (prev >> right_shift);
      result.digitAtPut(i + shift_words, result_digit);
      prev = digit;
    }
    if (overflow) {
      // signed shift takes cares of keeping the sign
      word overflow_digit = static_cast<word>(prev) >> right_shift;
      result.digitAtPut(result_digits - 1, static_cast<uword>(overflow_digit));
    }
  }
  DCHECK(result.isValid(), "result must be valid");
  return *result;
}

RawObject Runtime::intBinaryXor(Thread* thread, const Int& left,
                                const Int& right) {
  word left_digits = left.numDigits();
  word right_digits = right.numDigits();
  if (left_digits == 1 && right_digits == 1) {
    return newInt(left.asWord() ^ right.asWord());
  }

  HandleScope scope(thread);
  Int longer(&scope, left_digits > right_digits ? *left : *right);
  Int shorter(&scope, left_digits <= right_digits ? *left : *right);

  word num_digits = longer.numDigits();
  LargeInt result(&scope, heap()->createLargeInt(num_digits));
  for (word i = 0, e = shorter.numDigits(); i < e; ++i) {
    result.digitAtPut(i, longer.digitAt(i) ^ shorter.digitAt(i));
  }
  if (shorter.isNegative()) {
    for (word i = shorter.numDigits(); i < num_digits; ++i) {
      result.digitAtPut(i, ~longer.digitAt(i));
    }
  } else {
    for (word i = shorter.numDigits(); i < num_digits; ++i) {
      result.digitAtPut(i, longer.digitAt(i));
    }
  }
  return normalizeLargeInt(thread, result);
}

RawObject Runtime::intSubtract(Thread* thread, const Int& left,
                               const Int& right) {
  if (left.isSmallInt() && right.isSmallInt()) {
    // Take a shortcut because we know the result fits in a word.
    word left_digit = SmallInt::cast(*left).value();
    word right_digit = SmallInt::cast(*right).value();
    return newInt(left_digit - right_digit);
  }

  HandleScope scope(thread);
  word left_digits = left.numDigits();
  word right_digits = right.numDigits();

  word shorter_digits = Utils::minimum(left_digits, right_digits);
  word longer_digits = Utils::maximum(left_digits, right_digits);
  word result_digits = longer_digits + 1;
  LargeInt result(&scope, heap()->createLargeInt(result_digits));
  uword borrow = 0;
  for (word i = 0; i < shorter_digits; i++) {
    uword difference =
        subtractWithBorrow(left.digitAt(i), right.digitAt(i), borrow, &borrow);
    result.digitAtPut(i, difference);
  }
  uword left_sign_extension = left.isNegative() ? kMaxUword : 0;
  uword right_sign_extension = right.isNegative() ? kMaxUword : 0;
  if (right_digits == longer_digits) {
    for (word i = shorter_digits; i < longer_digits; i++) {
      uword difference = subtractWithBorrow(left_sign_extension,
                                            right.digitAt(i), borrow, &borrow);
      result.digitAtPut(i, difference);
    }
  } else {
    for (word i = shorter_digits; i < longer_digits; i++) {
      uword difference = subtractWithBorrow(
          left.digitAt(i), right_sign_extension, borrow, &borrow);
      result.digitAtPut(i, difference);
    }
  }
  uword high_digit = left_sign_extension - right_sign_extension - borrow;
  result.digitAtPut(result_digits - 1, high_digit);
  return normalizeLargeInt(thread, result);
}

RawObject Runtime::intToBytes(Thread* thread, const Int& num, word length,
                              endian endianness) {
  HandleScope scope(thread);
  Object result(&scope, Unbound::object());
  byte buffer[SmallBytes::kMaxLength];
  byte* dst;
  if (length <= SmallBytes::kMaxLength) {
    dst = buffer;
  } else {
    result = heap()->createLargeBytes(length);
    dst = reinterpret_cast<byte*>(LargeBytes::cast(*result).address());
  }
  word extension_idx;
  word extension_length;
  if (endianness == endian::little && endian::native == endian::little) {
    word copied = num.copyTo(dst, length);
    extension_idx = copied;
    extension_length = length - copied;
  } else {
    word num_digits = num.numDigits();
    for (word i = 0; i < num_digits; ++i) {
      uword digit = num.digitAt(i);
      for (int x = 0; x < kWordSize; ++x) {
        word idx = i * kWordSize + x;
        byte b = digit & kMaxByte;
        // The last digit may have more (insignificant) bits than the
        // resulting buffer.
        if (idx >= length) {
          return length <= SmallBytes::kMaxLength
                     ? SmallBytes::fromBytes({buffer, length})
                     : *result;
        }
        if (endianness == endian::big) {
          idx = length - idx - 1;
        }
        dst[idx] = b;
        digit >>= kBitsPerByte;
      }
    }
    word num_bytes = num_digits * kWordSize;
    extension_idx = endianness == endian::big ? 0 : num_bytes;
    extension_length = length - num_bytes;
  }
  if (extension_length > 0) {
    byte sign_extension = num.isNegative() ? 0xff : 0;
    for (word i = 0; i < extension_length; ++i) {
      dst[extension_idx + i] = sign_extension;
    }
  }
  return length <= SmallBytes::kMaxLength
             ? SmallBytes::fromBytes({buffer, length})
             : *result;
}

// Str replacement when the result can fit in SmallStr.
static RawObject strReplaceSmallStr(const Str& src, const Str& oldstr,
                                    const Str& newstr, word count,
                                    word result_len) {
  DCHECK_BOUND(result_len, SmallStr::kMaxLength);
  word src_len = src.charLength();
  word old_len = oldstr.charLength();
  word new_len = newstr.charLength();
  byte buffer[SmallStr::kMaxLength];
  byte* dst = buffer;
  for (word i = 0, match_count = 0; i < src_len;) {
    if (match_count == count || !strHasPrefix(src, oldstr, i)) {
      *dst++ = src.charAt(i++);
      continue;
    }
    newstr.copyTo(dst, new_len);
    dst += new_len;
    i += old_len;
    match_count++;
  }
  return SmallStr::fromBytes(View<byte>(buffer, result_len));
}

RawObject Runtime::strReplace(Thread* thread, const Str& src, const Str& oldstr,
                              const Str& newstr, word count) {
  word src_len = src.charLength();
  if (count < 0) {
    count = SmallInt::kMaxValue;  // PY_SSIZE_T_MAX.
  } else if (count == 0 || src_len == 0) {
    return *src;
  }

  if (oldstr.equals(*newstr)) {
    return *src;
  }

  // Update the count to the number of occurences of oldstr in src, capped by
  // the given count.
  count = strCountSubStr(src, oldstr, count);
  if (count == 0) {
    return *src;
  }

  word old_len = oldstr.charLength();
  word new_len = newstr.charLength();
  word result_len = src_len + (new_len - old_len) * count;
  if (result_len <= SmallStr::kMaxLength) {
    return strReplaceSmallStr(src, oldstr, newstr, count, result_len);
  }

  HandleScope scope(thread);
  LargeStr result(&scope, heap()->createLargeStr(result_len));
  word diff = new_len - old_len;
  word offset = 0;
  word match_count = 0;
  word i;
  for (i = 0; i < src_len && match_count < count;) {
    // TODO(T41400083): Use a different search algorithm
    if (strHasPrefix(src, oldstr, i)) {
      byte* dst = reinterpret_cast<byte*>(LargeStr::cast(*result).address());
      newstr.copyTo(dst + i + offset, new_len);
      match_count++;
      offset += diff;
      i += old_len;
      continue;
    }
    byte* dst = reinterpret_cast<byte*>(result.address());
    dst[i + offset] = src.charAt(i);
    i++;
  }

  // Copy the rest of the string.
  if (i < src_len) {
    if (src.isLargeStr()) {
      byte* src_byte = reinterpret_cast<byte*>(LargeStr::cast(*src).address());
      byte* dst = reinterpret_cast<byte*>(result.address());
      std::memcpy(dst + i + offset, src_byte + i, src_len - i);
    } else {
      for (; i < src_len; i++) {
        byte* dst = reinterpret_cast<byte*>(result.address());
        dst[i + offset] = src.charAt(i);
      }
    }
  }

  return *result;
}

word Runtime::nextModuleIndex() { return ++max_module_index_; }

const BuiltinAttribute BuiltinsBase::kAttributes[] = {
    {SymbolId::kSentinelId, -1},
};
const BuiltinMethod BuiltinsBase::kBuiltinMethods[] = {
    {SymbolId::kSentinelId, nullptr},
};

const BuiltinMethod ModuleBaseBase::kBuiltinMethods[] = {
    {SymbolId::kSentinelId, nullptr},
};
const BuiltinType ModuleBaseBase::kBuiltinTypes[] = {
    {SymbolId::kSentinelId, LayoutId::kSentinelId},
};

}  // namespace python
