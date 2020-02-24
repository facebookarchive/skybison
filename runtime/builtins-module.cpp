#include "builtins-module.h"

#include <cerrno>

#include "builtins.h"
#include "bytes-builtins.h"
#include "capi-handles.h"
#include "exception-builtins.h"
#include "formatter.h"
#include "frozen-modules.h"
#include "int-builtins.h"
#include "marshal.h"
#include "module-builtins.h"
#include "modules.h"
#include "object-builtins.h"
#include "objects.h"
#include "runtime.h"
#include "str-builtins.h"
#include "type-builtins.h"
#include "under-builtins-module.h"

namespace py {

RawObject delAttribute(Thread* thread, const Object& self,
                       const Object& name_obj) {
  HandleScope scope(thread);
  Object name(&scope, attributeName(thread, name_obj));
  if (name.isErrorException()) return *name;
  Object result(&scope, thread->runtime()->attributeDel(thread, self, name));
  if (result.isErrorException()) return *result;
  return NoneType::object();
}

RawObject getAttribute(Thread* thread, const Object& self,
                       const Object& name_obj) {
  HandleScope scope(thread);
  Object name(&scope, attributeName(thread, name_obj));
  if (name.isErrorException()) return *name;
  return thread->runtime()->attributeAt(thread, self, name);
}

RawObject hasAttribute(Thread* thread, const Object& self,
                       const Object& name_obj) {
  HandleScope scope(thread);
  Object name(&scope, attributeName(thread, name_obj));
  if (name.isErrorException()) return *name;

  Object result(&scope, thread->runtime()->attributeAt(thread, self, name));
  if (!result.isErrorException()) {
    return Bool::trueObj();
  }
  if (!thread->pendingExceptionMatches(LayoutId::kAttributeError)) {
    return *result;
  }
  thread->clearPendingException();
  return Bool::falseObj();
}

RawObject setAttribute(Thread* thread, const Object& self,
                       const Object& name_obj, const Object& value) {
  HandleScope scope(thread);
  Object name(&scope, attributeName(thread, name_obj));
  if (name.isErrorException()) return *name;

  Object result(&scope,
                thread->invokeMethod3(self, ID(__setattr__), name, value));
  if (result.isErrorException()) return *result;
  return NoneType::object();
}

const BuiltinType BuiltinsModule::kBuiltinTypes[] = {
    {ID(ArithmeticError), LayoutId::kArithmeticError},
    {ID(AssertionError), LayoutId::kAssertionError},
    {ID(AttributeError), LayoutId::kAttributeError},
    {ID(BaseException), LayoutId::kBaseException},
    {ID(BlockingIOError), LayoutId::kBlockingIOError},
    {ID(BrokenPipeError), LayoutId::kBrokenPipeError},
    {ID(BufferError), LayoutId::kBufferError},
    {ID(BytesWarning), LayoutId::kBytesWarning},
    {ID(ChildProcessError), LayoutId::kChildProcessError},
    {ID(ConnectionAbortedError), LayoutId::kConnectionAbortedError},
    {ID(ConnectionError), LayoutId::kConnectionError},
    {ID(ConnectionRefusedError), LayoutId::kConnectionRefusedError},
    {ID(ConnectionResetError), LayoutId::kConnectionResetError},
    {ID(DeprecationWarning), LayoutId::kDeprecationWarning},
    {ID(EOFError), LayoutId::kEOFError},
    {ID(Exception), LayoutId::kException},
    {ID(FileExistsError), LayoutId::kFileExistsError},
    {ID(FileNotFoundError), LayoutId::kFileNotFoundError},
    {ID(FloatingPointError), LayoutId::kFloatingPointError},
    {ID(FutureWarning), LayoutId::kFutureWarning},
    {ID(GeneratorExit), LayoutId::kGeneratorExit},
    {ID(ImportError), LayoutId::kImportError},
    {ID(ImportWarning), LayoutId::kImportWarning},
    {ID(IndentationError), LayoutId::kIndentationError},
    {ID(IndexError), LayoutId::kIndexError},
    {ID(InterruptedError), LayoutId::kInterruptedError},
    {ID(IsADirectoryError), LayoutId::kIsADirectoryError},
    {ID(KeyError), LayoutId::kKeyError},
    {ID(KeyboardInterrupt), LayoutId::kKeyboardInterrupt},
    {ID(LookupError), LayoutId::kLookupError},
    {ID(MemoryError), LayoutId::kMemoryError},
    {ID(ModuleNotFoundError), LayoutId::kModuleNotFoundError},
    {ID(NameError), LayoutId::kNameError},
    {ID(NoneType), LayoutId::kNoneType},
    {ID(NotADirectoryError), LayoutId::kNotADirectoryError},
    {ID(NotImplementedError), LayoutId::kNotImplementedError},
    {ID(NotImplementedType), LayoutId::kNotImplementedType},
    {ID(OSError), LayoutId::kOSError},
    {ID(OverflowError), LayoutId::kOverflowError},
    {ID(PendingDeprecationWarning), LayoutId::kPendingDeprecationWarning},
    {ID(PermissionError), LayoutId::kPermissionError},
    {ID(ProcessLookupError), LayoutId::kProcessLookupError},
    {ID(RecursionError), LayoutId::kRecursionError},
    {ID(ReferenceError), LayoutId::kReferenceError},
    {ID(ResourceWarning), LayoutId::kResourceWarning},
    {ID(RuntimeError), LayoutId::kRuntimeError},
    {ID(RuntimeWarning), LayoutId::kRuntimeWarning},
    {ID(StopAsyncIteration), LayoutId::kStopAsyncIteration},
    {ID(StopIteration), LayoutId::kStopIteration},
    {ID(SyntaxError), LayoutId::kSyntaxError},
    {ID(SyntaxWarning), LayoutId::kSyntaxWarning},
    {ID(SystemError), LayoutId::kSystemError},
    {ID(SystemExit), LayoutId::kSystemExit},
    {ID(TabError), LayoutId::kTabError},
    {ID(TimeoutError), LayoutId::kTimeoutError},
    {ID(TypeError), LayoutId::kTypeError},
    {ID(UnboundLocalError), LayoutId::kUnboundLocalError},
    {ID(UnicodeDecodeError), LayoutId::kUnicodeDecodeError},
    {ID(UnicodeEncodeError), LayoutId::kUnicodeEncodeError},
    {ID(UnicodeError), LayoutId::kUnicodeError},
    {ID(UnicodeTranslateError), LayoutId::kUnicodeTranslateError},
    {ID(UnicodeWarning), LayoutId::kUnicodeWarning},
    {ID(UserWarning), LayoutId::kUserWarning},
    {ID(ValueError), LayoutId::kValueError},
    {ID(Warning), LayoutId::kWarning},
    {ID(ZeroDivisionError), LayoutId::kZeroDivisionError},
    {ID(_strarray), LayoutId::kStrArray},
    {ID(_traceback), LayoutId::kTraceback},
    {ID(bool), LayoutId::kBool},
    {ID(bytearray), LayoutId::kByteArray},
    {ID(bytearray_iterator), LayoutId::kByteArrayIterator},
    {ID(bytes), LayoutId::kBytes},
    {ID(bytes_iterator), LayoutId::kBytesIterator},
    {ID(classmethod), LayoutId::kClassMethod},
    {ID(code), LayoutId::kCode},
    {ID(complex), LayoutId::kComplex},
    {ID(coroutine), LayoutId::kCoroutine},
    {ID(dict), LayoutId::kDict},
    {ID(dict_itemiterator), LayoutId::kDictItemIterator},
    {ID(dict_items), LayoutId::kDictItems},
    {ID(dict_keyiterator), LayoutId::kDictKeyIterator},
    {ID(dict_keys), LayoutId::kDictKeys},
    {ID(dict_valueiterator), LayoutId::kDictValueIterator},
    {ID(dict_values), LayoutId::kDictValues},
    {ID(ellipsis), LayoutId::kEllipsis},
    {ID(float), LayoutId::kFloat},
    {ID(frozenset), LayoutId::kFrozenSet},
    {ID(function), LayoutId::kFunction},
    {ID(generator), LayoutId::kGenerator},
    {ID(int), LayoutId::kInt},
    {ID(iterator), LayoutId::kSeqIterator},
    {ID(list), LayoutId::kList},
    {ID(list_iterator), LayoutId::kListIterator},
    {ID(longrange_iterator), LayoutId::kLongRangeIterator},
    {ID(mappingproxy), LayoutId::kMappingProxy},
    {ID(memoryview), LayoutId::kMemoryView},
    {ID(method), LayoutId::kBoundMethod},
    {ID(module), LayoutId::kModule},
    {ID(module_proxy), LayoutId::kModuleProxy},
    {ID(object), LayoutId::kObject},
    {ID(property), LayoutId::kProperty},
    {ID(range), LayoutId::kRange},
    {ID(range_iterator), LayoutId::kRangeIterator},
    {ID(set), LayoutId::kSet},
    {ID(set_iterator), LayoutId::kSetIterator},
    {ID(slice), LayoutId::kSlice},
    {ID(staticmethod), LayoutId::kStaticMethod},
    {ID(str), LayoutId::kStr},
    {ID(str_iterator), LayoutId::kStrIterator},
    {ID(super), LayoutId::kSuper},
    {ID(tuple), LayoutId::kTuple},
    {ID(tuple_iterator), LayoutId::kTupleIterator},
    {ID(type), LayoutId::kType},
    {ID(type_proxy), LayoutId::kTypeProxy},
    {SymbolId::kSentinelId, LayoutId::kSentinelId},
};

const SymbolId BuiltinsModule::kIntrinsicIds[] = {
    ID(_index),
    ID(_number_check),
    ID(_slice_index),
    ID(_slice_index_not_none),
    ID(isinstance),
    ID(len),
    SymbolId::kSentinelId,
};

void BuiltinsModule::initialize(Thread* thread, const Module& module) {
  moduleAddBuiltinTypes(thread, module, kBuiltinTypes);

  Runtime* runtime = thread->runtime();
  runtime->cacheBuildClass(thread, module);
  runtime->cacheDunderImport(thread, module);
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

  executeFrozenModule(thread, &kBuiltinsModuleData, module);
  runtime->cacheBuiltinsInstances(thread);

  // Populate some builtin types with shortcut constructors.
  {
    Module under_builtins(&scope, runtime->findModuleById(ID(_builtins)));
    Type stop_iteration_type(&scope, runtime->typeAt(LayoutId::kStopIteration));
    Object ctor(&scope,
                moduleAtById(thread, under_builtins, ID(_stop_iteration_ctor)));
    CHECK(ctor.isFunction(), "_stop_iteration_ctor should be a function");
    stop_iteration_type.setCtor(*ctor);

    Type strarray_type(&scope, runtime->typeAt(LayoutId::kStrArray));
    ctor = moduleAtById(thread, under_builtins, ID(_strarray_ctor));
    CHECK(ctor.isFunction(), "_strarray_ctor should be a function");
    strarray_type.setCtor(*ctor);
  }

  // Mark functions that have an intrinsic implementation.
  for (word i = 0; kIntrinsicIds[i] != SymbolId::kSentinelId; i++) {
    SymbolId intrinsic_id = kIntrinsicIds[i];
    Function::cast(moduleAtById(thread, module, intrinsic_id))
        .setIntrinsicId(static_cast<word>(intrinsic_id));
  }
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

RawObject FUNC(builtins, bin)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object number(&scope, args.get(0));
  number = intFromIndex(thread, number);
  if (number.isError()) {
    return *number;
  }
  Int number_int(&scope, intUnderlying(*number));
  return formatIntBinarySimple(thread, number_int);
}

RawObject FUNC(builtins, delattr)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Object name(&scope, args.get(1));
  Object result(&scope, delAttribute(thread, self, name));
  return *result;
}

RawObject FUNC(builtins, __build_class__)(Thread* thread, Frame* frame,
                                          word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Arguments args(frame, nargs);
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
  Tuple bases(&scope, args.get(4));
  Dict kwargs(&scope, args.get(5));

  if (bootstrap == Bool::trueObj()) {
    CHECK(name.isStr(), "bootstrap class names must not be str subclass");

    // A bootstrap class initialization uses the existing class dictionary.
    CHECK(frame->previousFrame() != nullptr, "must have a caller frame");
    Module module(&scope, frame->previousFrame()->function().moduleObject());
    Object type_obj(&scope, moduleAt(thread, module, name));
    CHECK(type_obj.isType(),
          "Name '%s' is not bound to a type object. "
          "You may need to add it to the builtins module.",
          Str::cast(*name).toCStr());
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

    Dict type_dict(&scope, runtime->newDict());
    Object result(&scope, thread->runClassFunction(body, type_dict));
    if (result.isError()) return *result;
    CHECK(!typeAssignFromDict(thread, type, type_dict).isErrorException(),
          "error while assigning bootstrap type dict");
    // TODO(T53997177): Centralize type initialization
    typeAddDocstring(thread, type);
    // A bootstrap type initialization is complete at this point.

    Object ctor(&scope, NoneType::object());
    // Use __new__ as _ctor if __init__ is undefined.
    if (type.isBuiltin() &&
        typeAtById(thread, type, ID(__init__)).isErrorNotFound()) {
      Object dunder_new(&scope, typeAtById(thread, type, ID(__new__)));
      if (!dunder_new.isErrorNotFound()) {
        ctor = *dunder_new;
      }
    }
    if (ctor.isNoneType()) {
      ctor = runtime->lookupNameInModule(thread, ID(_builtins),
                                         ID(_type_dunder_call));
    }
    CHECK(ctor.isFunction(), "ctor is expected to be a function");
    type.setCtor(*ctor);
    return *type;
  }

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
    frame->pushValue(*prepare_method);
    Tuple pargs(&scope, runtime->newTuple(2));
    pargs.atPut(0, *name);
    pargs.atPut(1, *bases);
    frame->pushValue(*pargs);
    frame->pushValue(*kwargs);
    dict_obj =
        Interpreter::callEx(thread, frame, CallFunctionExFlag::VAR_KEYWORDS);
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
  Object body_result(&scope, thread->runClassFunction(body, type_dict));
  if (body_result.isError()) return *body_result;

  frame->pushValue(*metaclass);
  Tuple pargs(&scope, runtime->newTuple(3));
  pargs.atPut(0, *name);
  pargs.atPut(1, *bases);
  pargs.atPut(2, *type_dict);
  frame->pushValue(*pargs);
  frame->pushValue(*kwargs);
  return Interpreter::callEx(thread, frame, CallFunctionExFlag::VAR_KEYWORDS);
}

RawObject FUNC(builtins, callable)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object arg(&scope, args.get(0));
  return Bool::fromBool(thread->runtime()->isCallable(thread, arg));
}

RawObject FUNC(builtins, chr)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
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
  Object dont_inherit(&scope, Bool::trueObj());
  Object optimize_int(&scope, SmallInt::fromWord(optimize));
  return thread->invokeFunction6(ID(builtins), ID(compile), source, filename,
                                 mode_str, flags_int, dont_inherit,
                                 optimize_int);
}

RawObject FUNC(builtins, id)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  // NOTE: This pins a handle until the runtime exits.
  // TODO(emacs): Either determine that this function is used so little that it
  // does not matter or add a section to the GC to clean up handles created by
  // id().
  return thread->runtime()->newIntFromCPtr(
      ApiHandle::newReference(thread, args.get(0)));
}

RawObject FUNC(builtins, oct)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object number(&scope, args.get(0));
  number = intFromIndex(thread, number);
  if (number.isError()) {
    return *number;
  }
  Int number_int(&scope, intUnderlying(*number));
  return formatIntOctalSimple(thread, number_int);
}

RawObject FUNC(builtins, ord)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
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
      if (num_bytes == str.charLength()) {
        return SmallInt::fromWord(code_point);
      }
    }
  } else if (runtime->isInstanceOfByteArray(*obj)) {
    ByteArray byte_array(&scope, *obj);
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

RawObject FUNC(builtins, __import__)(Thread* thread, Frame* frame, word nargs) {
  // Note that this is a simplified __import__ implementation that is used
  // during early bootstrap; it is replaced by importlib.__import__ once
  // import lib is fully initialized.
  Arguments args(frame, nargs);
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

// TODO(T39322942): Turn this into the Range constructor (__init__ or __new__)
RawObject FUNC(builtins, getattr)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
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

RawObject FUNC(builtins, hasattr)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Object name(&scope, args.get(1));
  return hasAttribute(thread, self, name);
}

RawObject FUNC(builtins, hash)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object object(&scope, args.get(0));
  return Interpreter::hash(thread, object);
}

RawObject FUNC(builtins, hex)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object number(&scope, args.get(0));
  number = intFromIndex(thread, number);
  if (number.isError()) {
    return *number;
  }
  Int number_int(&scope, intUnderlying(*number));
  return formatIntHexadecimalSimple(thread, number_int);
}

RawObject FUNC(builtins, setattr)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Object name(&scope, args.get(1));
  Object value(&scope, args.get(2));
  return setAttribute(thread, self, name, value);
}

}  // namespace py
