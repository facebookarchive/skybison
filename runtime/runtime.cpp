#include "runtime.h"

#include <unistd.h>
#include <climits>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <memory>

#include "builtins-module.h"
#include "bytecode.h"
#include "callback.h"
#include "complex-builtins.h"
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
#include "layout.h"
#include "list-builtins.h"
#include "marshal.h"
#include "object-builtins.h"
#include "os.h"
#include "range-builtins.h"
#include "ref-builtins.h"
#include "scavenger.h"
#include "set-builtins.h"
#include "siphash.h"
#include "str-builtins.h"
#include "super-builtins.h"
#include "sys-module.h"
#include "thread.h"
#include "time-module.h"
#include "trampolines-inl.h"
#include "tuple-builtins.h"
#include "type-builtins.h"
#include "utils.h"
#include "visitor.h"

namespace python {

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

Runtime::Runtime(word heap_size)
    : heap_(heap_size), new_value_cell_callback_(this) {
  initializeRandom();
  initializeThreads();
  // This must be called before initializeTypes is called. Methods in
  // initializeTypes rely on instances that are created in this method.
  initializePrimitiveInstances();
  initializeInterned();
  initializeSymbols();
  initializeTypes();
  initializeModules();
  initializeApiData();
}

Runtime::Runtime() : Runtime(64 * kMiB) {}

Runtime::~Runtime() {
  // TODO(T30392425): This is an ugly and fragile workaround for having multiple
  // runtimes created and destroyed by a single thread.
  if (Thread::currentThread() == nullptr) {
    CHECK(threads_ != nullptr, "the runtime does not have any threads");
    Thread::setCurrentThread(threads_);
  }
  freeApiHandles();
  for (Thread* thread = threads_; thread != nullptr;) {
    if (thread == Thread::currentThread()) {
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
  bound_method->setFunction(*function);
  bound_method->setSelf(*self);
  return *bound_method;
}

RawObject Runtime::newLayout() {
  HandleScope scope;
  Layout layout(&scope, heap()->createLayout(LayoutId::kError));
  layout->setInObjectAttributes(empty_object_array_);
  layout->setOverflowAttributes(empty_object_array_);
  layout->setAdditions(newList());
  layout->setDeletions(newList());
  layout->setNumInObjectAttributes(0);
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
  Tuple super_attributes(&scope, super_layout->inObjectAttributes());

  // Sanity check that a subclass that has fixed attributes does inherit from a
  // superclass with attributes that are not fixed.
  for (word i = 0; i < super_attributes->length(); i++) {
    Tuple elt(&scope, super_attributes->at(i));
    AttributeInfo info(elt->at(1));
    CHECK(info.isInObject() && info.isFixedOffset(),
          "all superclass attributes must be in-object and fixed");
  }

  // Create an empty layout for the subclass
  Layout result(&scope, newLayout());
  result->setId(subclass_id);

  // Copy down all of the superclass attributes into the subclass layout
  Tuple in_object(&scope,
                  newTuple(super_attributes->length() + attributes.length()));
  super_attributes->copyTo(*in_object);
  appendBuiltinAttributes(attributes, in_object, super_attributes->length());

  // Install the in-object attributes
  result->setInObjectAttributes(*in_object);
  result->setNumInObjectAttributes(in_object->length());

  return *result;
}

void Runtime::appendBuiltinAttributes(View<BuiltinAttribute> attributes,
                                      const Tuple& dst, word start_index) {
  if (attributes.length() == 0) {
    return;
  }
  HandleScope scope;
  Tuple entry(&scope, empty_object_array_);
  for (word i = 0; i < attributes.length(); i++) {
    AttributeInfo info(
        attributes.get(i).offset,
        AttributeInfo::Flag::kInObject | AttributeInfo::Flag::kFixedOffset);
    entry = newTuple(2);
    SymbolId symbol_id = attributes.get(i).name;
    if (symbol_id == SymbolId::kInvalid) {
      entry->atPut(0, NoneType::object());
    } else {
      entry->atPut(0, symbols()->at(symbol_id));
    }
    entry->atPut(1, info.asSmallInt());
    dst->atPut(start_index + i, *entry);
  }
}

RawObject Runtime::addEmptyBuiltinType(SymbolId name, LayoutId subclass_id,
                                       LayoutId superclass_id) {
  return addBuiltinType(name, subclass_id, superclass_id,
                        View<BuiltinAttribute>(nullptr, 0),
                        View<BuiltinMethod>(nullptr, 0));
}

RawObject Runtime::addBuiltinTypeWithAttrs(SymbolId name, LayoutId subclass_id,
                                           LayoutId superclass_id,
                                           View<BuiltinAttribute> attrs) {
  return addBuiltinType(name, subclass_id, superclass_id, attrs,
                        View<BuiltinMethod>(nullptr, 0));
}

RawObject Runtime::addBuiltinTypeWithMethods(SymbolId name,
                                             LayoutId subclass_id,
                                             LayoutId superclass_id,
                                             View<BuiltinMethod> methods) {
  return addBuiltinType(name, subclass_id, superclass_id,
                        View<BuiltinAttribute>(nullptr, 0), methods);
}

RawObject Runtime::addBuiltinType(SymbolId name, LayoutId subclass_id,
                                  LayoutId superclass_id,
                                  View<BuiltinAttribute> attrs,
                                  View<BuiltinMethod> methods) {
  HandleScope scope;

  // Create a class object for the subclass
  Type subclass(&scope, newType());
  subclass->setName(symbols()->at(name));

  Layout layout(&scope, layoutCreateSubclassWithBuiltins(subclass_id,
                                                         superclass_id, attrs));

  // Assign the layout to the class
  layout->setDescribedType(*subclass);

  // Now we can create an MRO
  Tuple mro(&scope, createMro(layout, superclass_id));

  subclass->setMro(*mro);
  subclass->setInstanceLayout(*layout);
  Type superclass(&scope, typeAt(superclass_id));
  subclass->setFlagsAndBuiltinBase(superclass->flags(), subclass_id);

  // Install the layout and class
  layoutAtPut(subclass_id, *layout);

  // Add the provided methods.
  for (const BuiltinMethod& meth : methods) {
    typeAddBuiltinFunction(subclass, meth.name, meth.address);
  }

  // return the class
  return *subclass;
}

RawObject Runtime::newBytes(word length, byte fill) {
  DCHECK(length >= 0, "invalid length %ld", length);
  if (length == 0) {
    return empty_byte_array_;
  }
  RawObject result = heap()->createBytes(length);
  byte* dst = reinterpret_cast<byte*>(RawBytes::cast(result)->address());
  std::memset(dst, fill, length);
  return result;
}

RawObject Runtime::newBytesWithAll(View<byte> array) {
  if (array.length() == 0) {
    return empty_byte_array_;
  }
  RawObject result = heap()->createBytes(array.length());
  byte* dst = reinterpret_cast<byte*>(RawBytes::cast(result)->address());
  std::memcpy(dst, array.data(), array.length());
  return result;
}

RawObject Runtime::newType() { return newTypeWithMetaclass(LayoutId::kType); }

RawObject Runtime::newTypeWithMetaclass(LayoutId metaclass_id) {
  HandleScope scope;
  Type result(&scope, heap()->createType(metaclass_id));
  Dict dict(&scope, newDict());
  result->setFlagsAndBuiltinBase(static_cast<RawType::Flag>(0),
                                 LayoutId::kObject);
  result->setDict(*dict);
  return *result;
}

RawObject Runtime::classGetAttr(Thread* thread, const Object& receiver,
                                const Object& name) {
  DCHECK(name->isStr(), "Name is not a string");
  HandleScope scope(thread);
  Type type(&scope, *receiver);
  Type meta_type(&scope, typeOf(*receiver));

  // Look for the attribute in the meta class
  Object meta_attr(&scope, lookupNameInMro(thread, meta_type, name));
  if (!meta_attr->isError()) {
    if (isDataDescriptor(thread, meta_attr)) {
      // TODO(T25692531): Call __get__ from meta_attr
      UNIMPLEMENTED("custom descriptors are unsupported");
    }
  }

  // No data descriptor found on the meta class, look in the mro of the type
  Object attr(&scope, lookupNameInMro(thread, type, name));
  if (!attr->isError()) {
    if (isNonDataDescriptor(thread, attr)) {
      Object instance(&scope, NoneType::object());
      return Interpreter::callDescriptorGet(thread, thread->currentFrame(),
                                            attr, instance, receiver);
    }
    return *attr;
  }

  // No attr found in type or its mro, use the non-data descriptor found in
  // the metaclass (if any).
  if (!meta_attr->isError()) {
    if (isNonDataDescriptor(thread, meta_attr)) {
      Object owner(&scope, *meta_type);
      return Interpreter::callDescriptorGet(thread, thread->currentFrame(),
                                            meta_attr, receiver, owner);
    }

    // If a regular attribute was found in the metaclass, return it
    if (!meta_attr->isError()) {
      return *meta_attr;
    }
  }

  // TODO(T25140871): Refactor this into something like:
  //     thread->throwMissingAttributeError(name)
  return thread->raiseAttributeErrorWithCStr("missing attribute");
}

RawObject Runtime::classSetAttr(Thread* thread, const Object& receiver,
                                const Object& name, const Object& value) {
  DCHECK(name->isStr(), "Name is not a string");
  HandleScope scope(thread);
  Type type(&scope, *receiver);
  if (type->isBuiltin()) {
    // TODO(T25140871): Refactor this into something that includes the type name
    // like:
    //     thread->throwImmutableTypeManipulationError(type)
    return thread->raiseTypeErrorWithCStr(
        "can't set attributes of built-in/extension type");
  }

  // Check for a data descriptor
  Type metatype(&scope, typeOf(*receiver));
  Object meta_attr(&scope, lookupNameInMro(thread, metatype, name));
  if (!meta_attr->isError()) {
    if (isDataDescriptor(thread, meta_attr)) {
      // TODO(T25692531): Call __set__ from meta_attr
      UNIMPLEMENTED("custom descriptors are unsupported");
    }
  }

  // No data descriptor found, store the attribute in the type dict
  Dict type_dict(&scope, type->dict());
  dictAtPutInValueCell(type_dict, name, value);

  return NoneType::object();
}

RawObject Runtime::classDelAttr(Thread* thread, const Object& receiver,
                                const Object& name) {
  if (!name->isStr()) {
    // TODO(T25140871): Refactor into something like:
    //     thread->throwUnexpectedTypeError(expected, actual)
    return thread->raiseTypeErrorWithCStr("attribute name must be a string");
  }

  HandleScope scope(thread);
  Type type(&scope, *receiver);
  // TODO(mpage): This needs to handle built-in extension types.
  if (type->isBuiltin()) {
    // TODO(T25140871): Refactor this into something that includes the type name
    // like:
    //     thread->throwImmutableTypeManipulationError(type)
    return thread->raiseTypeErrorWithCStr(
        "can't set attributes of built-in/extension type");
  }

  // Check for a delete descriptor
  Type metatype(&scope, typeOf(*receiver));
  Object meta_attr(&scope, lookupNameInMro(thread, metatype, name));
  if (!meta_attr->isError()) {
    if (isDeleteDescriptor(thread, meta_attr)) {
      return Interpreter::callDescriptorDelete(thread, thread->currentFrame(),
                                               meta_attr, receiver);
    }
  }

  // No delete descriptor found, attempt to delete from the type dict
  Dict type_dict(&scope, type->dict());
  if (dictRemove(type_dict, name)->isError()) {
    // TODO(T25140871): Refactor this into something like:
    //     thread->throwMissingAttributeError(name)
    return thread->raiseAttributeErrorWithCStr("missing attribute");
  }

  return NoneType::object();
}

// Generic attribute lookup code used for instance objects
RawObject Runtime::instanceGetAttr(Thread* thread, const Object& receiver,
                                   const Object& name) {
  DCHECK(name->isStr(), "Name is not a string");
  if (RawStr::cast(*name)->equals(symbols()->DunderClass())) {
    // TODO(T27735822): Make __class__ a descriptor
    return typeOf(*receiver);
  }

  // Look for the attribute in the class
  HandleScope scope(thread);
  Type type(&scope, typeOf(*receiver));
  Object type_attr(&scope, lookupNameInMro(thread, type, name));
  if (!type_attr->isError()) {
    if (isDataDescriptor(thread, type_attr)) {
      Object owner(&scope, *type);
      return Interpreter::callDescriptorGet(thread, thread->currentFrame(),
                                            type_attr, receiver, owner);
    }
  }

  // No data descriptor found on the class, look at the instance.
  if (receiver->isHeapObject()) {
    HeapObject instance(&scope, *receiver);
    RawObject result = thread->runtime()->instanceAt(thread, instance, name);
    if (!result->isError()) {
      return result;
    }
  }

  // Nothing found in the instance, if we found a non-data descriptor via the
  // class search, use it.
  if (!type_attr->isError()) {
    if (isNonDataDescriptor(thread, type_attr)) {
      Object owner(&scope, *type);
      return Interpreter::callDescriptorGet(thread, thread->currentFrame(),
                                            type_attr, receiver, owner);
    }

    // If a regular attribute was found in the class, return it
    return *type_attr;
  }

  // TODO(T25140871): Refactor this into something like:
  //     thread->throwMissingAttributeError(name)
  return thread->raiseAttributeErrorWithCStr("missing attribute");
}

RawObject Runtime::instanceSetAttr(Thread* thread, const Object& receiver,
                                   const Object& name, const Object& value) {
  DCHECK(name->isStr(), "Name is not a string");
  // Check for a data descriptor
  HandleScope scope(thread);
  Type type(&scope, typeOf(*receiver));
  Object type_attr(&scope, lookupNameInMro(thread, type, name));
  if (!type_attr->isError()) {
    if (isDataDescriptor(thread, type_attr)) {
      return Interpreter::callDescriptorSet(thread, thread->currentFrame(),
                                            type_attr, receiver, value);
    }
  }

  // No data descriptor found, store on the instance
  HeapObject instance(&scope, *receiver);
  return thread->runtime()->instanceAtPut(thread, instance, name, value);
}

RawObject Runtime::instanceDelAttr(Thread* thread, const Object& receiver,
                                   const Object& name) {
  if (!name->isStr()) {
    // TODO(T25140871): Refactor into something like:
    //     thread->throwUnexpectedTypeError(expected, actual)
    return thread->raiseTypeErrorWithCStr("attribute name must be a string");
  }

  // Check for a descriptor with __delete__
  HandleScope scope(thread);
  Type type(&scope, typeOf(*receiver));
  Object type_attr(&scope, lookupNameInMro(thread, type, name));
  if (!type_attr->isError()) {
    if (isDeleteDescriptor(thread, type_attr)) {
      return Interpreter::callDescriptorDelete(thread, thread->currentFrame(),
                                               type_attr, receiver);
    }
  }

  // No delete descriptor found, delete from the instance
  HeapObject instance(&scope, *receiver);
  Object result(&scope, instanceDel(thread, instance, name));
  if (result->isError()) {
    // TODO(T25140871): Refactor this into something like:
    //     thread->throwMissingAttributeError(name)
    return thread->raiseAttributeErrorWithCStr("missing attribute");
  }

  return *result;
}

RawObject Runtime::functionGetAttr(Thread* thread, const Object& receiver,
                                   const Object& name) {
  DCHECK(name->isStr(), "Name is not a string");

  // Initialize Dict if non-existent
  HandleScope scope(thread);
  Function func(&scope, *receiver);
  if (func->dict()->isNoneType()) {
    func->setDict(newDict());
  }

  AttributeInfo info;
  Layout layout(&scope, layoutAt(receiver->layoutId()));
  if (layoutFindAttribute(thread, layout, name, &info)) {
    return instanceGetAttr(thread, receiver, name);
  }
  Dict function_dict(&scope, func->dict());
  Object result(&scope, dictAt(function_dict, name));
  if (!result->isError()) {
    return *result;
  }
  return thread->raiseAttributeErrorWithCStr("missing attribute");
}

RawObject Runtime::functionSetAttr(Thread* thread, const Object& receiver,
                                   const Object& name, const Object& value) {
  DCHECK(name->isStr(), "Name is not a string");

  // Initialize Dict if non-existent
  HandleScope scope(thread);
  Function func(&scope, *receiver);
  if (func->dict()->isNoneType()) {
    func->setDict(newDict());
  }

  AttributeInfo info;
  Layout layout(&scope, layoutAt(receiver->layoutId()));
  if (layoutFindAttribute(thread, layout, name, &info)) {
    // TODO(eelizondo): Handle __dict__ with descriptor
    return instanceSetAttr(thread, receiver, name, value);
  }
  Dict function_dict(&scope, func->dict());
  dictAtPut(function_dict, name, value);
  return NoneType::object();
}

// Note that PEP 562 adds support for data descriptors in module objects.
// We are targeting python 3.6 for now, so we won't worry about that.
RawObject Runtime::moduleGetAttr(Thread* thread, const Object& receiver,
                                 const Object& name) {
  DCHECK(name->isStr(), "Name is not a string");
  HandleScope scope(thread);
  Module mod(&scope, *receiver);
  Object ret(&scope, moduleAt(mod, name));

  if (!ret->isError()) {
    return *ret;
  }
  // TODO(T25140871): Refactor this into something like:
  //     thread->throwMissingAttributeError(name)
  return thread->raiseAttributeErrorWithCStr("missing attribute");
}

RawObject Runtime::moduleSetAttr(Thread* thread, const Object& receiver,
                                 const Object& name, const Object& value) {
  DCHECK(name->isStr(), "Name is not a string");
  HandleScope scope(thread);
  Module mod(&scope, *receiver);
  moduleAtPut(mod, name, value);
  return NoneType::object();
}

RawObject Runtime::moduleDelAttr(Thread* thread, const Object& receiver,
                                 const Object& name) {
  if (!name->isStr()) {
    // TODO(T25140871): Refactor into something like:
    //     thread->throwUnexpectedTypeError(expected, actual)
    return thread->raiseTypeErrorWithCStr("attribute name must be a string");
  }

  // Check for a descriptor with __delete__
  HandleScope scope(thread);
  Type type(&scope, typeOf(*receiver));
  Object type_attr(&scope, lookupNameInMro(thread, type, name));
  if (!type_attr->isError()) {
    if (isDeleteDescriptor(thread, type_attr)) {
      return Interpreter::callDescriptorDelete(thread, thread->currentFrame(),
                                               type_attr, receiver);
    }
  }

  // No delete descriptor found, attempt to delete from the module dict
  Module module(&scope, *receiver);
  Dict module_dict(&scope, module->dict());
  if (dictRemove(module_dict, name)->isError()) {
    // TODO(T25140871): Refactor this into something like:
    //     thread->throwMissingAttributeError(name)
    return thread->raiseAttributeErrorWithCStr("missing attribute");
  }

  return NoneType::object();
}

bool Runtime::isDataDescriptor(Thread* thread, const Object& object) {
  // TODO(T25692962): Track "descriptorness" through a bit on the class
  HandleScope scope(thread);
  Type type(&scope, typeOf(*object));
  return !lookupSymbolInMro(thread, type, SymbolId::kDunderSet)->isError();
}

bool Runtime::isNonDataDescriptor(Thread* thread, const Object& object) {
  // TODO(T25692962): Track "descriptorness" through a bit on the class
  HandleScope scope(thread);
  Type type(&scope, typeOf(*object));
  return !lookupSymbolInMro(thread, type, SymbolId::kDunderGet)->isError();
}

bool Runtime::isDeleteDescriptor(Thread* thread, const Object& object) {
  // TODO(T25692962): Track "descriptorness" through a bit on the class
  HandleScope scope(thread);
  Type type(&scope, typeOf(*object));
  return !lookupSymbolInMro(thread, type, SymbolId::kDunderDelete)->isError();
}

RawObject Runtime::newCode() {
  HandleScope scope;
  Code result(&scope, heap()->create<RawCode>());
  result->setArgcount(0);
  result->setKwonlyargcount(0);
  result->setCell2arg(0);
  result->setNlocals(0);
  result->setStacksize(0);
  result->setFlags(0);
  result->setFreevars(empty_object_array_);
  result->setCellvars(empty_object_array_);
  result->setFirstlineno(0);
  return *result;
}

RawObject Runtime::newBuiltinFunction(SymbolId name, Function::Entry entry,
                                      Function::Entry entry_kw,
                                      Function::Entry entry_ex) {
  HandleScope scope;
  Function result(&scope, heap()->create<RawFunction>());
  result->setName(symbols()->at(name));
  result->setEntry(entry);
  result->setEntryKw(entry_kw);
  result->setEntryEx(entry_ex);
  return *result;
}

RawObject Runtime::newFunction() {
  HandleScope scope;
  Function result(&scope, heap()->create<RawFunction>());
  result->setEntry(unimplementedTrampoline);
  result->setEntryKw(unimplementedTrampoline);
  result->setEntryEx(unimplementedTrampoline);
  return *result;
}

RawObject Runtime::newCoroutine() { return heap()->create<RawCoroutine>(); }

RawObject Runtime::newGenerator() { return heap()->create<RawGenerator>(); }

RawObject Runtime::newHeapFrame(const Code& code) {
  DCHECK(code->hasCoroutineOrGenerator(),
         "expected a RawGenerator/RawCoroutine code object");

  word num_args = code->totalArgs();
  word num_vars = code->totalVars();
  word extra_words = num_args + num_vars + code->stacksize();
  HandleScope scope;
  HeapFrame frame(
      &scope, heap()->createInstance(LayoutId::kHeapFrame,
                                     HeapFrame::numAttributes(extra_words)));
  frame->setMaxStackSize(code->stacksize());
  return *frame;
}

RawObject Runtime::newInstance(const Layout& layout) {
  word num_words = layout->instanceSize();
  RawObject object = heap()->createInstance(layout->id(), num_words);
  RawHeapObject instance = RawHeapObject::cast(object);
  // Set the overflow array
  instance->instanceVariableAtPut(layout->overflowOffset(),
                                  empty_object_array_);
  return instance;
}

void Runtime::typeAddBuiltinFunction(const Type& type, SymbolId name,
                                     Function::Entry entry) {
  typeAddBuiltinFunctionKwEx(type, name, entry, unimplementedTrampoline,
                             unimplementedTrampoline);
}

void Runtime::typeAddBuiltinFunctionKw(const Type& type, SymbolId name,
                                       Function::Entry entry,
                                       Function::Entry entry_kw) {
  typeAddBuiltinFunctionKwEx(type, name, entry, entry_kw,
                             unimplementedTrampoline);
}

void Runtime::typeAddBuiltinFunctionKwEx(const Type& type, SymbolId name,
                                         Function::Entry entry,
                                         Function::Entry entry_kw,
                                         Function::Entry entry_ex) {
  HandleScope scope;
  Function function(&scope,
                    newBuiltinFunction(name, entry, entry_kw, entry_ex));
  Object key(&scope, symbols()->at(name));
  Object value(&scope, *function);
  Dict dict(&scope, type->dict());
  dictAtPutInValueCell(dict, key, value);
}

void Runtime::classAddExtensionFunction(const Type& type, SymbolId name,
                                        void* c_function) {
  DCHECK(!type->extensionType()->isNoneType(),
         "RawType must contain extension type");

  HandleScope scope;
  Function function(&scope, newFunction());
  Object key(&scope, symbols()->at(name));
  function->setName(*key);
  function->setCode(newIntFromCPtr(c_function));
  function->setEntry(extensionTrampoline);
  function->setEntryKw(extensionTrampolineKw);
  function->setEntryEx(extensionTrampolineEx);
  Object value(&scope, *function);
  Dict dict(&scope, type->dict());
  dictAtPutInValueCell(dict, key, value);
}

RawObject Runtime::newList() {
  HandleScope scope;
  List result(&scope, heap()->create<RawList>());
  result->setNumItems(0);
  result->setItems(empty_object_array_);
  return *result;
}

RawObject Runtime::newListIterator(const Object& list) {
  HandleScope scope;
  ListIterator list_iterator(&scope, heap()->create<RawListIterator>());
  list_iterator->setIndex(0);
  list_iterator->setList(*list);
  return *list_iterator;
}

RawObject Runtime::newModule(const Object& name) {
  HandleScope scope;
  Module result(&scope, heap()->create<RawModule>());
  Dict dict(&scope, newDict());
  result->setDict(*dict);
  result->setName(*name);
  result->setDef(newIntFromCPtr(nullptr));
  Object key(&scope, symbols()->DunderName());
  moduleAtPut(result, key, name);

  Object none(&scope, NoneType::object());
  Object doc_key(&scope, symbols()->DunderDoc());
  moduleAtPut(result, doc_key, none);
  Object package_key(&scope, symbols()->DunderPackage());
  moduleAtPut(result, package_key, none);
  Object loader_key(&scope, symbols()->DunderLoader());
  moduleAtPut(result, loader_key, none);
  Object spec_key(&scope, symbols()->DunderSpec());
  moduleAtPut(result, spec_key, none);

  return *result;
}

RawObject Runtime::newIntFromCPtr(void* ptr) {
  return newInt(reinterpret_cast<word>(ptr));
}

RawObject Runtime::newTuple(word length) {
  if (length == 0) {
    return empty_object_array_;
  }
  return heap()->createTuple(length, NoneType::object());
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
  return RawFloat::cast(heap()->createFloat(value));
}

RawObject Runtime::newComplex(double real, double imag) {
  return RawComplex::cast(heap()->createComplex(real, imag));
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
    result->digitAtPut(i, digits.get(i));
  }
  DCHECK(result->isValid(), "Invalid digits");
  return *result;
}

RawObject Runtime::newProperty(const Object& getter, const Object& setter,
                               const Object& deleter) {
  HandleScope scope;
  Property new_prop(&scope, heap()->create<RawProperty>());
  new_prop->setGetter(*getter);
  new_prop->setSetter(*setter);
  new_prop->setDeleter(*deleter);
  return *new_prop;
}

RawObject Runtime::newRange(word start, word stop, word step) {
  auto range = RawRange::cast(heap()->createRange());
  range->setStart(start);
  range->setStop(stop);
  range->setStep(step);
  return range;
}

RawObject Runtime::newRangeIterator(const Object& range) {
  HandleScope scope;
  RangeIterator range_iterator(&scope, heap()->create<RawRangeIterator>());
  range_iterator->setRange(*range);
  return *range_iterator;
}

RawObject Runtime::newSetIterator(const Object& set) {
  HandleScope scope;
  SetIterator result(&scope, heap()->create<RawSetIterator>());
  result->setSet(*set);
  result->setIndex(0);
  result->setConsumedCount(0);
  return *result;
}

RawObject Runtime::newSlice(const Object& start, const Object& stop,
                            const Object& step) {
  HandleScope scope;
  Slice slice(&scope, heap()->create<RawSlice>());
  slice->setStart(*start);
  slice->setStop(*stop);
  slice->setStep(*step);
  return *slice;
}

RawObject Runtime::newStaticMethod() {
  return heap()->create<RawStaticMethod>();
}

RawObject Runtime::newStrFromCStr(const char* c_str) {
  word length = std::strlen(c_str);
  auto data = reinterpret_cast<const byte*>(c_str);
  return newStrWithAll(View<byte>(data, length));
}

RawObject Runtime::newStrFromFormat(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  int length = std::vsnprintf(nullptr, 0, fmt, args);
  DCHECK(length >= 0, "RawError occurred doing snprintf");
  va_end(args);
  va_start(args, fmt);
  char* buf = new char[length + 1];
  std::vsprintf(buf, fmt, args);
  va_end(args);
  RawObject result = newStrFromCStr(buf);
  delete[] buf;
  return result;
}

RawObject Runtime::newStrWithAll(View<byte> code_units) {
  word length = code_units.length();
  if (length <= RawSmallStr::kMaxLength) {
    return SmallStr::fromBytes(code_units);
  }
  RawObject result = heap()->createLargeStr(length);
  DCHECK(result != Error::object(), "failed to create large string");
  byte* dst = reinterpret_cast<byte*>(RawLargeStr::cast(result)->address());
  const byte* src = code_units.data();
  memcpy(dst, src, length);
  return result;
}

RawObject Runtime::internStrFromCStr(const char* c_str) {
  HandleScope scope;
  // TODO(T29648342): Optimize lookup to avoid creating an intermediary Str
  Object str(&scope, newStrFromCStr(c_str));
  return internStr(str);
}

RawObject Runtime::internStr(const Object& str) {
  HandleScope scope;
  Set set(&scope, interned());
  DCHECK(str->isStr(), "not a string");
  if (str->isSmallStr()) {
    return *str;
  }
  Object key_hash(&scope, hash(*str));
  return setAddWithHash(set, str, key_hash);
}

RawObject Runtime::hash(RawObject object) {
  if (!object->isHeapObject()) {
    return immediateHash(object);
  }
  if (object->isBytes() || object->isLargeStr()) {
    return valueHash(object);
  }
  return identityHash(object);
}

RawObject Runtime::immediateHash(RawObject object) {
  if (object->isSmallInt()) {
    return object;
  }
  if (object->isBool()) {
    return SmallInt::fromWord(RawBool::cast(object)->value() ? 1 : 0);
  }
  if (object->isSmallStr()) {
    return SmallInt::fromWord(object.raw() >> RawSmallStr::kTagSize);
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

void Runtime::setArgv(int argc, const char** argv) {
  HandleScope scope;
  List list(&scope, newList());
  CHECK(argc >= 1, "Unexpected argc");
  for (int i = 1; i < argc; i++) {  // skip program name (i.e. "python")
    Object arg_val(&scope, newStrFromCStr(argv[i]));
    listAdd(list, arg_val);
  }

  Object module_name(&scope, symbols()->Sys());
  Module sys_module(&scope, findModule(module_name));
  Object argv_value(&scope, *list);
  moduleAddGlobal(sys_module, SymbolId::kArgv, argv_value);
}

RawObject Runtime::identityHash(RawObject object) {
  RawHeapObject src = RawHeapObject::cast(object);
  word code = src->header()->hashCode();
  if (code == 0) {
    code = random() & RawHeader::kHashCodeMask;
    code = (code == 0) ? 1 : code;
    src->setHeader(src->header()->withHashCode(code));
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
  RawHeapObject src = RawHeapObject::cast(object);
  RawHeader header = src->header();
  word code = header->hashCode();
  if (code == 0) {
    word size = src->headerCountOrOverflow();
    code = siphash24(View<byte>(reinterpret_cast<byte*>(src->address()), size));
    code &= RawHeader::kHashCodeMask;
    code = (code == 0) ? 1 : code;
    src->setHeader(header->withHashCode(code));
    DCHECK(code == src->header()->hashCode(), "hash failure");
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
  list->setItems(*array);
  const word allocated = static_cast<word>(LayoutId::kLastBuiltinId) + 1;
  CHECK(allocated < array->length(), "bad allocation %ld", allocated);
  list->setNumItems(allocated);
  layouts_ = *list;
}

RawObject Runtime::createMro(const Layout& subclass_layout,
                             LayoutId superclass_id) {
  HandleScope scope;
  CHECK(subclass_layout->describedType()->isType(),
        "subclass layout must have a described class");
  Type superclass(&scope, typeAt(superclass_id));
  Tuple src(&scope, superclass->mro());
  Tuple dst(&scope, newTuple(1 + src->length()));
  dst->atPut(0, subclass_layout->describedType());
  for (word i = 0; i < src->length(); i++) {
    dst->atPut(1 + i, src->at(i));
  }
  return *dst;
}

void Runtime::initializeHeapTypes() {
  ObjectBuiltins::initialize(this);

  // Abstract classes.
  StrBuiltins::initialize(this);
  IntBuiltins::initialize(this);

  // Exception hierarchy
  initializeExceptionTypes();

  // Concrete classes.

  addEmptyBuiltinType(SymbolId::kBytes, LayoutId::kBytes, LayoutId::kObject);
  initializeClassMethodType();
  addEmptyBuiltinType(SymbolId::kCode, LayoutId::kCode, LayoutId::kObject);
  ComplexBuiltins::initialize(this);
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
  FunctionBuiltins::initialize(this);
  addEmptyBuiltinType(SymbolId::kLayout, LayoutId::kLayout, LayoutId::kObject);
  ListBuiltins::initialize(this);
  ListIteratorBuiltins::initialize(this);
  addEmptyBuiltinType(SymbolId::kMethod, LayoutId::kBoundMethod,
                      LayoutId::kObject);
  addEmptyBuiltinType(SymbolId::kModule, LayoutId::kModule, LayoutId::kObject);
  addEmptyBuiltinType(SymbolId::kNotImplementedType, LayoutId::kNotImplemented,
                      LayoutId::kObject);
  TupleBuiltins::initialize(this);
  TupleIteratorBuiltins::initialize(this);
  initializePropertyType();
  RangeBuiltins::initialize(this);
  RangeIteratorBuiltins::initialize(this);
  initializeRefType();
  SetBuiltins::initialize(this);
  SetIteratorBuiltins::initialize(this);
  StrIteratorBuiltins::initialize(this);
  GeneratorBaseBuiltins::initialize(this);
  addEmptyBuiltinType(SymbolId::kSlice, LayoutId::kSlice, LayoutId::kObject);
  initializeStaticMethodType();
  initializeSuperType();
  addEmptyBuiltinType(SymbolId::kTraceback, LayoutId::kTraceback,
                      LayoutId::kObject);
  initializeTypeType();
  addEmptyBuiltinType(SymbolId::kValueCell, LayoutId::kValueCell,
                      LayoutId::kObject);
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
  addEmptyBuiltinType(SymbolId::kUnicodeError, LayoutId::kUnicodeError,
                      LayoutId::kValueError);

  // UnicodeError subclasses
  addEmptyBuiltinType(SymbolId::kUnicodeEncodeError,
                      LayoutId::kUnicodeEncodeError, LayoutId::kUnicodeError);
  addEmptyBuiltinType(SymbolId::kUnicodeDecodeError,
                      LayoutId::kUnicodeDecodeError, LayoutId::kUnicodeError);
  addEmptyBuiltinType(SymbolId::kUnicodeTranslateError,
                      LayoutId::kUnicodeTranslateError,
                      LayoutId::kUnicodeError);

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

void Runtime::initializeRefType() {
  HandleScope scope;
  Type ref(&scope, addEmptyBuiltinType(SymbolId::kRef, LayoutId::kWeakRef,
                                       LayoutId::kObject));

  typeAddBuiltinFunction(ref, SymbolId::kDunderInit,
                         nativeTrampoline<builtinRefInit>);

  typeAddBuiltinFunction(ref, SymbolId::kDunderNew,
                         nativeTrampoline<builtinRefNew>);
}

void Runtime::initializeClassMethodType() {
  HandleScope scope;
  Type classmethod(
      &scope, addEmptyBuiltinType(SymbolId::kClassmethod,
                                  LayoutId::kClassMethod, LayoutId::kObject));

  typeAddBuiltinFunction(classmethod, SymbolId::kDunderGet,
                         nativeTrampoline<builtinClassMethodGet>);

  typeAddBuiltinFunction(classmethod, SymbolId::kDunderInit,
                         nativeTrampoline<builtinClassMethodInit>);

  typeAddBuiltinFunction(classmethod, SymbolId::kDunderNew,
                         nativeTrampoline<builtinClassMethodNew>);
}

void Runtime::initializeTypeType() {
  HandleScope scope;
  Type type(&scope, addEmptyBuiltinType(SymbolId::kType, LayoutId::kType,
                                        LayoutId::kObject));

  typeAddBuiltinFunctionKw(type, SymbolId::kDunderCall,
                           nativeTrampoline<builtinTypeCall>,
                           nativeTrampolineKw<builtinTypeCallKw>);

  typeAddBuiltinFunction(type, SymbolId::kDunderInit,
                         nativeTrampoline<builtinTypeInit>);

  typeAddBuiltinFunction(type, SymbolId::kDunderNew,
                         nativeTrampoline<builtinTypeNew>);
}

void Runtime::initializeImmediateTypes() {
  BoolBuiltins::initialize(this);
  NoneBuiltins::initialize(this);
  SmallStrBuiltins::initialize(this);
  SmallIntBuiltins::initialize(this);
}

void Runtime::initializePropertyType() {
  HandleScope scope;
  Type property(&scope,
                addEmptyBuiltinType(SymbolId::kProperty, LayoutId::kProperty,
                                    LayoutId::kObject));

  typeAddBuiltinFunction(property, SymbolId::kDeleter,
                         nativeTrampoline<builtinPropertyDeleter>);

  typeAddBuiltinFunction(property, SymbolId::kDunderGet,
                         nativeTrampoline<builtinPropertyDunderGet>);

  typeAddBuiltinFunction(property, SymbolId::kDunderSet,
                         nativeTrampoline<builtinPropertyDunderSet>);

  typeAddBuiltinFunction(property, SymbolId::kDunderInit,
                         nativeTrampoline<builtinPropertyInit>);

  typeAddBuiltinFunction(property, SymbolId::kDunderNew,
                         nativeTrampoline<builtinPropertyNew>);

  typeAddBuiltinFunction(property, SymbolId::kGetter,
                         nativeTrampoline<builtinPropertyGetter>);

  typeAddBuiltinFunction(property, SymbolId::kSetter,
                         nativeTrampoline<builtinPropertySetter>);
}

void Runtime::initializeStaticMethodType() {
  HandleScope scope;
  Type staticmethod(
      &scope, addEmptyBuiltinType(SymbolId::kStaticMethod,
                                  LayoutId::kStaticMethod, LayoutId::kObject));

  typeAddBuiltinFunction(staticmethod, SymbolId::kDunderGet,
                         nativeTrampoline<builtinStaticMethodGet>);

  typeAddBuiltinFunction(staticmethod, SymbolId::kDunderInit,
                         nativeTrampoline<builtinStaticMethodInit>);

  typeAddBuiltinFunction(staticmethod, SymbolId::kDunderNew,
                         nativeTrampoline<builtinStaticMethodNew>);
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
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->currentFrame();
  HandleScope scope(thread);
  while (callbacks_ != NoneType::object()) {
    Object weak(&scope, WeakRef::dequeueReference(&callbacks_));
    Object callback(&scope, RawWeakRef::cast(*weak)->callback());
    Interpreter::callMethod1(thread, frame, callback, weak);
    thread->ignorePendingException();
    RawWeakRef::cast(*weak)->setCallback(NoneType::object());
  }
}

RawObject Runtime::run(const char* buffer) {
  HandleScope scope;

  Object name(&scope, symbols()->DunderMain());
  Object main(&scope, findModule(name));
  if (main->isNoneType()) {
    main = createMainModule();
  }
  Module main_module(&scope, *main);
  return executeModule(buffer, main_module);
}

RawObject Runtime::runFromCStr(const char* c_str) {
  const char* buffer = compile(c_str);
  RawObject result = run(buffer);
  delete[] buffer;
  return result;
}

RawObject Runtime::executeModule(const char* buffer, const Module& module) {
  HandleScope scope;
  Marshal::Reader reader(&scope, this, buffer);

  reader.readLong();
  reader.readLong();
  reader.readLong();

  Code code(&scope, reader.readObject());
  DCHECK(code->argcount() == 0, "invalid argcount %ld", code->argcount());

  return Thread::currentThread()->runModuleFunction(module, code);
}

extern "C" struct _inittab _PyImport_Inittab[];

RawObject Runtime::importModule(const Object& name) {
  HandleScope scope;
  Object cached_module(&scope, findModule(name));
  if (!cached_module->isNoneType()) {
    return *cached_module;
  }
  for (int i = 0; _PyImport_Inittab[i].name != nullptr; i++) {
    if (RawStr::cast(*name)->equalsCStr(_PyImport_Inittab[i].name)) {
      PyObject* module = (*_PyImport_Inittab[i].initfunc)();
      Module mod(&scope, ApiHandle::fromPyObject(module)->asObject());
      addModule(mod);
      return *mod;
    }
  }
  return Thread::currentThread()->raiseRuntimeErrorWithCStr(
      "importModule is unimplemented!");
}

// TODO(cshapiro): support fromlist and level. Ideally, we'll never implement
// that functionality in c++, instead using the pure-python importlib
// implementation that ships with cpython.
RawObject Runtime::importModuleFromBuffer(const char* buffer,
                                          const Object& name) {
  HandleScope scope;
  Object cached_module(&scope, findModule(name));
  if (!cached_module->isNoneType()) {
    return *cached_module;
  }

  Module module(&scope, newModule(name));
  addModule(module);
  executeModule(buffer, module);
  return *module;
}

void Runtime::initializeThreads() {
  auto main_thread = new Thread(Thread::kDefaultStackSize);
  threads_ = main_thread;
  main_thread->setRuntime(this);
  Thread::setCurrentThread(main_thread);
}

void Runtime::initializePrimitiveInstances() {
  empty_object_array_ = heap()->createTuple(0, NoneType::object());
  empty_byte_array_ = heap()->createBytes(0);
  ellipsis_ = heap()->createEllipsis();
  not_implemented_ = heap()->create<RawNotImplemented>();
  callbacks_ = NoneType::object();
}

void Runtime::initializeInterned() { interned_ = newSet(); }

void Runtime::initializeRandom() {
  uword random_state[2];
  uword hash_secret[2];
  OS::secureRandom(reinterpret_cast<byte*>(&random_state),
                   sizeof(random_state));
  OS::secureRandom(reinterpret_cast<byte*>(&hash_secret), sizeof(hash_secret));
  seedRandom(random_state, hash_secret);
}

void Runtime::initializeSymbols() {
  HandleScope scope;
  symbols_ = new Symbols(this);
  for (int i = 0; i < static_cast<int>(SymbolId::kMaxId); i++) {
    SymbolId id = static_cast<SymbolId>(i);
    Object symbol(&scope, symbols()->at(id));
    internStr(symbol);
  }
}

void Runtime::visitRoots(PointerVisitor* visitor) {
  visitRuntimeRoots(visitor);
  visitThreadRoots(visitor);
}

void Runtime::visitRuntimeRoots(PointerVisitor* visitor) {
  // Visit layouts
  visitor->visitPointer(&layouts_);

  // Visit instances
  visitor->visitPointer(&empty_byte_array_);
  visitor->visitPointer(&empty_object_array_);
  visitor->visitPointer(&ellipsis_);
  visitor->visitPointer(&not_implemented_);
  visitor->visitPointer(&build_class_);
  visitor->visitPointer(&display_hook_);

  // Visit interned strings.
  visitor->visitPointer(&interned_);

  // Visit modules
  visitor->visitPointer(&modules_);

  // Visit C-API data.
  visitor->visitPointer(&api_handles_);
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
  HandleScope scope;
  Dict dict(&scope, modules());
  Object key(&scope, module->name());
  Object value(&scope, *module);
  dictAtPut(dict, key, value);
}

RawObject Runtime::findModule(const Object& name) {
  DCHECK(name->isStr(), "name not a string");

  HandleScope scope;
  Dict dict(&scope, modules());
  RawObject value = dictAt(dict, name);
  if (value->isError()) {
    return NoneType::object();
  }
  return value;
}

RawObject Runtime::moduleAt(const Module& module, const Object& key) {
  HandleScope scope;
  Dict dict(&scope, module->dict());
  Object value_cell(&scope, dictAt(dict, key));
  if (value_cell->isError()) {
    return Error::object();
  }
  return RawValueCell::cast(*value_cell)->value();
}

void Runtime::moduleAtPut(const Module& module, const Object& key,
                          const Object& value) {
  HandleScope scope;
  Dict dict(&scope, module->dict());
  dictAtPutInValueCell(dict, key, value);
}

struct {
  SymbolId name;
  void (Runtime::*create_module)();
} kBuiltinModules[] = {
    {SymbolId::kBuiltins, &Runtime::createBuiltinsModule},
    {SymbolId::kSys, &Runtime::createSysModule},
    {SymbolId::kTime, &Runtime::createTimeModule},
    {SymbolId::kUnderImp, &Runtime::createImportModule},
    {SymbolId::kUnderIo, &Runtime::createUnderIoModule},
    {SymbolId::kUnderThread, &Runtime::createThreadModule},
    {SymbolId::kUnderWarnings, &Runtime::createWarningsModule},
    {SymbolId::kUnderWeakRef, &Runtime::createWeakRefModule},
};

void Runtime::initializeModules() {
  modules_ = newDict();
  for (uword i = 0; i < ARRAYSIZE(kBuiltinModules); i++) {
    (this->*kBuiltinModules[i].create_module)();
  }
}

void Runtime::initializeApiData() {
  api_handles_ = newDict();
  api_caches_ = newDict();
}

RawObject Runtime::typeOf(RawObject object) {
  HandleScope scope;
  Layout layout(&scope, layoutAt(object->layoutId()));
  return layout->describedType();
}

RawObject Runtime::layoutAt(LayoutId layout_id) {
  return RawList::cast(layouts_)->at(static_cast<word>(layout_id));
}

void Runtime::layoutAtPut(LayoutId layout_id, RawObject object) {
  RawList::cast(layouts_)->atPut(static_cast<word>(layout_id), object);
}

RawObject Runtime::typeAt(LayoutId layout_id) {
  return RawLayout::cast(layoutAt(layout_id))->describedType();
}

LayoutId Runtime::reserveLayoutId() {
  HandleScope scope;
  List list(&scope, layouts_);
  Object value(&scope, NoneType::object());
  word result = list->numItems();
  DCHECK(result <= RawHeader::kMaxLayoutId,
         "exceeded layout id space in header word");
  listAdd(list, value);
  return static_cast<LayoutId>(result);
}

SymbolId Runtime::binaryOperationSelector(Interpreter::BinaryOp op) {
  return kBinaryOperationSelector[static_cast<int>(op)];
}

SymbolId Runtime::swappedBinaryOperationSelector(Interpreter::BinaryOp op) {
  return kSwappedBinaryOperationSelector[static_cast<int>(op)];
}

bool Runtime::shouldReverseBinaryOperation(
    Thread* thread, const Object& left, const Object& left_reversed_method,
    const Object& right, const Object& right_reversed_method) {
  HandleScope scope(thread);
  Type left_type(&scope, typeOf(*left));
  Type right_type(&scope, typeOf(*right));
  bool is_derived_type =
      (*left_type != *right_type) && isSubclass(right_type, left_type);
  bool is_method_different = !right_reversed_method->isError() &&
                             *left_reversed_method != *right_reversed_method;
  return is_derived_type && is_method_different;
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

bool Runtime::isMethodOverloaded(Thread* thread, const Type& type,
                                 SymbolId selector) {
  HandleScope scope(thread);
  Tuple mro(&scope, type->mro());
  Object key(&scope, symbols()->at(selector));
  DCHECK(mro->length() > 0, "empty MRO");
  for (word i = 0; i < mro->length() - 1; i++) {
    Type mro_type(&scope, mro->at(i));
    Dict dict(&scope, mro_type->dict());
    Object value_cell(&scope, dictAt(dict, key));
    if (!value_cell->isError()) {
      return true;
    }
  }
  return false;
}

RawObject Runtime::moduleAddGlobal(const Module& module, SymbolId name,
                                   const Object& value) {
  HandleScope scope;
  Dict dict(&scope, module->dict());
  Object key(&scope, symbols()->at(name));
  return dictAtPutInValueCell(dict, key, value);
}

RawObject Runtime::moduleAddBuiltinFunction(const Module& module, SymbolId name,
                                            const Function::Entry entry,
                                            const Function::Entry entry_kw,
                                            const Function::Entry entry_ex) {
  HandleScope scope;
  Object key(&scope, symbols()->at(name));
  Dict dict(&scope, module->dict());
  Object value(&scope, newBuiltinFunction(name, entry, entry_kw, entry_ex));
  return dictAtPutInValueCell(dict, key, value);
}

void Runtime::createBuiltinsModule() {
  HandleScope scope;
  Object name(&scope, newStrFromCStr("builtins"));
  Module module(&scope, newModule(name));

  // Fill in builtins...
  build_class_ = moduleAddBuiltinFunction(
      module, SymbolId::kDunderBuildClass, nativeTrampoline<builtinBuildClass>,
      nativeTrampolineKw<builtinBuildClassKw>, unimplementedTrampoline);

  moduleAddBuiltinFunction(module, SymbolId::kCallable,
                           nativeTrampoline<builtinCallable>,
                           unimplementedTrampoline, unimplementedTrampoline);
  moduleAddBuiltinFunction(module, SymbolId::kChr, nativeTrampoline<builtinChr>,
                           unimplementedTrampoline, unimplementedTrampoline);
  moduleAddBuiltinFunction(module, SymbolId::kGetattr,
                           nativeTrampoline<builtinGetattr>,
                           unimplementedTrampoline, unimplementedTrampoline);
  moduleAddBuiltinFunction(module, SymbolId::kHasattr,
                           nativeTrampoline<builtinHasattr>,
                           unimplementedTrampoline, unimplementedTrampoline);
  moduleAddBuiltinFunction(module, SymbolId::kIsInstance,
                           nativeTrampoline<builtinIsinstance>,
                           unimplementedTrampoline, unimplementedTrampoline);
  moduleAddBuiltinFunction(module, SymbolId::kIsSubclass,
                           nativeTrampoline<builtinIssubclass>,
                           unimplementedTrampoline, unimplementedTrampoline);
  moduleAddBuiltinFunction(module, SymbolId::kLen, nativeTrampoline<builtinLen>,
                           unimplementedTrampoline, unimplementedTrampoline);
  moduleAddBuiltinFunction(module, SymbolId::kOrd, nativeTrampoline<builtinOrd>,
                           unimplementedTrampoline, unimplementedTrampoline);
  moduleAddBuiltinFunction(
      module, SymbolId::kPrint, nativeTrampoline<builtinPrint>,
      nativeTrampolineKw<builtinPrintKw>, unimplementedTrampoline);
  moduleAddBuiltinFunction(module, SymbolId::kRange,
                           nativeTrampoline<builtinRange>,
                           unimplementedTrampoline, unimplementedTrampoline);
  moduleAddBuiltinFunction(module, SymbolId::kRepr,
                           nativeTrampoline<builtinRepr>,
                           unimplementedTrampoline, unimplementedTrampoline);
  moduleAddBuiltinFunction(module, SymbolId::kSetattr,
                           nativeTrampoline<builtinSetattr>,
                           unimplementedTrampoline, unimplementedTrampoline);
  // Add builtin types
  moduleAddBuiltinType(module, SymbolId::kArithmeticError,
                       LayoutId::kArithmeticError);
  moduleAddBuiltinType(module, SymbolId::kAssertionError,
                       LayoutId::kAssertionError);
  moduleAddBuiltinType(module, SymbolId::kAttributeError,
                       LayoutId::kAttributeError);
  moduleAddBuiltinType(module, SymbolId::kBaseException,
                       LayoutId::kBaseException);
  moduleAddBuiltinType(module, SymbolId::kBlockingIOError,
                       LayoutId::kBlockingIOError);
  moduleAddBuiltinType(module, SymbolId::kBool, LayoutId::kBool);
  moduleAddBuiltinType(module, SymbolId::kBrokenPipeError,
                       LayoutId::kBrokenPipeError);
  moduleAddBuiltinType(module, SymbolId::kBufferError, LayoutId::kBufferError);
  moduleAddBuiltinType(module, SymbolId::kBytesWarning,
                       LayoutId::kBytesWarning);
  moduleAddBuiltinType(module, SymbolId::kChildProcessError,
                       LayoutId::kChildProcessError);
  moduleAddBuiltinType(module, SymbolId::kClassmethod, LayoutId::kClassMethod);
  moduleAddBuiltinType(module, SymbolId::kComplex, LayoutId::kComplex);
  moduleAddBuiltinType(module, SymbolId::kConnectionAbortedError,
                       LayoutId::kConnectionAbortedError);
  moduleAddBuiltinType(module, SymbolId::kConnectionError,
                       LayoutId::kConnectionError);
  moduleAddBuiltinType(module, SymbolId::kConnectionRefusedError,
                       LayoutId::kConnectionRefusedError);
  moduleAddBuiltinType(module, SymbolId::kConnectionResetError,
                       LayoutId::kConnectionResetError);
  moduleAddBuiltinType(module, SymbolId::kDeprecationWarning,
                       LayoutId::kDeprecationWarning);
  moduleAddBuiltinType(module, SymbolId::kDict, LayoutId::kDict);
  moduleAddBuiltinType(module, SymbolId::kEOFError, LayoutId::kEOFError);
  moduleAddBuiltinType(module, SymbolId::kException, LayoutId::kException);
  moduleAddBuiltinType(module, SymbolId::kFileExistsError,
                       LayoutId::kFileExistsError);
  moduleAddBuiltinType(module, SymbolId::kFileNotFoundError,
                       LayoutId::kFileNotFoundError);
  moduleAddBuiltinType(module, SymbolId::kFloat, LayoutId::kFloat);
  moduleAddBuiltinType(module, SymbolId::kFloatingPointError,
                       LayoutId::kFloatingPointError);
  moduleAddBuiltinType(module, SymbolId::kFutureWarning,
                       LayoutId::kFutureWarning);
  moduleAddBuiltinType(module, SymbolId::kGeneratorExit,
                       LayoutId::kGeneratorExit);
  moduleAddBuiltinType(module, SymbolId::kImportError, LayoutId::kImportError);
  moduleAddBuiltinType(module, SymbolId::kImportWarning,
                       LayoutId::kImportWarning);
  moduleAddBuiltinType(module, SymbolId::kIndentationError,
                       LayoutId::kIndentationError);
  moduleAddBuiltinType(module, SymbolId::kIndexError, LayoutId::kIndexError);
  moduleAddBuiltinType(module, SymbolId::kInt, LayoutId::kInt);
  moduleAddBuiltinType(module, SymbolId::kInterruptedError,
                       LayoutId::kInterruptedError);
  moduleAddBuiltinType(module, SymbolId::kIsADirectoryError,
                       LayoutId::kIsADirectoryError);
  moduleAddBuiltinType(module, SymbolId::kKeyError, LayoutId::kKeyError);
  moduleAddBuiltinType(module, SymbolId::kKeyboardInterrupt,
                       LayoutId::kKeyboardInterrupt);
  moduleAddBuiltinType(module, SymbolId::kList, LayoutId::kList);
  moduleAddBuiltinType(module, SymbolId::kLookupError, LayoutId::kLookupError);
  moduleAddBuiltinType(module, SymbolId::kMemoryError, LayoutId::kMemoryError);
  moduleAddBuiltinType(module, SymbolId::kModuleNotFoundError,
                       LayoutId::kModuleNotFoundError);
  moduleAddBuiltinType(module, SymbolId::kNameError, LayoutId::kNameError);
  moduleAddBuiltinType(module, SymbolId::kNotADirectoryError,
                       LayoutId::kNotADirectoryError);
  moduleAddBuiltinType(module, SymbolId::kNotImplementedError,
                       LayoutId::kNotImplementedError);
  moduleAddBuiltinType(module, SymbolId::kOSError, LayoutId::kOSError);
  moduleAddBuiltinType(module, SymbolId::kObjectTypename, LayoutId::kObject);
  moduleAddBuiltinType(module, SymbolId::kOverflowError,
                       LayoutId::kOverflowError);
  moduleAddBuiltinType(module, SymbolId::kPendingDeprecationWarning,
                       LayoutId::kPendingDeprecationWarning);
  moduleAddBuiltinType(module, SymbolId::kPermissionError,
                       LayoutId::kPermissionError);
  moduleAddBuiltinType(module, SymbolId::kProcessLookupError,
                       LayoutId::kProcessLookupError);
  moduleAddBuiltinType(module, SymbolId::kProperty, LayoutId::kProperty);
  moduleAddBuiltinType(module, SymbolId::kRecursionError,
                       LayoutId::kRecursionError);
  moduleAddBuiltinType(module, SymbolId::kReferenceError,
                       LayoutId::kReferenceError);
  moduleAddBuiltinType(module, SymbolId::kResourceWarning,
                       LayoutId::kResourceWarning);
  moduleAddBuiltinType(module, SymbolId::kRuntimeError,
                       LayoutId::kRuntimeError);
  moduleAddBuiltinType(module, SymbolId::kRuntimeWarning,
                       LayoutId::kRuntimeWarning);
  moduleAddBuiltinType(module, SymbolId::kSet, LayoutId::kSet);
  moduleAddBuiltinType(module, SymbolId::kStaticMethod,
                       LayoutId::kStaticMethod);
  moduleAddBuiltinType(module, SymbolId::kStopAsyncIteration,
                       LayoutId::kStopAsyncIteration);
  moduleAddBuiltinType(module, SymbolId::kStopIteration,
                       LayoutId::kStopIteration);
  moduleAddBuiltinType(module, SymbolId::kStr, LayoutId::kStr);
  moduleAddBuiltinType(module, SymbolId::kSuper, LayoutId::kSuper);
  moduleAddBuiltinType(module, SymbolId::kSyntaxError, LayoutId::kSyntaxError);
  moduleAddBuiltinType(module, SymbolId::kSyntaxWarning,
                       LayoutId::kSyntaxWarning);
  moduleAddBuiltinType(module, SymbolId::kSystemError, LayoutId::kSystemError);
  moduleAddBuiltinType(module, SymbolId::kSystemExit, LayoutId::kSystemExit);
  moduleAddBuiltinType(module, SymbolId::kTabError, LayoutId::kTabError);
  moduleAddBuiltinType(module, SymbolId::kTimeoutError,
                       LayoutId::kTimeoutError);
  moduleAddBuiltinType(module, SymbolId::kTuple, LayoutId::kTuple);
  moduleAddBuiltinType(module, SymbolId::kType, LayoutId::kType);
  moduleAddBuiltinType(module, SymbolId::kTypeError, LayoutId::kTypeError);
  moduleAddBuiltinType(module, SymbolId::kUnboundLocalError,
                       LayoutId::kUnboundLocalError);
  moduleAddBuiltinType(module, SymbolId::kUnicodeDecodeError,
                       LayoutId::kUnicodeDecodeError);
  moduleAddBuiltinType(module, SymbolId::kUnicodeEncodeError,
                       LayoutId::kUnicodeEncodeError);
  moduleAddBuiltinType(module, SymbolId::kUnicodeError,
                       LayoutId::kUnicodeError);
  moduleAddBuiltinType(module, SymbolId::kUnicodeTranslateError,
                       LayoutId::kUnicodeTranslateError);
  moduleAddBuiltinType(module, SymbolId::kUnicodeWarning,
                       LayoutId::kUnicodeWarning);
  moduleAddBuiltinType(module, SymbolId::kUserWarning, LayoutId::kUserWarning);
  moduleAddBuiltinType(module, SymbolId::kValueError, LayoutId::kValueError);
  moduleAddBuiltinType(module, SymbolId::kWarning, LayoutId::kWarning);
  moduleAddBuiltinType(module, SymbolId::kZeroDivisionError,
                       LayoutId::kZeroDivisionError);

  Object not_implemented(&scope, notImplemented());
  moduleAddGlobal(module, SymbolId::kNotImplemented, not_implemented);

  addModule(module);
  executeModule(kBuiltinsModuleData, module);
}

void Runtime::moduleAddBuiltinType(const Module& module, SymbolId name,
                                   LayoutId layout_id) {
  HandleScope scope;
  Object value(&scope, typeAt(layout_id));
  moduleAddGlobal(module, name, value);
}

void Runtime::moduleImportAllFrom(const Dict& dict, const Module& module) {
  HandleScope scope;
  Dict module_dict(&scope, module->dict());
  Tuple module_keys(&scope, dictKeys(module_dict));
  for (word i = 0; i < module_keys->length(); i++) {
    Object symbol_name(&scope, module_keys->at(i));
    CHECK(symbol_name->isStr(), "Symbol is not a String");

    // Load all the symbols not starting with '_'
    Str symbol_name_str(&scope, *symbol_name);
    if (symbol_name_str->charAt(0) != '_') {
      Object value(&scope, moduleAt(module, symbol_name));
      dictAtPutInValueCell(dict, symbol_name, value);
    }
  }
}

void Runtime::createSysModule() {
  HandleScope scope;
  Object name(&scope, symbols()->Sys());
  Module module(&scope, newModule(name));

  Object modules(&scope, modules_);
  moduleAddGlobal(module, SymbolId::kModules, modules);

  display_hook_ = moduleAddBuiltinFunction(
      module, SymbolId::kDisplayhook, nativeTrampoline<builtinSysDisplayhook>,
      unimplementedTrampoline, unimplementedTrampoline);

  // Fill in sys...
  moduleAddBuiltinFunction(module, SymbolId::kExit,
                           nativeTrampoline<builtinSysExit>,
                           unimplementedTrampoline, unimplementedTrampoline);

  Object stdout_val(&scope, SmallInt::fromWord(STDOUT_FILENO));
  moduleAddGlobal(module, SymbolId::kStdout, stdout_val);

  Object stderr_val(&scope, SmallInt::fromWord(STDERR_FILENO));
  moduleAddGlobal(module, SymbolId::kStderr, stderr_val);

  Object meta_path(&scope, newList());
  moduleAddGlobal(module, SymbolId::kMetaPath, meta_path);

  Object platform(&scope, newStrFromCStr(OS::name()));
  moduleAddGlobal(module, SymbolId::kPlatform, platform);

  // Count the number of modules and create a tuple
  word num_external_modules = 0;
  while (_PyImport_Inittab[num_external_modules].name != nullptr) {
    num_external_modules++;
  }
  word num_modules = ARRAYSIZE(kBuiltinModules) + num_external_modules;
  Tuple builtins_tuple(&scope, newTuple(num_modules));

  // Add all the available builtin modules
  for (uword i = 0; i < ARRAYSIZE(kBuiltinModules); i++) {
    Object module_name(&scope, symbols()->at(kBuiltinModules[i].name));
    builtins_tuple->atPut(i, *module_name);
  }

  // Add all the available extension builtin modules
  for (int i = 0; _PyImport_Inittab[i].name != nullptr; i++) {
    Object module_name(&scope, newStrFromCStr(_PyImport_Inittab[i].name));
    builtins_tuple->atPut(ARRAYSIZE(kBuiltinModules) + i, *module_name);
  }

  // Create builtin_module_names tuple
  Object builtins(&scope, *builtins_tuple);
  moduleAddGlobal(module, SymbolId::kBuiltinModuleNames, builtins);

  addModule(module);
}

void Runtime::createImportModule() {
  HandleScope scope;
  Object name(&scope, symbols()->UnderImp());
  Module module(&scope, newModule(name));

  // _imp.acquire_lock
  moduleAddBuiltinFunction(module, SymbolId::kAcquireLock,
                           nativeTrampoline<builtinImpAcquireLock>,
                           unimplementedTrampoline, unimplementedTrampoline);

  // _imp.create_builtin
  moduleAddBuiltinFunction(module, SymbolId::kCreateBuiltin,
                           nativeTrampoline<builtinImpCreateBuiltin>,
                           unimplementedTrampoline, unimplementedTrampoline);

  // _imp.exec_builtin
  moduleAddBuiltinFunction(module, SymbolId::kExecBuiltin,
                           nativeTrampoline<builtinImpExecBuiltin>,
                           unimplementedTrampoline, unimplementedTrampoline);

  // _imp.exec_dynamic
  moduleAddBuiltinFunction(module, SymbolId::kExecDynamic,
                           nativeTrampoline<builtinImpExecDynamic>,
                           unimplementedTrampoline, unimplementedTrampoline);

  // _imp.extension_suffixes
  moduleAddBuiltinFunction(module, SymbolId::kExtensionSuffixes,
                           nativeTrampoline<builtinImpExtensionSuffixes>,
                           unimplementedTrampoline, unimplementedTrampoline);

  // _imp.fix_co_filename
  moduleAddBuiltinFunction(module, SymbolId::kFixCoFilename,
                           nativeTrampoline<builtinImpFixCoFilename>,
                           unimplementedTrampoline, unimplementedTrampoline);

  // _imp.get_frozen_object
  moduleAddBuiltinFunction(module, SymbolId::kGetFrozenObject,
                           nativeTrampoline<builtinImpGetFrozenObject>,
                           unimplementedTrampoline, unimplementedTrampoline);

  // _imp.is_builtin
  moduleAddBuiltinFunction(module, SymbolId::kIsBuiltin,
                           nativeTrampoline<builtinImpIsBuiltin>,
                           unimplementedTrampoline, unimplementedTrampoline);

  // _imp.is_frozen
  moduleAddBuiltinFunction(module, SymbolId::kIsFrozen,
                           nativeTrampoline<builtinImpIsFrozen>,
                           unimplementedTrampoline, unimplementedTrampoline);

  // _imp.is_frozen_package
  moduleAddBuiltinFunction(module, SymbolId::kIsFrozenPackage,
                           nativeTrampoline<builtinImpIsFrozenPackage>,
                           unimplementedTrampoline, unimplementedTrampoline);

  // _imp.release_lock
  moduleAddBuiltinFunction(module, SymbolId::kReleaseLock,
                           nativeTrampoline<builtinImpReleaseLock>,
                           unimplementedTrampoline, unimplementedTrampoline);
  addModule(module);
}

void Runtime::createWarningsModule() {
  HandleScope scope;
  Object name(&scope, symbols()->UnderWarnings());
  Module module(&scope, newModule(name));
  addModule(module);
}

void Runtime::createWeakRefModule() {
  HandleScope scope;
  Object name(&scope, symbols()->UnderWeakRef());
  Module module(&scope, newModule(name));

  moduleAddBuiltinType(module, SymbolId::kRef, LayoutId::kWeakRef);
  addModule(module);
}

void Runtime::createThreadModule() {
  HandleScope scope;
  Object name(&scope, symbols()->UnderThread());
  Module module(&scope, newModule(name));
  addModule(module);
}

void Runtime::createTimeModule() {
  HandleScope scope;
  Object name(&scope, symbols()->Time());
  Module module(&scope, newModule(name));

  // time.time
  moduleAddBuiltinFunction(module, SymbolId::kTime,
                           nativeTrampoline<builtinTime>,
                           unimplementedTrampoline, unimplementedTrampoline);

  addModule(module);
}

void Runtime::createUnderIoModule() {
  HandleScope scope;
  Object name(&scope, symbols()->UnderIo());
  Module module(&scope, newModule(name));

  // TODO(eelizondo): Remove once _io is fully imported
  moduleAddBuiltinFunction(module, SymbolId::kUnderReadFile,
                           nativeTrampoline<ioReadFile>,
                           unimplementedTrampoline, unimplementedTrampoline);
  moduleAddBuiltinFunction(module, SymbolId::kUnderReadBytes,
                           nativeTrampoline<ioReadBytes>,
                           unimplementedTrampoline, unimplementedTrampoline);

  addModule(module);
}

RawObject Runtime::createMainModule() {
  HandleScope scope;
  Object name(&scope, symbols()->DunderMain());
  Module module(&scope, newModule(name));

  // Fill in __main__...

  addModule(module);

  return *module;
}

// List

void Runtime::listEnsureCapacity(const List& list, word index) {
  if (index < list->capacity()) {
    return;
  }
  HandleScope scope;
  word new_capacity = (list->capacity() < kInitialEnsuredCapacity)
                          ? kInitialEnsuredCapacity
                          : list->capacity() << 1;
  if (new_capacity < index) {
    new_capacity = Utils::nextPowerOfTwo(index);
  }
  Tuple old_array(&scope, list->items());
  Tuple new_array(&scope, newTuple(new_capacity));
  old_array->copyTo(*new_array);
  list->setItems(*new_array);
}

void Runtime::listAdd(const List& list, const Object& value) {
  HandleScope scope;
  word index = list->numItems();
  listEnsureCapacity(list, index);
  list->setNumItems(index + 1);
  list->atPut(index, *value);
}

RawObject Runtime::listExtend(Thread* thread, const List& dst,
                              const Object& iterable) {
  HandleScope scope(thread);
  Object elt(&scope, NoneType::object());
  word index = dst->numItems();
  // Special case for lists
  if (iterable->isList()) {
    List src(&scope, *iterable);
    if (src->numItems() > 0) {
      word new_capacity = index + src->numItems();
      listEnsureCapacity(dst, new_capacity);
      dst->setNumItems(new_capacity);
      for (word i = 0; i < src->numItems(); i++) {
        dst->atPut(index++, src->at(i));
      }
    }
    return *dst;
  }
  // Special case for list iterators
  if (iterable->isListIterator()) {
    ListIterator list_iter(&scope, *iterable);
    List src(&scope, list_iter->list());
    word new_capacity = index + src->numItems();
    listEnsureCapacity(dst, new_capacity);
    dst->setNumItems(new_capacity);
    for (word i = 0; i < src->numItems(); i++) {
      elt = list_iter->next();
      if (elt->isError()) {
        break;
      }
      dst->atPut(index++, src->at(i));
    }
    return *dst;
  }
  // Special case for tuples
  if (iterable->isTuple()) {
    Tuple tuple(&scope, *iterable);
    if (tuple->length() > 0) {
      word new_capacity = index + tuple->length();
      listEnsureCapacity(dst, new_capacity);
      dst->setNumItems(new_capacity);
      for (word i = 0; i < tuple->length(); i++) {
        dst->atPut(index++, tuple->at(i));
      }
    }
    return *dst;
  }
  // Special case for sets
  if (iterable->isSet()) {
    Set set(&scope, *iterable);
    if (set->numItems() > 0) {
      Tuple data(&scope, set->data());
      word new_capacity = index + set->numItems();
      listEnsureCapacity(dst, new_capacity);
      dst->setNumItems(new_capacity);
      for (word i = 0; i < data->length(); i += Set::Bucket::kNumPointers) {
        if (!Set::Bucket::isFilled(data, i)) {
          continue;
        }
        dst->atPut(index++, Set::Bucket::key(*data, i));
      }
    }
    return *dst;
  }
  // Special case for dicts
  if (iterable->isDict()) {
    Dict dict(&scope, *iterable);
    if (dict->numItems() > 0) {
      Tuple keys(&scope, dictKeys(dict));
      word new_capacity = index + dict->numItems();
      listEnsureCapacity(dst, new_capacity);
      dst->setNumItems(new_capacity);
      for (word i = 0; i < keys->length(); i++) {
        dst->atPut(index++, keys->at(i));
      }
    }
    return *dst;
  }
  // Generic case
  Object iter_method(
      &scope, Interpreter::lookupMethod(thread, thread->currentFrame(),
                                        iterable, SymbolId::kDunderIter));
  if (iter_method->isError()) {
    return thread->raiseTypeErrorWithCStr("object is not iterable");
  }
  Object iterator(&scope,
                  Interpreter::callMethod1(thread, thread->currentFrame(),
                                           iter_method, iterable));
  if (iterator->isError()) {
    return thread->raiseTypeErrorWithCStr("object is not iterable");
  }
  Object next_method(
      &scope, Interpreter::lookupMethod(thread, thread->currentFrame(),
                                        iterator, SymbolId::kDunderNext));
  if (next_method->isError()) {
    return thread->raiseTypeErrorWithCStr("iter() returned a non-iterator");
  }
  Object value(&scope, NoneType::object());
  while (!isIteratorExhausted(thread, iterator)) {
    value = Interpreter::callMethod1(thread, thread->currentFrame(),
                                     next_method, iterator);
    if (value->isError()) {
      return *value;
    }
    listAdd(dst, value);
  }
  return *dst;
}

void Runtime::listInsert(const List& list, const Object& value, word index) {
  listAdd(list, value);
  word last_index = list->numItems() - 1;
  if (index < 0) {
    index = last_index + index;
  }
  index =
      Utils::maximum(static_cast<word>(0), Utils::minimum(last_index, index));
  for (word i = last_index; i > index; i--) {
    list->atPut(i, list->at(i - 1));
  }
  list->atPut(index, *value);
}

RawObject Runtime::listPop(const List& list, word index) {
  HandleScope scope;
  Object popped(&scope, list->at(index));
  list->atPut(index, NoneType::object());
  word last_index = list->numItems() - 1;
  for (word i = index; i < last_index; i++) {
    list->atPut(i, list->at(i + 1));
  }
  list->setNumItems(list->numItems() - 1);
  return *popped;
}

RawObject Runtime::listReplicate(Thread* thread, const List& list,
                                 word ntimes) {
  HandleScope scope(thread);
  word len = list->numItems();
  Tuple items(&scope, newTuple(ntimes * len));
  for (word i = 0; i < ntimes; i++) {
    for (word j = 0; j < len; j++) {
      items->atPut((i * len) + j, list->at(j));
    }
  }
  List result(&scope, newList());
  result->setItems(*items);
  result->setNumItems(items->length());
  return *result;
}

char* Runtime::compile(const char* src) {
  // increment this if you change the caching code, to invalidate existing
  // cache entries.
  uint64_t seed[2] = {0, 1};
  word hash = 0;

  // Hash the input.
  ::siphash(reinterpret_cast<const uint8_t*>(src), strlen(src),
            reinterpret_cast<const uint8_t*>(seed),
            reinterpret_cast<uint8_t*>(&hash), sizeof(hash));

  const char* cache_env = OS::getenv("PYRO_CACHE_DIR");
  std::string cache_dir;
  if (cache_env != nullptr) {
    cache_dir = cache_env;
  } else {
    const char* home_env = OS::getenv("HOME");
    if (home_env != nullptr) {
      cache_dir = home_env;
      cache_dir += "/.pyro-compile-cache";
    }
  }

  char filename_buf[512] = {};
  snprintf(filename_buf, 512, "%s/%016zx", cache_dir.c_str(), hash);

  // Read compiled code from the cache
  if (!cache_dir.empty() && OS::fileExists(filename_buf)) {
    return OS::readFile(filename_buf);
  }

  // Cache miss, must run the compiler.
  std::unique_ptr<char[]> tmp_dir(OS::temporaryDirectory("python-tests"));
  const std::string dir(tmp_dir.get());
  const std::string py = dir + "/foo.py";
  const std::string pyc = dir + "/foo.pyc";
  const std::string cleanup = "rm -rf " + dir;
  std::ofstream output(py);
  output << src;
  output.close();
  const std::string command =
      "/usr/local/fbcode/gcc-5-glibc-2.23/bin/python3.6 -m compileall -q -b " +
      py;
  CHECK(system(command.c_str()) == 0, "Bytecode compilation failed");

  word len;
  char* result = OS::readFile(pyc.c_str(), &len);
  CHECK(system(cleanup.c_str()) == 0, "Bytecode compilation cleanup failed");

  // Cache the output if possible.
  if (!cache_dir.empty() && OS::dirExists(cache_dir.c_str())) {
    OS::writeFileExcl(filename_buf, result, len);
  }

  return result;
}

// Dict

RawObject Runtime::newDict() {
  HandleScope scope;
  Dict result(&scope, heap()->create<RawDict>());
  result->setNumItems(0);
  result->setData(empty_object_array_);
  return *result;
}

RawObject Runtime::newDictWithSize(word initial_size) {
  HandleScope scope;
  // TODO(jeethu): initialSize should be scaled up by a load factor.
  word initial_capacity = Utils::nextPowerOfTwo(initial_size);
  Tuple array(&scope,
              newTuple(Utils::maximum(static_cast<word>(kInitialDictCapacity),
                                      initial_capacity) *
                       Dict::Bucket::kNumPointers));
  Dict result(&scope, newDict());
  result->setData(*array);
  return *result;
}

void Runtime::dictAtPutWithHash(const Dict& dict, const Object& key,
                                const Object& value, const Object& key_hash) {
  HandleScope scope;
  Tuple data(&scope, dict->data());
  word index = -1;
  bool found = dictLookup(data, key, key_hash, &index);
  if (index == -1) {
    // TODO(mpage): Grow at a predetermined load factor, rather than when full
    Tuple new_data(&scope, dictGrow(data));
    dictLookup(new_data, key, key_hash, &index);
    DCHECK(index != -1, "invalid index %ld", index);
    dict->setData(*new_data);
    Dict::Bucket::set(*new_data, index, *key_hash, *key, *value);
  } else {
    Dict::Bucket::set(*data, index, *key_hash, *key, *value);
  }
  if (!found) {
    dict->setNumItems(dict->numItems() + 1);
  }
}

void Runtime::dictAtPut(const Dict& dict, const Object& key,
                        const Object& value) {
  HandleScope scope;
  Object key_hash(&scope, hash(*key));
  dictAtPutWithHash(dict, key, value, key_hash);
}

RawTuple Runtime::dictGrow(const Tuple& data) {
  HandleScope scope;
  word new_length = data->length() * kDictGrowthFactor;
  if (new_length == 0) {
    new_length = kInitialDictCapacity * Dict::Bucket::kNumPointers;
  }
  Tuple new_data(&scope, newTuple(new_length));
  // Re-insert items
  for (word i = 0; i < data->length(); i += Dict::Bucket::kNumPointers) {
    if (!Dict::Bucket::isFilled(*data, i)) {
      continue;
    }
    Object key(&scope, Dict::Bucket::key(*data, i));
    Object hash(&scope, Dict::Bucket::hash(*data, i));
    word index = -1;
    dictLookup(new_data, key, hash, &index);
    DCHECK(index != -1, "invalid index %ld", index);
    Dict::Bucket::set(*new_data, index, *hash, *key,
                      Dict::Bucket::value(*data, i));
  }
  return *new_data;
}

RawObject Runtime::dictAtWithHash(const Dict& dict, const Object& key,
                                  const Object& key_hash) {
  HandleScope scope;
  Tuple data(&scope, dict->data());
  word index = -1;
  bool found = dictLookup(data, key, key_hash, &index);
  if (found) {
    DCHECK(index != -1, "invalid index %ld", index);
    return Dict::Bucket::value(*data, index);
  }
  return Error::object();
}

RawObject Runtime::dictAt(const Dict& dict, const Object& key) {
  HandleScope scope;
  Object key_hash(&scope, hash(*key));
  return dictAtWithHash(dict, key, key_hash);
}

RawObject Runtime::dictAtIfAbsentPut(const Dict& dict, const Object& key,
                                     Callback<RawObject>* thunk) {
  HandleScope scope;
  Tuple data(&scope, dict->data());
  word index = -1;
  Object key_hash(&scope, hash(*key));
  bool found = dictLookup(data, key, key_hash, &index);
  if (found) {
    DCHECK(index != -1, "invalid index %ld", index);
    return Dict::Bucket::value(*data, index);
  }
  Object value(&scope, thunk->call());
  if (index == -1) {
    // TODO(mpage): Grow at a predetermined load factor, rather than when full
    Tuple new_data(&scope, dictGrow(data));
    dictLookup(new_data, key, key_hash, &index);
    DCHECK(index != -1, "invalid index %ld", index);
    dict->setData(*new_data);
    Dict::Bucket::set(*new_data, index, *key_hash, *key, *value);
  } else {
    Dict::Bucket::set(*data, index, *key_hash, *key, *value);
  }
  dict->setNumItems(dict->numItems() + 1);
  return *value;
}

RawObject Runtime::dictAtPutInValueCell(const Dict& dict, const Object& key,
                                        const Object& value) {
  RawObject result = dictAtIfAbsentPut(dict, key, newValueCellCallback());
  RawValueCell::cast(result)->setValue(*value);
  return result;
}

bool Runtime::dictIncludes(const Dict& dict, const Object& key) {
  HandleScope scope;
  Tuple data(&scope, dict->data());
  Object key_hash(&scope, hash(*key));
  word ignore;
  return dictLookup(data, key, key_hash, &ignore);
}

RawObject Runtime::dictRemove(const Dict& dict, const Object& key) {
  HandleScope scope;
  Tuple data(&scope, dict->data());
  word index = -1;
  Object key_hash(&scope, hash(*key));
  Object result(&scope, Error::object());
  bool found = dictLookup(data, key, key_hash, &index);
  if (found) {
    DCHECK(index != -1, "unexpected index %ld", index);
    result = Dict::Bucket::value(*data, index);
    Dict::Bucket::setTombstone(*data, index);
    dict->setNumItems(dict->numItems() - 1);
  }
  return *result;
}

bool Runtime::dictLookup(const Tuple& data, const Object& key,
                         const Object& key_hash, word* index) {
  word start = Dict::Bucket::getIndex(*data, *key_hash);
  word current = start;
  word next_free_index = -1;

  // TODO(mpage) - Quadratic probing?
  word length = data->length();
  if (length == 0) {
    *index = -1;
    return false;
  }

  do {
    if (Dict::Bucket::hasKey(*data, current, *key)) {
      *index = current;
      return true;
    }
    if (next_free_index == -1 && Dict::Bucket::isTombstone(*data, current)) {
      next_free_index = current;
    } else if (Dict::Bucket::isEmpty(*data, current)) {
      if (next_free_index == -1) {
        next_free_index = current;
      }
      break;
    }
    current = (current + Dict::Bucket::kNumPointers) % length;
  } while (current != start);

  *index = next_free_index;

  return false;
}

RawTuple Runtime::dictKeys(const Dict& dict) {
  HandleScope scope;
  Tuple data(&scope, dict->data());
  Tuple keys(&scope, newTuple(dict->numItems()));
  word num_keys = 0;
  for (word i = 0; i < data->length(); i += Dict::Bucket::kNumPointers) {
    if (Dict::Bucket::isFilled(*data, i)) {
      DCHECK(num_keys < keys->length(), "%ld ! < %ld", num_keys,
             keys->length());
      keys->atPut(num_keys, Dict::Bucket::key(*data, i));
      num_keys++;
    }
  }
  DCHECK(num_keys == keys->length(), "%ld != %ld", num_keys, keys->length());
  return *keys;
}

// DictItemIterator

RawObject Runtime::newDictItemIterator(const Dict& dict) {
  HandleScope scope;
  DictItemIterator result(&scope, heap()->create<RawDictItemIterator>());
  result->setIndex(0);
  result->setDict(*dict);
  result->setNumFound(0);
  return *result;
}

// DictItems

RawObject Runtime::newDictItems(const Dict& dict) {
  HandleScope scope;
  DictItems result(&scope, heap()->create<RawDictItems>());
  result->setDict(*dict);
  return *result;
}

// DictKeyIterator

RawObject Runtime::newDictKeyIterator(const Dict& dict) {
  HandleScope scope;
  DictKeyIterator result(&scope, heap()->create<RawDictKeyIterator>());
  result->setIndex(0);
  result->setDict(*dict);
  result->setNumFound(0);
  return *result;
}

// DictKeys

RawObject Runtime::newDictKeys(const Dict& dict) {
  HandleScope scope;
  DictKeys result(&scope, heap()->create<RawDictKeys>());
  result->setDict(*dict);
  return *result;
}

// DictValueIterator

RawObject Runtime::newDictValueIterator(const Dict& dict) {
  HandleScope scope;
  DictValueIterator result(&scope, heap()->create<RawDictValueIterator>());
  result->setIndex(0);
  result->setDict(*dict);
  result->setNumFound(0);
  return *result;
}

// DictValues

RawObject Runtime::newDictValues(const Dict& dict) {
  HandleScope scope;
  DictValues result(&scope, heap()->create<RawDictValues>());
  result->setDict(*dict);
  return *result;
}

// Set

RawObject Runtime::newSet() {
  HandleScope scope;
  Set result(&scope, heap()->create<RawSet>());
  result->setNumItems(0);
  result->setData(empty_object_array_);
  return *result;
}

RawObject Runtime::newSetWithSize(word initial_size) {
  HandleScope scope;
  Set result(&scope, heap()->create<RawSet>());
  word initial_capacity = Utils::nextPowerOfTwo(initial_size);
  Tuple array(&scope,
              newTuple(Utils::maximum(static_cast<word>(kInitialSetCapacity),
                                      initial_capacity) *
                       Set::Bucket::kNumPointers));
  result->setData(*array);
  result->setNumItems(0);
  return *result;
}

template <SetLookupType type>
word Runtime::setLookup(const Tuple& data, const Object& key,
                        const Object& key_hash) {
  word start = Set::Bucket::getIndex(*data, *key_hash);
  word current = start;
  word next_free_index = -1;

  // TODO(mpage) - Quadratic probing?
  word length = data->length();
  if (length == 0) {
    return -1;
  }

  do {
    if (Set::Bucket::hasKey(*data, current, *key)) {
      return current;
    }
    if (next_free_index == -1 && Set::Bucket::isTombstone(*data, current)) {
      if (type == SetLookupType::Insertion) {
        return current;
      }
      next_free_index = current;
    } else if (Set::Bucket::isEmpty(*data, current)) {
      if (next_free_index == -1) {
        next_free_index = current;
      }
      break;
    }
    current = (current + Set::Bucket::kNumPointers) & (length - 1);
  } while (current != start);

  if (type == SetLookupType::Insertion) {
    return next_free_index;
  }
  return -1;
}

RawTuple Runtime::setGrow(const Tuple& data) {
  HandleScope scope;
  word new_length = data->length() * kSetGrowthFactor;
  if (new_length == 0) {
    new_length = kInitialSetCapacity * Set::Bucket::kNumPointers;
  }
  Tuple new_data(&scope, newTuple(new_length));
  // Re-insert items
  for (word i = 0, length = data->length(); i < length;
       i += Set::Bucket::kNumPointers) {
    if (Set::Bucket::isFilled(*data, i)) {
      Object key(&scope, Set::Bucket::key(*data, i));
      Object hash(&scope, Set::Bucket::hash(*data, i));
      word index = setLookup<SetLookupType::Insertion>(new_data, key, hash);
      DCHECK(index != -1, "unexpected index %ld", index);
      Set::Bucket::set(*new_data, index, *hash, *key);
    }
  }
  return *new_data;
}

RawObject Runtime::setAddWithHash(const Set& set, const Object& value,
                                  const Object& key_hash) {
  HandleScope scope;
  Tuple data(&scope, set->data());
  word index = setLookup<SetLookupType::Lookup>(data, value, key_hash);
  if (index != -1) {
    return Set::Bucket::key(*data, index);
  }
  Tuple new_data(&scope, *data);
  if (data->length() == 0 || set->numItems() >= data->length() / 2) {
    new_data = setGrow(data);
  }
  index = setLookup<SetLookupType::Insertion>(new_data, value, key_hash);
  DCHECK(index != -1, "unexpected index %ld", index);
  set->setData(*new_data);
  Set::Bucket::set(*new_data, index, *key_hash, *value);
  set->setNumItems(set->numItems() + 1);
  return *value;
}

RawObject Runtime::setAdd(const Set& set, const Object& value) {
  HandleScope scope;
  Object key_hash(&scope, hash(*value));
  return setAddWithHash(set, value, key_hash);
}

RawObject Runtime::setCopy(const Set& set) {
  word num_items = set->numItems();
  if (num_items == 0) {
    return newSet();
  }

  HandleScope scope;
  Set new_set(&scope, newSetWithSize(num_items));
  Tuple data(&scope, set->data());
  Tuple new_data(&scope, new_set->data());
  Object key(&scope, NoneType::object());
  Object key_hash(&scope, NoneType::object());
  for (word i = 0, data_len = data->length(); i < data_len;
       i += Set::Bucket::kNumPointers) {
    if (!Set::Bucket::isFilled(*data, i)) {
      continue;
    }
    key = Set::Bucket::key(*data, i);
    key_hash = Set::Bucket::hash(*data, i);
    Set::Bucket::set(*new_data, i, *key_hash, *key);
  }
  new_set->setNumItems(set->numItems());
  return *new_set;
}

bool Runtime::setIsSubset(Thread* thread, const Set& set, const Set& other) {
  HandleScope scope(thread);
  Tuple data(&scope, set->data());
  Tuple other_data(&scope, other->data());
  Object key(&scope, NoneType::object());
  Object key_hash(&scope, NoneType::object());
  for (word i = 0; i < data->length(); i += Set::Bucket::kNumPointers) {
    if (RawSet::Bucket::isFilled(*data, i)) {
      key = RawSet::Bucket::key(*data, i);
      key_hash = RawSet::Bucket::hash(*data, i);
      if (setLookup<SetLookupType::Lookup>(other_data, key, key_hash) == -1) {
        return false;
      }
    }
  }
  return true;
}

bool Runtime::setIsProperSubset(Thread* thread, const Set& set,
                                const Set& other) {
  if (set->numItems() == other->numItems()) {
    return false;
  }
  return setIsSubset(thread, set, other);
}

bool Runtime::setEquals(Thread* thread, const Set& set, const Set& other) {
  if (set->numItems() != other->numItems()) {
    return false;
  }
  if (*set == *other) {
    return true;
  }
  return setIsSubset(thread, set, other);
}

bool Runtime::setIncludes(const Set& set, const Object& value) {
  HandleScope scope;
  Tuple data(&scope, set->data());
  Object key_hash(&scope, hash(*value));
  return setLookup<SetLookupType::Lookup>(data, value, key_hash) != -1;
}

RawObject Runtime::setIntersection(Thread* thread, const Set& set,
                                   const Object& iterable) {
  HandleScope scope;
  Set dst(&scope, Runtime::newSet());
  Object key(&scope, NoneType::object());
  Object key_hash(&scope, NoneType::object());
  // Special case for sets
  if (iterable->isSet()) {
    Set self(&scope, *set);
    Set other(&scope, *iterable);
    if (set->numItems() == 0 || other->numItems() == 0) {
      return *dst;
    }
    // Iterate over the smaller set
    if (set->numItems() > other->numItems()) {
      self = *iterable;
      other = *set;
    }
    Tuple data(&scope, self->data());
    Tuple other_data(&scope, other->data());
    for (word i = 0; i < data->length(); i += Set::Bucket::kNumPointers) {
      if (!Set::Bucket::isFilled(*data, i)) {
        continue;
      }
      key = Set::Bucket::key(*data, i);
      key_hash = Set::Bucket::hash(*data, i);
      if (setLookup<SetLookupType::Lookup>(other_data, key, key_hash) != -1) {
        setAddWithHash(dst, key, key_hash);
      }
    }
    return *dst;
  }
  // Generic case
  Object iter_method(
      &scope, Interpreter::lookupMethod(thread, thread->currentFrame(),
                                        iterable, SymbolId::kDunderIter));
  if (iter_method->isError()) {
    return thread->raiseTypeErrorWithCStr("object is not iterable");
  }
  Object iterator(&scope,
                  Interpreter::callMethod1(thread, thread->currentFrame(),
                                           iter_method, iterable));
  if (iterator->isError()) {
    return thread->raiseTypeErrorWithCStr("object is not iterable");
  }
  Object next_method(
      &scope, Interpreter::lookupMethod(thread, thread->currentFrame(),
                                        iterator, SymbolId::kDunderNext));
  if (next_method->isError()) {
    return thread->raiseTypeErrorWithCStr("iter() returned a non-iterator");
  }
  if (set->numItems() == 0) {
    return *dst;
  }
  Tuple data(&scope, set->data());
  while (!isIteratorExhausted(thread, iterator)) {
    key = Interpreter::callMethod1(thread, thread->currentFrame(), next_method,
                                   iterator);
    if (key->isError()) {
      return *key;
    }
    key_hash = hash(*key);
    if (setLookup<SetLookupType::Lookup>(data, key, key_hash) != -1) {
      setAddWithHash(dst, key, key_hash);
    }
  }
  return *dst;
}

bool Runtime::setRemove(const Set& set, const Object& value) {
  HandleScope scope;
  Tuple data(&scope, set->data());
  Object key_hash(&scope, hash(*value));
  word index = setLookup<SetLookupType::Lookup>(data, value, key_hash);
  if (index != -1) {
    Set::Bucket::setTombstone(*data, index);
    set->setNumItems(set->numItems() - 1);
    return true;
  }
  return false;
}

RawObject Runtime::setUpdate(Thread* thread, const Set& dst,
                             const Object& iterable) {
  HandleScope scope(thread);
  Object elt(&scope, NoneType::object());
  // Special case for lists
  if (iterable->isList()) {
    List src(&scope, *iterable);
    for (word i = 0; i < src->numItems(); i++) {
      elt = src->at(i);
      setAdd(dst, elt);
    }
    return *dst;
  }
  // Special case for lists iterators
  if (iterable->isListIterator()) {
    ListIterator list_iter(&scope, *iterable);
    List src(&scope, list_iter->list());
    for (word i = 0; i < src->numItems(); i++) {
      elt = src->at(i);
      setAdd(dst, elt);
    }
  }
  // Special case for tuples
  if (iterable->isTuple()) {
    Tuple tuple(&scope, *iterable);
    if (tuple->length() > 0) {
      for (word i = 0; i < tuple->length(); i++) {
        elt = tuple->at(i);
        setAdd(dst, elt);
      }
    }
    return *dst;
  }
  // Special case for sets
  if (iterable->isSet()) {
    Set src(&scope, *iterable);
    Tuple data(&scope, src->data());
    if (src->numItems() > 0) {
      Object hash(&scope, NoneType::object());
      for (word i = 0; i < data->length(); i += Set::Bucket::kNumPointers) {
        if (Set::Bucket::isFilled(*data, i)) {
          elt = Set::Bucket::key(*data, i);
          // take hash from data to avoid recomputing it.
          hash = Set::Bucket::hash(*data, i);
          setAddWithHash(dst, elt, hash);
        }
      }
    }
    return *dst;
  }
  // Special case for dicts
  if (iterable->isDict()) {
    Dict dict(&scope, *iterable);
    if (dict->numItems() > 0) {
      Tuple keys(&scope, dictKeys(dict));
      Object value(&scope, NoneType::object());
      for (word i = 0; i < keys->length(); i++) {
        value = keys->at(i);
        setAdd(dst, value);
      }
    }
    return *dst;
  }
  // Generic case
  Object iter_method(
      &scope, Interpreter::lookupMethod(thread, thread->currentFrame(),
                                        iterable, SymbolId::kDunderIter));
  if (iter_method->isError()) {
    return thread->raiseTypeErrorWithCStr("object is not iterable");
  }
  Object iterator(&scope,
                  Interpreter::callMethod1(thread, thread->currentFrame(),
                                           iter_method, iterable));
  if (iterator->isError()) {
    return thread->raiseTypeErrorWithCStr("object is not iterable");
  }
  Object next_method(
      &scope, Interpreter::lookupMethod(thread, thread->currentFrame(),
                                        iterator, SymbolId::kDunderNext));
  if (next_method->isError()) {
    return thread->raiseTypeErrorWithCStr("iter() returned a non-iterator");
  }
  Object value(&scope, NoneType::object());
  while (!isIteratorExhausted(thread, iterator)) {
    value = Interpreter::callMethod1(thread, thread->currentFrame(),
                                     next_method, iterator);
    if (value->isError()) {
      return *value;
    }
    setAdd(dst, value);
  }
  return *dst;
}

RawObject Runtime::dictUpdate(Thread* thread, const Dict& dict,
                              const Object& mapping) {
  return dictUpdate<DictUpdateType::Update>(thread, dict, mapping);
}

RawObject Runtime::dictMerge(Thread* thread, const Dict& dict,
                             const Object& mapping) {
  return dictUpdate<DictUpdateType::Merge>(thread, dict, mapping);
}

template <DictUpdateType type>
inline RawObject Runtime::dictUpdate(Thread* thread, const Dict& dict,
                                     const Object& mapping) {
  HandleScope scope;
  Object key(&scope, NoneType::object());
  Object value(&scope, NoneType::object());
  if (mapping->isDict()) {
    DCHECK(*mapping != *dict, "Cannot update dict with itself");
    Dict other(&scope, *mapping);
    Tuple data(&scope, other->data());
    for (word i = 0; i < data->length(); i += Dict::Bucket::kNumPointers) {
      if (Dict::Bucket::isFilled(*data, i)) {
        key = Dict::Bucket::key(*data, i);
        value = Dict::Bucket::value(*data, i);
        if (type == DictUpdateType::Merge) {
          if (!isInstanceOfStr(*key)) {
            return thread->raiseTypeErrorWithCStr("keywords must be strings");
          }
          if (dictIncludes(dict, key)) {
            return thread->raiseTypeErrorWithCStr(
                "got multiple values for keyword argument");
          }
        }
        dictAtPut(dict, key, value);
      }
    }
    return *dict;
  }
  Frame* frame = thread->currentFrame();
  Object keys_method(&scope, Interpreter::lookupMethod(thread, frame, mapping,
                                                       SymbolId::kKeys));

  if (keys_method->isError()) {
    return thread->raiseTypeErrorWithCStr("object is not a mapping");
  }

  // Generic mapping, use keys() and __getitem__()
  Object subscr_method(&scope,
                       Interpreter::lookupMethod(thread, frame, mapping,
                                                 SymbolId::kDunderGetItem));
  if (subscr_method->isError()) {
    return thread->raiseTypeErrorWithCStr("object is not subscriptable");
  }
  Object keys(&scope,
              Interpreter::callMethod1(thread, frame, keys_method, mapping));
  if (keys->isList()) {
    List keys_list(&scope, *keys);
    for (word i = 0; i < keys_list->numItems(); ++i) {
      key = keys_list->at(i);
      if (type == DictUpdateType::Merge) {
        if (!isInstanceOfStr(*key)) {
          return thread->raiseTypeErrorWithCStr("keywords must be strings");
        }
        if (dictIncludes(dict, key)) {
          return thread->raiseTypeErrorWithCStr(
              "got multiple values for keyword argument");
        }
      }
      value =
          Interpreter::callMethod2(thread, frame, subscr_method, mapping, key);
      if (value->isError()) {
        return *value;
      }
      dictAtPut(dict, key, value);
    }
    return *dict;
  }

  if (keys->isTuple()) {
    Tuple keys_tuple(&scope, *keys);
    for (word i = 0; i < keys_tuple->length(); ++i) {
      key = keys_tuple->at(i);
      if (type == DictUpdateType::Merge) {
        if (!isInstanceOfStr(*key)) {
          return thread->raiseTypeErrorWithCStr("keywords must be strings");
        }
        if (dictIncludes(dict, key)) {
          return thread->raiseTypeErrorWithCStr(
              "got multiple values for keyword argument");
        }
      }
      value =
          Interpreter::callMethod2(thread, frame, subscr_method, mapping, key);
      if (value->isError()) {
        return *value;
      }
      dictAtPut(dict, key, value);
    }
    return *dict;
  }

  // keys is probably an iterator
  Object iter_method(
      &scope, Interpreter::lookupMethod(thread, thread->currentFrame(), keys,
                                        SymbolId::kDunderIter));
  if (iter_method->isError()) {
    return thread->raiseTypeErrorWithCStr("o.keys() are not iterable");
  }

  Object iterator(&scope,
                  Interpreter::callMethod1(thread, thread->currentFrame(),
                                           iter_method, keys));
  if (iterator->isError()) {
    return thread->raiseTypeErrorWithCStr("o.keys() are not iterable");
  }
  Object next_method(
      &scope, Interpreter::lookupMethod(thread, thread->currentFrame(),
                                        iterator, SymbolId::kDunderNext));
  if (next_method->isError()) {
    thread->raiseTypeErrorWithCStr("o.keys() are not iterable");
    thread->abortOnPendingException();
  }
  while (!isIteratorExhausted(thread, iterator)) {
    key = Interpreter::callMethod1(thread, thread->currentFrame(), next_method,
                                   iterator);
    if (key->isError()) {
      return *key;
    }
    if (type == DictUpdateType::Merge) {
      if (!isInstanceOfStr(*key)) {
        return thread->raiseTypeErrorWithCStr("keywords must be strings");
      }
      if (dictIncludes(dict, key)) {
        return thread->raiseTypeErrorWithCStr(
            "got multiple values for keyword argument");
      }
    }
    value =
        Interpreter::callMethod2(thread, frame, subscr_method, mapping, key);
    if (value->isError()) {
      return *value;
    }
    dictAtPut(dict, key, value);
  }
  return *dict;
}

RawObject Runtime::dictItemIteratorNext(Thread* thread,
                                        DictItemIterator& iter) {
  HandleScope scope(thread);
  Dict dict(&scope, iter.dict());
  Tuple buckets(&scope, dict.data());
  word jump = Dict::Bucket::kNumPointers;

  word i = iter.index();
  for (; i < buckets->length() && Dict::Bucket::isEmpty(*buckets, i);
       i += jump) {
  }

  if (i < buckets->length()) {
    // At this point, we have found a valid index in the buckets.
    Object key(&scope, Dict::Bucket::key(*buckets, i));
    Object value(&scope, Dict::Bucket::value(*buckets, i));
    Tuple kv_pair(&scope, newTuple(2));
    kv_pair->atPut(0, *key);
    kv_pair->atPut(1, *value);
    iter.setIndex(i + jump);
    iter.setNumFound(iter.numFound() + 1);
    return *kv_pair;
  }

  // We hit the end.
  iter.setIndex(i);
  return Error::object();
}

RawObject Runtime::dictKeyIteratorNext(Thread* thread, DictKeyIterator& iter) {
  HandleScope scope(thread);
  Dict dict(&scope, iter.dict());
  Tuple buckets(&scope, dict.data());
  word jump = Dict::Bucket::kNumPointers;

  word i = iter.index();
  for (; i < buckets->length() && Dict::Bucket::isEmpty(*buckets, i);
       i += jump) {
  }

  if (i < buckets->length()) {
    // At this point, we have found a valid index in the buckets.
    iter.setIndex(i + jump);
    iter.setNumFound(iter.numFound() + 1);
    return Dict::Bucket::key(*buckets, i);
  }

  // We hit the end.
  iter.setIndex(i);
  return Error::object();
}

RawObject Runtime::dictValueIteratorNext(Thread* thread,
                                         DictValueIterator& iter) {
  HandleScope scope(thread);
  Dict dict(&scope, iter.dict());
  Tuple buckets(&scope, dict.data());
  word jump = Dict::Bucket::kNumPointers;

  word i = iter.index();
  for (; i < buckets->length() && Dict::Bucket::isEmpty(*buckets, i);
       i += jump) {
  }

  if (i < buckets->length()) {
    // At this point, we have found a valid index in the buckets.
    iter.setIndex(i + jump);
    iter.setNumFound(iter.numFound() + 1);
    return Dict::Bucket::value(*buckets, i);
  }

  // We hit the end.
  iter.setIndex(i);
  return Error::object();
}

RawObject Runtime::genSend(Thread* thread, const GeneratorBase& gen,
                           const Object& value) {
  HandleScope scope(thread);
  HeapFrame heap_frame(&scope, gen->heapFrame());
  thread->checkStackOverflow(heap_frame->numFrameWords() * kPointerSize);
  Frame* live_frame = heap_frame->copyToNewStackFrame(thread->currentFrame());
  if (live_frame->virtualPC() != 0) {
    live_frame->pushValue(*value);
  }
  thread->linkFrame(live_frame);
  return Interpreter::execute(thread, live_frame);
}

void Runtime::genSave(Thread* thread, const GeneratorBase& gen) {
  HandleScope scope(thread);
  HeapFrame heap_frame(&scope, gen->heapFrame());
  Frame* live_frame = thread->currentFrame();
  DCHECK(live_frame->valueStackSize() <= heap_frame->maxStackSize(),
         "not enough space in RawGeneratorBase to save live stack");
  heap_frame->copyFromStackFrame(live_frame);
  thread->popFrame();
}

RawGeneratorBase Runtime::genFromStackFrame(Frame* frame) {
  // For now, we have the invariant that GeneratorBase bodies are only invoked
  // by __next__() or send(), which have the GeneratorBase as their first local.
  return RawGeneratorBase::cast(frame->previousFrame()->getLocal(0));
}

RawObject Runtime::newValueCell() { return heap()->create<RawValueCell>(); }

RawObject Runtime::newWeakRef() { return heap()->create<RawWeakRef>(); }

void Runtime::collectAttributes(const Code& code, const Dict& attributes) {
  HandleScope scope;
  Bytes bc(&scope, code->code());
  Tuple names(&scope, code->names());

  word len = bc->length();
  for (word i = 0; i < len - 3; i += 2) {
    // If the current instruction is EXTENDED_ARG we must skip it and the next
    // instruction.
    if (bc->byteAt(i) == Bytecode::EXTENDED_ARG) {
      i += 2;
      continue;
    }
    // Check for LOAD_FAST 0 (self)
    if (bc->byteAt(i) != Bytecode::LOAD_FAST || bc->byteAt(i + 1) != 0) {
      continue;
    }
    // Followed by a STORE_ATTR
    if (bc->byteAt(i + 2) != Bytecode::STORE_ATTR) {
      continue;
    }
    word name_index = bc->byteAt(i + 3);
    Object name(&scope, names->at(name_index));
    dictAtPut(attributes, name, name);
  }
}

RawObject Runtime::classConstructor(const Type& type) {
  HandleScope scope;
  Dict type_dict(&scope, type->dict());
  Object init(&scope, symbols()->DunderInit());
  RawObject value = dictAt(type_dict, init);
  if (value->isError()) {
    return NoneType::object();
  }
  return RawValueCell::cast(value)->value();
}

RawObject Runtime::computeInitialLayout(Thread* thread, const Type& type,
                                        LayoutId base_layout_id) {
  HandleScope scope(thread);
  // Create the layout
  LayoutId layout_id = reserveLayoutId();
  Layout layout(&scope, layoutCreateSubclassWithBuiltins(
                            layout_id, base_layout_id,
                            View<BuiltinAttribute>(nullptr, 0)));

  Tuple mro(&scope, type->mro());
  Dict attrs(&scope, newDict());

  // Collect set of in-object attributes by scanning the __init__ method of
  // each class in the MRO
  for (word i = 0; i < mro->length(); i++) {
    Type mro_type(&scope, mro->at(i));
    Object maybe_init(&scope, classConstructor(mro_type));
    if (!maybe_init->isFunction()) {
      continue;
    }
    Function init(&scope, *maybe_init);
    RawObject maybe_code = init->code();
    if (!maybe_code->isCode()) {
      continue;
    }
    Code code(&scope, maybe_code);
    collectAttributes(code, attrs);
  }

  layout->setNumInObjectAttributes(layout->numInObjectAttributes() +
                                   attrs->numItems());
  layoutAtPut(layout_id, *layout);

  return *layout;
}

RawObject Runtime::lookupNameInMro(Thread* thread, const Type& type,
                                   const Object& name) {
  HandleScope scope(thread);
  Tuple mro(&scope, type->mro());
  for (word i = 0; i < mro->length(); i++) {
    Type mro_type(&scope, mro->at(i));
    Dict dict(&scope, mro_type->dict());
    Object value_cell(&scope, dictAt(dict, name));
    if (!value_cell->isError()) {
      return RawValueCell::cast(*value_cell)->value();
    }
  }
  return Error::object();
}

RawObject Runtime::attributeAt(Thread* thread, const Object& receiver,
                               const Object& name) {
  if (!name->isStr()) {
    return thread->raiseTypeErrorWithCStr("attribute name must be a string");
  }

  // A minimal implementation of getattr needed to get richards running.
  RawObject result;
  HandleScope scope(thread);
  if (isInstanceOfType(*receiver)) {
    result = classGetAttr(thread, receiver, name);
  } else if (receiver->isModule()) {
    result = moduleGetAttr(thread, receiver, name);
  } else if (receiver->isSuper()) {
    // TODO(T27518836): remove when we support __getattro__
    result = superGetAttr(thread, receiver, name);
  } else if (receiver->isFunction()) {
    result = functionGetAttr(thread, receiver, name);
  } else {
    // everything else should fallback to instance
    result = instanceGetAttr(thread, receiver, name);
  }
  return result;
}

RawObject Runtime::attributeAtPut(Thread* thread, const Object& receiver,
                                  const Object& name, const Object& value) {
  if (!name->isStr()) {
    return thread->raiseTypeErrorWithCStr("attribute name must be a string");
  }

  HandleScope scope(thread);
  Object interned_name(&scope, internStr(name));
  // A minimal implementation of setattr needed to get richards running.
  RawObject result;
  if (isInstanceOfType(*receiver)) {
    result = classSetAttr(thread, receiver, interned_name, value);
  } else if (receiver->isModule()) {
    result = moduleSetAttr(thread, receiver, interned_name, value);
  } else if (receiver->isFunction()) {
    result = functionSetAttr(thread, receiver, interned_name, value);
  } else {
    // everything else should fallback to instance
    result = instanceSetAttr(thread, receiver, interned_name, value);
  }
  return result;
}

RawObject Runtime::attributeDel(Thread* thread, const Object& receiver,
                                const Object& name) {
  HandleScope scope(thread);
  // If present, __delattr__ overrides all attribute deletion logic.
  Type type(&scope, typeOf(*receiver));
  Object dunder_delattr(
      &scope, lookupSymbolInMro(thread, type, SymbolId::kDunderDelattr));
  RawObject result;
  if (!dunder_delattr->isError()) {
    result = Interpreter::callMethod2(thread, thread->currentFrame(),
                                      dunder_delattr, receiver, name);
  } else if (isInstanceOfType(*receiver)) {
    result = classDelAttr(thread, receiver, name);
  } else if (receiver->isModule()) {
    result = moduleDelAttr(thread, receiver, name);
  } else {
    result = instanceDelAttr(thread, receiver, name);
  }

  return result;
}

RawObject Runtime::strConcat(const Str& left, const Str& right) {
  HandleScope scope;
  word left_len = left->length();
  word right_len = right->length();
  word result_len = left_len + right_len;
  // Small result
  if (result_len <= RawSmallStr::kMaxLength) {
    byte buffer[RawSmallStr::kMaxLength];
    left->copyTo(buffer, left_len);
    right->copyTo(buffer + left_len, right_len);
    return SmallStr::fromBytes(View<byte>(buffer, result_len));
  }
  // Large result
  LargeStr result(&scope, heap()->createLargeStr(result_len));
  left->copyTo(reinterpret_cast<byte*>(result->address()), left_len);
  right->copyTo(reinterpret_cast<byte*>(result->address() + left_len),
                right_len);
  return *result;
}

RawObject Runtime::strJoin(Thread* thread, const Str& sep, const Tuple& items,
                           word allocated) {
  HandleScope scope(thread);
  word result_len = 0;
  for (word i = 0; i < allocated; ++i) {
    Object elt(&scope, items->at(i));
    if (!elt->isStr() && !isInstanceOfStr(*elt)) {
      return thread->raiseTypeError(
          newStrFromFormat("sequence item %ld: expected str instance", i));
    }
    Str str(&scope, items->at(i));
    result_len += str->length();
  }
  if (allocated > 1) {
    result_len += sep->length() * (allocated - 1);
  }
  // Small result
  if (result_len <= RawSmallStr::kMaxLength) {
    byte buffer[RawSmallStr::kMaxLength];
    for (word i = 0, offset = 0; i < allocated; ++i) {
      Str str(&scope, items->at(i));
      word str_len = str->length();
      str->copyTo(&buffer[offset], str_len);
      offset += str_len;
      if ((i + 1) < allocated) {
        word sep_len = sep->length();
        sep->copyTo(&buffer[offset], sep_len);
        offset += sep->length();
      }
    }
    return SmallStr::fromBytes(View<byte>(buffer, result_len));
  }
  // Large result
  LargeStr result(&scope, heap()->createLargeStr(result_len));
  for (word i = 0, offset = 0; i < allocated; ++i) {
    Str str(&scope, items->at(i));
    word str_len = str->length();
    str->copyTo(reinterpret_cast<byte*>(result->address() + offset), str_len);
    offset += str_len;
    if ((i + 1) < allocated) {
      word sep_len = sep->length();
      sep->copyTo(reinterpret_cast<byte*>(result->address() + offset), sep_len);
      offset += sep_len;
    }
  }
  return *result;
}

RawObject Runtime::computeFastGlobals(const Code& code, const Dict& globals,
                                      const Dict& builtins) {
  HandleScope scope;
  Bytes bytes(&scope, code->code());
  Tuple names(&scope, code->names());
  Tuple fast_globals(&scope, newTuple(names->length()));
  for (word i = 0; i < bytes->length(); i += 2) {
    Bytecode bc = static_cast<Bytecode>(bytes->byteAt(i));
    word arg = bytes->byteAt(i + 1);
    while (bc == EXTENDED_ARG) {
      i += 2;
      bc = static_cast<Bytecode>(bytes->byteAt(i));
      arg = (arg << 8) | bytes->byteAt(i + 1);
    }
    if (bc != LOAD_GLOBAL && bc != STORE_GLOBAL && bc != DELETE_GLOBAL) {
      continue;
    }
    Object key(&scope, names->at(arg));
    RawObject value = dictAt(globals, key);
    if (value->isError()) {
      value = dictAt(builtins, key);
      if (value->isError()) {
        // insert a place holder to allow {STORE|DELETE}_GLOBAL
        Object handle(&scope, value);
        value = dictAtPutInValueCell(builtins, key, handle);
        RawValueCell::cast(value)->makeUnbound();
      }
      Object handle(&scope, value);
      value = dictAtPutInValueCell(globals, key, handle);
    }
    DCHECK(value->isValueCell(), "not  value cell");
    fast_globals->atPut(arg, value);
  }
  return *fast_globals;
}

// See https://github.com/python/cpython/blob/master/Objects/lnotab_notes.txt
// for details about the line number table format
word Runtime::codeOffsetToLineNum(Thread* thread, const Code& code,
                                  word offset) {
  HandleScope scope(thread);
  Bytes table(&scope, code->lnotab());
  word line = code->firstlineno();
  word cur_offset = 0;
  for (word i = 0; i < table->length(); i += 2) {
    cur_offset += table->byteAt(i);
    if (cur_offset > offset) {
      break;
    }
    line += static_cast<sbyte>(table->byteAt(i + 1));
  }
  return line;
}

bool Runtime::isSubclass(const Type& subclass, const Type& superclass) {
  HandleScope scope;
  Tuple mro(&scope, subclass->mro());
  for (word i = 0; i < mro->length(); i++) {
    if (mro->at(i) == *superclass) {
      return true;
    }
  }
  return false;
}

bool Runtime::isInstance(const Object& obj, const Type& type) {
  HandleScope scope;
  Type obj_class(&scope, typeOf(*obj));
  return isSubclass(obj_class, type);
}

RawObject Runtime::newClassMethod() { return heap()->create<RawClassMethod>(); }

RawObject Runtime::newSuper() { return heap()->create<RawSuper>(); }

RawObject Runtime::newStrIterator(const Object& str) {
  HandleScope scope;
  StrIterator result(&scope, heap()->create<RawStrIterator>());
  result->setIndex(0);
  result->setStr(*str);
  return *result;
}

RawObject Runtime::newTupleIterator(const Object& tuple) {
  HandleScope scope;
  TupleIterator result(&scope, heap()->create<RawTupleIterator>());
  result->setIndex(0);
  result->setTuple(*tuple);
  return *result;
}

RawObject Runtime::computeBuiltinBase(Thread* thread, const Type& type) {
  // The base class can only be one of the builtin bases including object.
  // We use the first non-object builtin base if any, throw if multiple.
  HandleScope scope(thread);
  Tuple mro(&scope, type->mro());
  Type object_type(&scope, typeAt(LayoutId::kObject));
  Type candidate(&scope, *object_type);
  // Skip itself since builtin class won't go through this.
  DCHECK(*type == mro->at(0) && type->instanceLayout()->isNoneType(),
         "type's layout should not be set at this point");
  for (word i = 1; i < mro->length(); i++) {
    Type mro_type(&scope, mro->at(i));
    if (!mro_type->isBuiltin()) {
      continue;
    }
    if (*candidate == *object_type) {
      candidate = *mro_type;
    } else if (*mro_type != *object_type &&
               !RawTuple::cast(candidate->mro())->contains(*mro_type)) {
      return thread->raiseTypeErrorWithCStr(
          "multiple bases have instance lay-out conflict");
    }
  }
  return *candidate;
}

RawObject Runtime::instanceAt(Thread* thread, const HeapObject& instance,
                              const Object& name) {
  HandleScope scope(thread);

  // Figure out where the attribute lives in the instance
  Layout layout(&scope, layoutAt(instance->layoutId()));
  AttributeInfo info;
  if (!layoutFindAttribute(thread, layout, name, &info)) {
    return Error::object();
  }

  // Retrieve the attribute
  RawObject result;
  if (info.isInObject()) {
    result = instance->instanceVariableAt(info.offset());
  } else {
    Tuple overflow(&scope,
                   instance->instanceVariableAt(layout->overflowOffset()));
    result = overflow->at(info.offset());
  }

  return result;
}

RawObject Runtime::instanceAtPut(Thread* thread, const HeapObject& instance,
                                 const Object& name, const Object& value) {
  HandleScope scope(thread);

  // If the attribute doesn't exist we'll need to transition the layout
  bool has_new_layout_id = false;
  Layout layout(&scope, layoutAt(instance->layoutId()));
  AttributeInfo info;
  if (!layoutFindAttribute(thread, layout, name, &info)) {
    // Transition the layout
    layout = layoutAddAttribute(thread, layout, name, 0);
    has_new_layout_id = true;

    bool found = layoutFindAttribute(thread, layout, name, &info);
    CHECK(found, "couldn't find attribute on new layout");
  }

  // Store the attribute
  if (info.isInObject()) {
    instance->instanceVariableAtPut(info.offset(), *value);
  } else {
    // Build the new overflow array
    Tuple overflow(&scope,
                   instance->instanceVariableAt(layout->overflowOffset()));
    Tuple new_overflow(&scope, newTuple(overflow->length() + 1));
    overflow->copyTo(*new_overflow);
    new_overflow->atPut(info.offset(), *value);
    instance->instanceVariableAtPut(layout->overflowOffset(), *new_overflow);
  }

  if (has_new_layout_id) {
    instance->setHeader(instance->header()->withLayoutId(layout->id()));
  }

  return NoneType::object();
}

RawObject Runtime::instanceDel(Thread* thread, const HeapObject& instance,
                               const Object& name) {
  HandleScope scope(thread);

  // Make the attribute invisible
  Layout old_layout(&scope, layoutAt(instance->layoutId()));
  Object result(&scope, layoutDeleteAttribute(thread, old_layout, name));
  if (result->isError()) {
    return Error::object();
  }
  LayoutId new_layout_id = RawLayout::cast(*result)->id();
  instance->setHeader(instance->header()->withLayoutId(new_layout_id));

  // Remove the reference to the attribute value from the instance
  AttributeInfo info;
  bool found = layoutFindAttribute(thread, old_layout, name, &info);
  CHECK(found, "couldn't find attribute");
  if (info.isInObject()) {
    instance->instanceVariableAtPut(info.offset(), NoneType::object());
  } else {
    Tuple overflow(&scope,
                   instance->instanceVariableAt(old_layout->overflowOffset()));
    overflow->atPut(info.offset(), NoneType::object());
  }

  return NoneType::object();
}

RawObject Runtime::layoutFollowEdge(const List& edges, const Object& label) {
  DCHECK(edges->numItems() % 2 == 0,
         "edges must contain an even number of elements");
  for (word i = 0; i < edges->numItems(); i++) {
    if (edges->at(i) == *label) {
      return edges->at(i + 1);
    }
  }
  return Error::object();
}

void Runtime::layoutAddEdge(const List& edges, const Object& label,
                            const Object& layout) {
  DCHECK(edges->numItems() % 2 == 0,
         "edges must contain an even number of elements");
  listAdd(edges, label);
  listAdd(edges, layout);
}

bool Runtime::layoutFindAttribute(Thread* thread, const Layout& layout,
                                  const Object& name, AttributeInfo* info) {
  HandleScope scope(thread);
  Object iname(&scope, internStr(name));

  // Check in-object attributes
  Tuple in_object(&scope, layout->inObjectAttributes());
  for (word i = 0; i < in_object->length(); i++) {
    Tuple entry(&scope, in_object->at(i));
    if (entry->at(0) == *iname) {
      *info = AttributeInfo(entry->at(1));
      return true;
    }
  }

  // Check overflow attributes
  Tuple overflow(&scope, layout->overflowAttributes());
  for (word i = 0; i < overflow->length(); i++) {
    Tuple entry(&scope, overflow->at(i));
    if (entry->at(0) == *iname) {
      *info = AttributeInfo(entry->at(1));
      return true;
    }
  }

  return false;
}

RawObject Runtime::layoutCreateEmpty(Thread* thread) {
  HandleScope scope(thread);
  Layout result(&scope, newLayout());
  result->setId(reserveLayoutId());
  layoutAtPut(result->id(), *result);
  return *result;
}

RawObject Runtime::layoutCreateChild(Thread* thread, const Layout& layout) {
  HandleScope scope(thread);
  Layout new_layout(&scope, newLayout());
  new_layout->setId(reserveLayoutId());
  new_layout->setDescribedType(layout->describedType());
  new_layout->setNumInObjectAttributes(layout->numInObjectAttributes());
  new_layout->setInObjectAttributes(layout->inObjectAttributes());
  new_layout->setOverflowAttributes(layout->overflowAttributes());
  new_layout->setInstanceSize(layout->instanceSize());
  layoutAtPut(new_layout->id(), *new_layout);
  return *new_layout;
}

RawObject Runtime::layoutAddAttributeEntry(Thread* thread, const Tuple& entries,
                                           const Object& name,
                                           AttributeInfo info) {
  HandleScope scope(thread);
  Tuple new_entries(&scope, newTuple(entries->length() + 1));
  entries->copyTo(*new_entries);

  Tuple entry(&scope, newTuple(2));
  entry->atPut(0, *name);
  entry->atPut(1, info.asSmallInt());
  new_entries->atPut(entries->length(), *entry);

  return *new_entries;
}

RawObject Runtime::layoutAddAttribute(Thread* thread, const Layout& layout,
                                      const Object& name, word flags) {
  HandleScope scope(thread);
  Object iname(&scope, internStr(name));

  // Check if a edge for the attribute addition already exists
  List edges(&scope, layout->additions());
  RawObject result = layoutFollowEdge(edges, iname);
  if (!result->isError()) {
    return result;
  }

  // Create a new layout and figure out where to place the attribute
  Layout new_layout(&scope, layoutCreateChild(thread, layout));
  Tuple inobject(&scope, layout->inObjectAttributes());
  if (inobject->length() < layout->numInObjectAttributes()) {
    AttributeInfo info(inobject->length() * kPointerSize,
                       flags | AttributeInfo::Flag::kInObject);
    new_layout->setInObjectAttributes(
        layoutAddAttributeEntry(thread, inobject, name, info));
  } else {
    Tuple overflow(&scope, layout->overflowAttributes());
    AttributeInfo info(overflow->length(), flags);
    new_layout->setOverflowAttributes(
        layoutAddAttributeEntry(thread, overflow, name, info));
  }

  // Add the edge to the existing layout
  Object value(&scope, *new_layout);
  layoutAddEdge(edges, iname, value);

  return *new_layout;
}

RawObject Runtime::layoutDeleteAttribute(Thread* thread, const Layout& layout,
                                         const Object& name) {
  HandleScope scope(thread);

  // See if the attribute exists
  AttributeInfo info;
  if (!layoutFindAttribute(thread, layout, name, &info)) {
    return Error::object();
  }

  // Check if an edge exists for removing the attribute
  Object iname(&scope, internStr(name));
  List edges(&scope, layout->deletions());
  RawObject next_layout = layoutFollowEdge(edges, iname);
  if (!next_layout->isError()) {
    return next_layout;
  }

  // No edge was found, create a new layout and add an edge
  Layout new_layout(&scope, layoutCreateChild(thread, layout));
  if (info.isInObject()) {
    // The attribute to be deleted was an in-object attribute, mark it as
    // deleted
    Tuple old_inobject(&scope, layout->inObjectAttributes());
    Tuple new_inobject(&scope, newTuple(old_inobject->length()));
    for (word i = 0; i < old_inobject->length(); i++) {
      Tuple entry(&scope, old_inobject->at(i));
      if (entry->at(0) == *iname) {
        entry = newTuple(2);
        entry->atPut(0, NoneType::object());
        entry->atPut(
            1, AttributeInfo(0, AttributeInfo::Flag::kDeleted).asSmallInt());
      }
      new_inobject->atPut(i, *entry);
    }
    new_layout->setInObjectAttributes(*new_inobject);
  } else {
    // The attribute to be deleted was an overflow attribute, omit it from the
    // new overflow array
    Tuple old_overflow(&scope, layout->overflowAttributes());
    Tuple new_overflow(&scope, newTuple(old_overflow->length() - 1));
    bool is_deleted = false;
    for (word i = 0, j = 0; i < old_overflow->length(); i++) {
      Tuple entry(&scope, old_overflow->at(i));
      if (entry->at(0) == *iname) {
        is_deleted = true;
        continue;
      }
      if (is_deleted) {
        // Need to shift everything down by 1 once we've deleted the attribute
        entry = newTuple(2);
        entry->atPut(0, RawTuple::cast(old_overflow->at(i))->at(0));
        entry->atPut(1, AttributeInfo(j, info.flags()).asSmallInt());
      }
      new_overflow->atPut(j, *entry);
      j++;
    }
    new_layout->setOverflowAttributes(*new_overflow);
  }

  // Add the edge to the existing layout
  Object value(&scope, *new_layout);
  layoutAddEdge(edges, iname, value);

  return *new_layout;
}

void Runtime::initializeSuperType() {
  HandleScope scope;
  Type super(&scope, addEmptyBuiltinType(SymbolId::kSuper, LayoutId::kSuper,
                                         LayoutId::kObject));

  typeAddBuiltinFunction(super, SymbolId::kDunderInit,
                         nativeTrampoline<builtinSuperInit>);

  typeAddBuiltinFunction(super, SymbolId::kDunderNew,
                         nativeTrampoline<builtinSuperNew>);
}

RawObject Runtime::superGetAttr(Thread* thread, const Object& receiver,
                                const Object& name) {
  HandleScope scope(thread);
  Super super(&scope, *receiver);
  Type start_type(&scope, super->objectType());
  Tuple mro(&scope, start_type->mro());
  word i;
  for (i = 0; i < mro->length(); i++) {
    if (super->type() == mro->at(i)) {
      // skip super->type (if any)
      i++;
      break;
    }
  }
  for (; i < mro->length(); i++) {
    Type type(&scope, mro->at(i));
    Dict dict(&scope, type->dict());
    Object value_cell(&scope, dictAt(dict, name));
    if (value_cell->isError()) {
      continue;
    }
    Object value(&scope, RawValueCell::cast(*value_cell)->value());
    if (!isNonDataDescriptor(thread, value)) {
      return *value;
    }
    Object self(&scope, NoneType::object());
    if (super->object() != *start_type) {
      self = super->object();
    }
    Object owner(&scope, *start_type);
    return Interpreter::callDescriptorGet(thread, thread->currentFrame(), value,
                                          self, owner);
  }
  // fallback to normal instance getattr
  return instanceGetAttr(thread, receiver, name);
}

void Runtime::freeApiHandles() {
  // Clear the allocated ApiHandles
  HandleScope scope;
  Dict dict(&scope, apiHandles());
  Tuple keys(&scope, dictKeys(dict));
  for (word i = 0; i < keys->length(); i++) {
    Object key(&scope, keys->at(i));
    auto handle =
        static_cast<ApiHandle*>(RawInt::cast(dictAt(dict, key))->asCPtr());
    std::free(handle->cache());
    std::free(handle);
  }
}

RawObject Runtime::lookupSymbolInMro(Thread* thread, const Type& type,
                                     SymbolId symbol) {
  HandleScope scope(thread);
  Tuple mro(&scope, type->mro());
  Object key(&scope, symbols()->at(symbol));
  for (word i = 0; i < mro->length(); i++) {
    Type mro_type(&scope, mro->at(i));
    Dict dict(&scope, mro_type->dict());
    Object value_cell(&scope, dictAt(dict, key));
    if (!value_cell->isError()) {
      return RawValueCell::cast(*value_cell)->value();
    }
  }
  return Error::object();
}

RawObject Runtime::iteratorLengthHint(Thread* thread, const Object& iterator) {
  HandleScope scope(thread);
  Object length_hint_method(
      &scope, Interpreter::lookupMethod(thread, thread->currentFrame(),
                                        iterator, SymbolId::kDunderLengthHint));
  if (length_hint_method->isError()) {
    return *length_hint_method;
  }
  Object result(&scope, Interpreter::callMethod1(thread, thread->currentFrame(),
                                                 length_hint_method, iterator));
  if (result->isError()) {
    return *result;
  }
  if (!result->isSmallInt()) {
    return thread->raiseTypeErrorWithCStr(
        "__length_hint__ returned non-integer value");
  }
  return *result;
}

bool Runtime::isIteratorExhausted(Thread* thread, const Object& iterator) {
  HandleScope scope(thread);
  Object result(&scope, iteratorLengthHint(thread, iterator));
  if (result->isError()) {
    return true;
  }
  return (RawSmallInt::cast(*result)->value() == 0);
}

inline bool Runtime::isAsciiSpace(byte ch) {
  return ((ch >= 0x09) && (ch <= 0x0D)) || ((ch >= 0x1C) && (ch <= 0x1F)) ||
         ch == 0x20;
}

RawObject Runtime::strSubstr(const Str& str, word start, word length) {
  DCHECK(start >= 0, "from should be > 0");
  if (length <= 0) {
    return SmallStr::fromCStr("");
  }
  word str_len = str->length();
  DCHECK(start + length <= str_len, "overflow");
  if (start == 0 && length == str_len) {
    return *str;
  }
  // SmallStr result
  if (length <= RawSmallStr::kMaxLength) {
    byte buffer[RawSmallStr::kMaxLength];
    for (word i = 0; i < length; i++) {
      buffer[i] = str->charAt(start + i);
    }
    return SmallStr::fromBytes(View<byte>(buffer, length));
  }
  // LargeStr result
  HandleScope scope;
  LargeStr source(&scope, *str);
  LargeStr result(&scope, heap()->createLargeStr(length));
  std::memcpy(reinterpret_cast<void*>(result->address()),
              reinterpret_cast<void*>(source->address() + start), length);
  return *result;
}

word Runtime::strSpan(const Str& src, const Str& str) {
  word length = src->length();
  word str_length = str->length();
  word first = 0;
  for (; first < length; first++) {
    bool has_match = false;
    byte ch = src->charAt(first);
    for (word j = 0; j < str_length; j++) {
      if (ch == str->charAt(j)) {
        has_match = true;
        break;
      }
    }
    if (!has_match) {
      break;
    }
  }
  return first;
}

word Runtime::strRSpan(const Str& src, const Str& str, word rend) {
  DCHECK(rend >= 0, "string index underflow");
  word length = src->length();
  word str_length = str->length();
  word result = 0;
  for (word i = length - 1; i >= rend; i--, result++) {
    byte ch = src->charAt(i);
    bool has_match = false;
    for (word j = 0; j < str_length; j++) {
      if (ch == str->charAt(j)) {
        has_match = true;
        break;
      }
    }
    if (!has_match) {
      break;
    }
  }
  return result;
}

RawObject Runtime::strStripSpace(const Str& src,
                                 const StrStripDirection direction) {
  word length = src->length();
  if (length == 0) {
    return *src;
  }
  if (length == 1 && isAsciiSpace(src->charAt(0))) {
    return SmallStr::fromCStr("");
  }

  word first = 0;
  if (direction == StrStripDirection::Left ||
      direction == StrStripDirection::Both) {
    while (first < length && isAsciiSpace(src->charAt(first))) {
      ++first;
    }
  }

  word last = 0;
  if (direction == StrStripDirection::Right ||
      direction == StrStripDirection::Both) {
    for (word i = length - 1; i >= first && isAsciiSpace(src->charAt(i)); i--) {
      last++;
    }
  }
  return strSubstr(src, first, length - first - last);
}

RawObject Runtime::strStrip(const Str& src, const Str& str,
                            StrStripDirection direction) {
  word length = src->length();
  if (length == 0 || str->length() == 0) {
    return *src;
  }
  word first = 0;
  word last = 0;
  // TODO(jeethu): Use set lookup if chars is a LargeStr
  if (direction == StrStripDirection::Left ||
      direction == StrStripDirection::Both) {
    first = strSpan(src, str);
  }

  if (direction == StrStripDirection::Right ||
      direction == StrStripDirection::Both) {
    last = strRSpan(src, str, first);
  }
  return strSubstr(src, first, length - first - last);
}

RawObject Runtime::strIteratorNext(Thread* thread, StrIterator& iter) {
  HandleScope scope(thread);
  word idx = iter.index();
  Str underlying(&scope, iter.str());
  if (idx >= underlying->length()) {
    return Error::object();
  }

  char item = underlying->charAt(idx);
  char buffer[] = {item, '\0'};
  iter.setIndex(idx + 1);
  return RawSmallStr::fromCStr(buffer);
}

RawObject Runtime::intBinaryOr(Thread* thread, const Int& left,
                               const Int& right) {
  word left_digits = left->numDigits();
  word right_digits = right->numDigits();
  if (left_digits <= 1 && right_digits <= 1) {
    return newInt(left->asWord() | right->asWord());
  }

  HandleScope scope(thread);
  Int longer(&scope, left_digits > right_digits ? *left : *right);
  Int shorter(&scope, left_digits <= right_digits ? *left : *right);
  LargeInt result(&scope, heap()->createLargeInt(longer->numDigits()));
  for (word i = 0; i < shorter->numDigits(); i++) {
    result->digitAtPut(i, longer->digitAt(i) | shorter->digitAt(i));
  }
  for (word i = shorter->numDigits(); i < longer->numDigits(); i++) {
    result->digitAtPut(i, longer->digitAt(i));
  }
  return *result;
}

RawObject Runtime::intBinaryLshift(Thread* thread, const Int& num, word shift) {
  DCHECK(shift >= 0, "shift count needs to be non-negative");
  if (shift == 0 || (num->isSmallInt() && num->asWord() == 0)) {
    return *num;
  }
  const word shift_bits = shift % kBitsPerWord;
  const word shift_words = shift / kBitsPerWord;
  const word high_digit = num->digitAt(num->numDigits() - 1);

  // check if high digit overflows when shifted - if we need an extra digit
  word bit_length =
      Utils::highestBit(high_digit >= 0 ? high_digit : ~high_digit);
  bool overflow = bit_length + shift_bits >= kBitsPerWord;

  // check if result fits into one word
  if (num->numDigits() == 1 && (shift_words == 0 && !overflow)) {
    return newInt(high_digit << shift_bits);
  }

  // allocate large int and zero-initialize low digits
  const word num_digits = num->numDigits() + shift_words + overflow;
  HandleScope scope(thread);
  LargeInt result(&scope, heap()->createLargeInt(num_digits));
  for (word i = 0; i < shift_words; i++) {
    result->digitAtPut(i, 0);
  }

  // iterate over digits of num and handle carrying
  const word right_shift = kBitsPerWord - shift_bits;
  uword prev = 0;
  for (word i = 0; i < num->numDigits(); i++) {
    const uword digit = num->digitAt(i);
    result->digitAtPut(i + shift_words,
                       (digit << shift_bits) + (prev >> right_shift));
    prev = digit;
  }
  if (overflow) {
    // signed shift takes cares of keeping the sign
    word overflow_digit = static_cast<word>(prev) >> right_shift;
    result->digitAtPut(num_digits - 1, static_cast<uword>(overflow_digit));
  }
  return *result;
}

}  // namespace python
