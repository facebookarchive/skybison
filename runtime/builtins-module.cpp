#include "builtins-module.h"

#include <cerrno>
#include <cmath>
#include <csignal>

#include "builtins.h"
#include "bytes-builtins.h"
#include "capi.h"
#include "dict-builtins.h"
#include "exception-builtins.h"
#include "formatter.h"
#include "int-builtins.h"
#include "list-builtins.h"
#include "marshal.h"
#include "module-builtins.h"
#include "modules.h"
#include "object-builtins.h"
#include "objects.h"
#include "runtime.h"
#include "str-builtins.h"
#include "tuple-builtins.h"
#include "type-builtins.h"

namespace py {

RawObject delAttribute(Thread* thread, const Object& object,
                       const Object& name) {
  HandleScope scope(thread);
  Object interned(&scope, attributeName(thread, name));
  if (interned.isErrorException()) return *interned;
  Object result(&scope,
                thread->runtime()->attributeDel(thread, object, interned));
  if (result.isErrorException()) return *result;
  return NoneType::object();
}

RawObject getAttribute(Thread* thread, const Object& object,
                       const Object& name) {
  HandleScope scope(thread);
  Object interned(&scope, attributeName(thread, name));
  if (interned.isErrorException()) return *interned;
  return thread->runtime()->attributeAt(thread, object, interned);
}

RawObject hasAttribute(Thread* thread, const Object& object,
                       const Object& name) {
  HandleScope scope(thread);
  Object interned(&scope, attributeName(thread, name));
  if (interned.isErrorException()) return *interned;

  Object result(&scope,
                thread->runtime()->attributeAt(thread, object, interned));
  if (!result.isErrorException()) {
    return Bool::trueObj();
  }
  if (!thread->pendingExceptionMatches(LayoutId::kAttributeError)) {
    return *result;
  }
  thread->clearPendingException();
  return Bool::falseObj();
}

RawObject setAttribute(Thread* thread, const Object& object, const Object& name,
                       const Object& value) {
  HandleScope scope(thread);
  Object interned(&scope, attributeName(thread, name));
  if (interned.isErrorException()) return *interned;

  Object result(
      &scope, thread->invokeMethod3(object, ID(__setattr__), interned, value));
  if (result.isErrorException()) return *result;
  return NoneType::object();
}

ALIGN_16 bool FUNC(builtins, abs_intrinsic)(Thread* thread) {
  RawObject obj = thread->stackTop();
  if (obj.isSmallInt()) {
    thread->stackPop();
    word value = SmallInt::cast(obj).value();
    if (value < 0) {
      obj = SmallInt::fromWord(-value);
    }
    thread->stackSetTop(obj);
    return true;
  }
  if (obj.isFloat()) {
    thread->stackPop();
    double value = Float::cast(obj).value();
    thread->stackSetTop(thread->runtime()->newFloat(std::fabs(value)));
    return true;
  }
  return false;
}

ALIGN_16 bool FUNC(builtins, _index_intrinsic)(Thread* thread) {
  RawObject value = thread->stackTop();
  if (thread->runtime()->isInstanceOfInt(value)) {
    thread->stackPop();
    thread->stackSetTop(value);
    return true;
  }
  return false;
}

ALIGN_16 bool FUNC(builtins, next_intrinsic)(Thread* thread) {
  RawObject value = thread->stackTop();
  switch (value.layoutId()) {
    case LayoutId::kTupleIterator: {
      HandleScope scope(thread);
      TupleIterator tuple_iterator(&scope, value);
      RawObject result = tupleIteratorNext(thread, tuple_iterator);
      if (result.isErrorNoMoreItems()) {
        return false;
      }
      thread->stackPop();
      thread->stackSetTop(result);
      return true;
    }
    case LayoutId::kListIterator: {
      HandleScope scope(thread);
      ListIterator list_iterator(&scope, value);
      RawObject result = listIteratorNext(thread, list_iterator);
      if (result.isErrorOutOfBounds()) {
        return false;
      }
      thread->stackPop();
      thread->stackSetTop(result);
      return true;
    }
    default: {
      return false;
    }
  }
}

ALIGN_16 bool FUNC(builtins, _number_check_intrinsic)(Thread* thread) {
  Runtime* runtime = thread->runtime();
  RawObject arg = thread->stackTop();
  if (runtime->isInstanceOfInt(arg) || runtime->isInstanceOfFloat(arg)) {
    thread->stackPop();
    thread->stackSetTop(Bool::trueObj());
    return true;
  }
  return false;
}

ALIGN_16 bool FUNC(builtins, _slice_index_intrinsic)(Thread* thread) {
  RawObject value = thread->stackPeek(0);
  if (value.isNoneType() || thread->runtime()->isInstanceOfInt(value)) {
    thread->stackPop();
    thread->stackSetTop(value);
    return true;
  }
  return false;
}

ALIGN_16 bool FUNC(builtins, _slice_index_not_none_intrinsic)(Thread* thread) {
  RawObject value = thread->stackTop();
  if (thread->runtime()->isInstanceOfInt(value)) {
    thread->stackPop();
    thread->stackSetTop(value);
    return true;
  }
  return false;
}

ALIGN_16 bool FUNC(builtins, isinstance_intrinsic)(Thread* thread) {
  if (thread->runtime()->typeOf(thread->stackPeek(1)) == thread->stackPeek(0)) {
    thread->stackDrop(2);
    thread->stackSetTop(Bool::trueObj());
    return true;
  }
  return false;
}

ALIGN_16 bool FUNC(builtins, len_intrinsic)(Thread* thread) {
  RawObject arg = thread->stackTop();
  word length;
  switch (arg.layoutId()) {
    case LayoutId::kBytearray:
      length = Bytearray::cast(arg).numItems();
      break;
    case LayoutId::kDict:
      length = Dict::cast(arg).numItems();
      break;
    case LayoutId::kFrozenSet:
      length = FrozenSet::cast(arg).numItems();
      break;
    case LayoutId::kLargeBytes:
      length = LargeBytes::cast(arg).length();
      break;
    case LayoutId::kLargeStr:
      length = LargeStr::cast(arg).codePointLength();
      break;
    case LayoutId::kList:
      length = List::cast(arg).numItems();
      break;
    case LayoutId::kSet:
      length = Set::cast(arg).numItems();
      break;
    case LayoutId::kSmallBytes:
      length = SmallBytes::cast(arg).length();
      break;
    case LayoutId::kSmallStr:
      length = SmallStr::cast(arg).codePointLength();
      break;
    case LayoutId::kTuple:
      length = Tuple::cast(arg).length();
      break;
    default:
      return false;
  }
  thread->stackPop();
  thread->stackSetTop(SmallInt::fromWord(length));
  return true;
}

void FUNC(builtins, __init_module__)(Thread* thread, const Module& module,
                                     View<byte> bytecode) {
  Runtime* runtime = thread->runtime();
  runtime->setBuiltinsModuleId(module.id());
  runtime->cacheBuildClass(thread, module);
  HandleScope scope(thread);

  // Add module variables
  {
    Object dunder_debug(&scope, Bool::falseObj());
    moduleAtPutById(thread, module, ID(__debug__), dunder_debug);

    Object false_obj(&scope, Bool::falseObj());
    moduleAtPutById(thread, module, ID(False), false_obj);

    Object none(&scope, NoneType::object());
    moduleAtPutById(thread, module, ID(None), none);

    Object not_implemented(&scope, NotImplementedType::object());
    moduleAtPutById(thread, module, ID(NotImplemented), not_implemented);

    Object true_obj(&scope, Bool::trueObj());
    moduleAtPutById(thread, module, ID(True), true_obj);
  }

  executeFrozenModule(thread, module, bytecode);
}

static RawObject calculateMetaclass(Thread* thread, const Type& metaclass_type,
                                    const Tuple& bases) {
  HandleScope scope(thread);
  Type result(&scope, *metaclass_type);
  Runtime* runtime = thread->runtime();
  for (word i = 0, num_bases = bases.length(); i < num_bases; i++) {
    Type base_type(&scope, runtime->typeOf(bases.at(i)));
    if (typeIsSubclass(base_type, result)) {
      result = *base_type;
    } else if (!typeIsSubclass(result, base_type)) {
      return thread->raiseWithFmt(
          LayoutId::kTypeError,
          "metaclass conflict: the metaclass of a derived class must be a "
          "(non-strict) subclass of the metaclasses of all its bases");
    }
  }
  return *result;
}

RawObject FUNC(builtins, bin)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object number(&scope, args.get(0));
  number = intFromIndex(thread, number);
  if (number.isError()) {
    return *number;
  }
  Int number_int(&scope, intUnderlying(*number));
  return formatIntBinarySimple(thread, number_int);
}

RawObject FUNC(builtins, delattr)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Object name(&scope, args.get(1));
  Object result(&scope, delAttribute(thread, self, name));
  return *result;
}

static RawObject replaceNonTypeBases(Thread* thread, const Tuple& bases) {
  Runtime* runtime = thread->runtime();
  word num_bases = bases.length();
  bool has_nontype_base = false;
  for (word i = 0; i < num_bases; i++) {
    if (!runtime->isInstanceOfType(bases.at(i))) {
      has_nontype_base = true;
      break;
    }
  }
  if (!has_nontype_base) {
    return *bases;
  }
  HandleScope scope(thread);
  List new_bases(&scope, runtime->newList());
  Object base(&scope, NoneType::object());
  Object replacements(&scope, NoneType::object());
  Tuple entries(&scope, runtime->emptyTuple());
  for (word i = 0; i < num_bases; i++) {
    base = bases.at(i);
    if (runtime->isInstanceOfType(*base)) {
      runtime->listAdd(thread, new_bases, base);
      continue;
    }
    replacements = thread->invokeMethod2(base, ID(__mro_entries__), bases);
    if (replacements.isErrorException()) return *replacements;
    if (replacements.isErrorNotFound()) {
      runtime->listAdd(thread, new_bases, base);
      continue;
    }
    if (!replacements.isTuple()) {
      return thread->raiseWithFmt(LayoutId::kTypeError,
                                  "__mro_entries__ must return a tuple");
    }
    entries = *replacements;
    listExtend(thread, new_bases, entries, entries.length());
  }
  Tuple new_bases_items(&scope, new_bases.items());
  return runtime->tupleSubseq(thread, new_bases_items, 0, new_bases.numItems());
}

static void pickBuiltinTypeCtorFunction(Thread* thread, const Type& type) {
  HandleScope scope(thread);
  Object ctor(&scope, NoneType::object());
  LayoutId layout_id = type.instanceLayoutId();
  Runtime* runtime = thread->runtime();
  switch (layout_id) {
    case LayoutId::kList: {
      Module under_builtins(&scope, runtime->findModuleById(ID(_builtins)));
      ctor = moduleAtById(thread, under_builtins, ID(_list_ctor));
      break;
    }
    case LayoutId::kStr: {
      Module under_builtins(&scope, runtime->findModuleById(ID(_builtins)));
      ctor = moduleAtById(thread, under_builtins, ID(_str_ctor));
      break;
    }
    case LayoutId::kStopIteration: {
      Module under_builtins(&scope, runtime->findModuleById(ID(_builtins)));
      ctor = moduleAtById(thread, under_builtins, ID(_stop_iteration_ctor));
      break;
    }
    case LayoutId::kStrArray: {
      Module under_builtins(&scope, runtime->findModuleById(ID(_builtins)));
      ctor = moduleAtById(thread, under_builtins, ID(_str_array_ctor));
      break;
    }
    default: {
      if (typeAtById(thread, type, ID(__init__)).isErrorNotFound()) {
        // Use __new__ as _ctor if __init__ is undefined.
        Object dunder_new(&scope, typeAtById(thread, type, ID(__new__)));
        if (!dunder_new.isErrorNotFound()) {
          ctor = StaticMethod::cast(*dunder_new).function();
        }
      }
    }
  }
  if (ctor.isNoneType()) {
    ctor = runtime->lookupNameInModule(thread, ID(_builtins),
                                       ID(_type_dunder_call));
  }
  CHECK(ctor.isFunction(), "ctor is expected to be a function");
  type.setCtor(*ctor);
}

RawObject FUNC(builtins, __build_class__)(Thread* thread, Arguments args) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Object body_obj(&scope, args.get(0));
  if (!body_obj.isFunction()) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "__build_class__: func must be a function");
  }
  Function body(&scope, *body_obj);
  Object name(&scope, args.get(1));
  if (!runtime->isInstanceOfStr(*name)) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "__build_class__: name is not a string");
  }
  Object metaclass(&scope, args.get(2));
  Object bootstrap(&scope, args.get(3));
  Tuple orig_bases(&scope, args.get(4));
  Tuple bases(&scope, *orig_bases);
  Dict kwargs(&scope, args.get(5));

  if (bootstrap == Bool::trueObj()) {
    CHECK(name.isStr(), "bootstrap class names must not be str subclass");
    name = Runtime::internStr(thread, name);
    Object type_obj(&scope, findBuiltinTypeWithName(thread, name));
    CHECK(!type_obj.isErrorNotFound(), "Unknown builtin type");
    Type type(&scope, *type_obj);

    if (bases.length() == 0 && name != runtime->symbols()->at(ID(object))) {
      bases = runtime->implicitBases();
    }
    Tuple builtin_bases(&scope, type.bases());
    word bases_length = bases.length();
    CHECK(builtin_bases.length() == bases_length, "mismatching bases for '%s'",
          Str::cast(*name).toCStr());
    for (word i = 0; i < bases_length; i++) {
      CHECK(builtin_bases.at(i) == bases.at(i), "mismatching bases for '%s'",
            Str::cast(*name).toCStr());
    }

    if (type.mro().isNoneType()) {
      Type superclass(&scope, bases.at(0));
      DCHECK(!superclass.mro().isNoneType(), "superclass not initialized yet");
      Tuple superclass_mro(&scope, superclass.mro());
      word mro_length = superclass_mro.length() + 1;
      MutableTuple mro(&scope, runtime->newMutableTuple(mro_length));
      mro.atPut(0, *type);
      mro.replaceFromWith(1, *superclass_mro, mro_length - 1);
      type.setMro(mro.becomeImmutable());
    }

    Dict type_dict(&scope, runtime->newDict());
    Object result(&scope,
                  thread->callFunctionWithImplicitGlobals(body, type_dict));
    if (result.isError()) return *result;
    CHECK(!typeAssignFromDict(thread, type, type_dict).isErrorException(),
          "error while assigning bootstrap type dict");
    // TODO(T53997177): Centralize type initialization
    Object module_name(&scope, typeAtById(thread, type, ID(__module__)));
    // non-heap-types in CPython have no `__module__` unless there is a
    // "." in `tp_name`. Remove the attribute when it equals "builtins".
    if (module_name.isStr() &&
        Str::cast(*module_name).equals(runtime->symbols()->at(ID(builtins)))) {
      typeRemoveById(thread, type, ID(__module__));
    }

    Object qualname(&scope, NoneType::object());
    if (type.instanceLayoutId() == LayoutId::kType) {
      qualname = *name;
      // Note: `type` is the only type allowed to have a descriptor instead of
      // a string for `__qualname__`.
    } else {
      qualname = typeRemoveById(thread, type, ID(__qualname__));
      DCHECK(qualname.isStr() && Str::cast(*qualname).equals(*name),
             "unexpected __qualname__ attribute");
    }
    type.setQualname(*qualname);
    typeAddDocstring(thread, type);

    if (DCHECK_IS_ON()) {
      Object dunder_new(&scope, typeAtById(thread, type, ID(__new__)));
      if (!dunder_new.isStaticMethod()) {
        if (!(dunder_new.isNoneType() || dunder_new.isErrorNotFound())) {
          DCHECK(false, "__new__ for %s should be a staticmethod",
                 Str::cast(*name).toCStr());
        }
      }
    }

    pickBuiltinTypeCtorFunction(thread, type);
    runtime->builtinTypeCreated(thread, type);
    return *type;
  }

  Object updated_bases(&scope, replaceNonTypeBases(thread, bases));
  if (updated_bases.isErrorException()) {
    return *updated_bases;
  }
  bases = *updated_bases;
  bool metaclass_is_class;
  if (metaclass.isUnbound()) {
    metaclass_is_class = true;
    if (bases.length() == 0) {
      metaclass = runtime->typeAt(LayoutId::kType);
    } else {
      metaclass = runtime->typeOf(bases.at(0));
    }
  } else {
    metaclass_is_class = runtime->isInstanceOfType(*metaclass);
  }

  if (metaclass_is_class) {
    Type metaclass_type(&scope, *metaclass);
    metaclass = calculateMetaclass(thread, metaclass_type, bases);
    if (metaclass.isError()) return *metaclass;
  }

  Object dict_obj(&scope, NoneType::object());
  Object prepare_method(
      &scope, runtime->attributeAtById(thread, metaclass, ID(__prepare__)));
  if (prepare_method.isError()) {
    Object given(&scope, thread->pendingExceptionType());
    Object exc(&scope, runtime->typeAt(LayoutId::kAttributeError));
    if (!givenExceptionMatches(thread, given, exc)) {
      return *prepare_method;
    }
    thread->clearPendingException();
    dict_obj = runtime->newDict();
  } else {
    thread->stackPush(*prepare_method);
    Tuple pargs(&scope, runtime->newTupleWith2(name, bases));
    thread->stackPush(*pargs);
    thread->stackPush(*kwargs);
    dict_obj = Interpreter::callEx(thread, CallFunctionExFlag::VAR_KEYWORDS);
    if (dict_obj.isError()) return *dict_obj;
  }
  if (!runtime->isMapping(thread, dict_obj)) {
    if (metaclass_is_class) {
      Type metaclass_type(&scope, *metaclass);
      Str metaclass_type_name(&scope, metaclass_type.name());
      return thread->raiseWithFmt(
          LayoutId::kTypeError,
          "%S.__prepare__() must return a mapping, not %T",
          &metaclass_type_name, &dict_obj);
    }
    return thread->raiseWithFmt(
        LayoutId::kTypeError,
        "<metaclass>.__prepare__() must return a mapping, not %T", &dict_obj);
  }
  Dict type_dict(&scope, *dict_obj);

  // TODO(cshapiro): might need to do some kind of callback here and we want
  // backtraces to work correctly.  The key to doing that would be to put some
  // state on the stack in between the the incoming arguments from the builtin
  // caller and the on-stack state for the class body function call.
  Object body_result(&scope,
                     thread->callFunctionWithImplicitGlobals(body, type_dict));
  if (body_result.isError()) return *body_result;

  if (bases != orig_bases) {
    dictAtPutById(thread, type_dict, ID(__orig_bases__), orig_bases);
  }

  thread->stackPush(*metaclass);
  Tuple pargs(&scope, runtime->newTupleWith3(name, bases, type_dict));
  thread->stackPush(*pargs);
  thread->stackPush(*kwargs);
  return Interpreter::callEx(thread, CallFunctionExFlag::VAR_KEYWORDS);
}

RawObject FUNC(builtins, callable)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object arg(&scope, args.get(0));
  return Bool::fromBool(thread->runtime()->isCallable(thread, arg));
}

RawObject FUNC(builtins, chr)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object arg(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfInt(*arg)) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "an integer is required (got type %T)", &arg);
  }
  Int num(&scope, intUnderlying(*arg));
  if (!num.isSmallInt()) {
    return thread->raiseWithFmt(LayoutId::kOverflowError,
                                "Python int too large to convert to C long");
  }
  word code_point = num.asWord();
  if (code_point < 0 || code_point > kMaxUnicode) {
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "chr() arg not in range(0x110000)");
  }
  return SmallStr::fromCodePoint(static_cast<int32_t>(code_point));
}

RawObject compile(Thread* thread, const Object& source, const Object& filename,
                  SymbolId mode, word flags, int optimize) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object mode_str(&scope, runtime->symbols()->at(mode));
  Object flags_int(&scope, runtime->newInt(flags));
  Object optimize_int(&scope, SmallInt::fromWord(optimize));

  Object dunder_import(&scope, runtime->lookupNameInModule(thread, ID(builtins),
                                                           ID(__import__)));
  if (dunder_import.isErrorException()) return *dunder_import;
  Object compiler_name(&scope, runtime->symbols()->at(ID(compiler)));
  Object import_result(
      &scope, Interpreter::call1(thread, dunder_import, compiler_name));
  if (import_result.isErrorException()) return *import_result;
  Object none(&scope, NoneType::object());
  return thread->invokeFunction6(ID(compiler), ID(compile), source, filename,
                                 mode_str, flags_int, none, optimize_int);
}

RawObject FUNC(builtins, id)(Thread* thread, Arguments args) {
  // NOTE: This pins a handle until the runtime exits.
  // TODO(emacs): Either determine that this function is used so little that it
  // does not matter or add a section to the GC to clean up handles created by
  // id().
  return thread->runtime()->newIntFromCPtr(
      objectNewReference(thread, args.get(0)));
}

RawObject FUNC(builtins, oct)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object number(&scope, args.get(0));
  number = intFromIndex(thread, number);
  if (number.isError()) {
    return *number;
  }
  Int number_int(&scope, intUnderlying(*number));
  return formatIntOctalSimple(thread, number_int);
}

RawObject FUNC(builtins, ord)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (runtime->isInstanceOfBytes(*obj)) {
    Bytes bytes(&scope, bytesUnderlying(*obj));
    if (bytes.length() == 1) {
      int32_t code_point = bytes.byteAt(0);
      return SmallInt::fromWord(code_point);
    }
  } else if (runtime->isInstanceOfStr(*obj)) {
    Str str(&scope, strUnderlying(*obj));
    if (str.isSmallStr() && *str != Str::empty()) {
      word num_bytes;
      int32_t code_point = str.codePointAt(0, &num_bytes);
      if (num_bytes == str.length()) {
        return SmallInt::fromWord(code_point);
      }
    }
  } else if (runtime->isInstanceOfBytearray(*obj)) {
    Bytearray byte_array(&scope, *obj);
    if (byte_array.numItems() == 1) {
      int32_t code_point = byte_array.byteAt(0);
      return SmallInt::fromWord(code_point);
    }
  } else {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "Unsupported type in builtin 'ord'");
  }
  return thread->raiseWithFmt(LayoutId::kTypeError,
                              "Builtin 'ord' expects string of length 1");
}

RawObject FUNC(builtins, __import__)(Thread* thread, Arguments args) {
  // Note that this is a simplified __import__ implementation that is used
  // during early bootstrap; it is replaced by importlib.__import__ once
  // import lib is fully initialized.
  HandleScope scope(thread);
  Str name(&scope, args.get(0));
  name = Runtime::internStr(thread, name);
  // We ignore arg1, arg2, arg3.
  DCHECK(args.get(4) == SmallInt::fromWord(0), "only supports level=0");
  Runtime* runtime = thread->runtime();
  Object module(&scope, ensureBuiltinModule(thread, name));
  if (module.isErrorNotFound() || !runtime->isInstanceOfModule(*module)) {
    return thread->raiseWithFmt(LayoutId::kImportError,
                                "failed to import %S (bootstrap importer)",
                                &name);
  }
  return *module;
}

RawObject FUNC(builtins, _debug_break)(Thread*, Arguments) {
#if __has_builtin(__builtin_debugtrap)
  __builtin_debugtrap();
#elif defined(__i386__) || defined(__x86_64__)
  __asm__ volatile("int $0x03");
#else
  std::raise(SIGTRAP);
#endif
  return NoneType::object();
}

// TODO(T39322942): Turn this into the Range constructor (__init__ or __new__)
RawObject FUNC(builtins, getattr)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Object name(&scope, args.get(1));
  Object default_obj(&scope, args.get(2));
  Object result(&scope, getAttribute(thread, self, name));
  Runtime* runtime = thread->runtime();
  if (result.isError() && !default_obj.isUnbound()) {
    Type given(&scope, thread->pendingExceptionType());
    Type exc(&scope, runtime->typeAt(LayoutId::kAttributeError));
    if (givenExceptionMatches(thread, given, exc)) {
      thread->clearPendingException();
      result = *default_obj;
    }
  }
  return *result;
}

RawObject FUNC(builtins, hasattr)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Object name(&scope, args.get(1));
  return hasAttribute(thread, self, name);
}

RawObject FUNC(builtins, hash)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object object(&scope, args.get(0));
  return Interpreter::hash(thread, object);
}

RawObject FUNC(builtins, hex)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object number(&scope, args.get(0));
  number = intFromIndex(thread, number);
  if (number.isError()) {
    return *number;
  }
  Int number_int(&scope, intUnderlying(*number));
  return formatIntHexadecimalSimple(thread, number_int);
}

RawObject FUNC(builtins, setattr)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Object name(&scope, args.get(1));
  Object value(&scope, args.get(2));
  return setAttribute(thread, self, name, value);
}

RawObject METH(method, __eq__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  if (!self_obj.isBoundMethod()) {
    return thread->raiseWithFmt(
        LayoutId::kTypeError,
        "'__eq__' requires a 'method' object but received a '%T'", &self_obj);
  }
  Object other_obj(&scope, args.get(1));
  if (!other_obj.isBoundMethod()) {
    return NotImplementedType::object();
  }
  BoundMethod self(&scope, *self_obj);
  BoundMethod other(&scope, *other_obj);
  Bool func_eq(
      &scope, Runtime::objectEquals(thread, self.function(), other.function()));
  if (func_eq.isError()) return *func_eq;
  if (*func_eq == Bool::falseObj()) return Bool::falseObj();
  Bool self_eq(&scope,
               Runtime::objectEquals(thread, self.self(), other.self()));
  if (self_eq.isError()) return *self_eq;
  if (*self_eq == Bool::falseObj()) return Bool::falseObj();
  return Bool::trueObj();
}

}  // namespace py
