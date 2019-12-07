#include "builtins-module.h"

#include <cerrno>

#include "bytes-builtins.h"
#include "capi-handles.h"
#include "exception-builtins.h"
#include "formatter.h"
#include "frozen-modules.h"
#include "int-builtins.h"
#include "marshal.h"
#include "module-builtins.h"
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

  Object result(&scope, thread->invokeMethod3(self, SymbolId::kDunderSetattr,
                                              name, value));
  if (result.isErrorException()) return *result;
  return NoneType::object();
}

// clang-format off
const BuiltinMethod BuiltinsModule::kBuiltinMethods[] = {
    {SymbolId::kBin, bin},
    {SymbolId::kCallable, callable},
    {SymbolId::kChr, chr},
    {SymbolId::kDelattr, delattr},
    {SymbolId::kDunderImport, dunderImport},
    {SymbolId::kGetattr, getattr},
    {SymbolId::kHasattr, hasattr},
    {SymbolId::kHash, hash},
    {SymbolId::kHex, hex},
    {SymbolId::kId, id},
    {SymbolId::kOct, oct},
    {SymbolId::kOrd, ord},
    {SymbolId::kSetattr, setattr},
    {SymbolId::kSentinelId, nullptr},
};
// clang-format on

const BuiltinType BuiltinsModule::kBuiltinTypes[] = {
    {SymbolId::kArithmeticError, LayoutId::kArithmeticError},
    {SymbolId::kAssertionError, LayoutId::kAssertionError},
    {SymbolId::kAttributeError, LayoutId::kAttributeError},
    {SymbolId::kBaseException, LayoutId::kBaseException},
    {SymbolId::kBlockingIOError, LayoutId::kBlockingIOError},
    {SymbolId::kBool, LayoutId::kBool},
    {SymbolId::kBrokenPipeError, LayoutId::kBrokenPipeError},
    {SymbolId::kBufferError, LayoutId::kBufferError},
    {SymbolId::kBytearray, LayoutId::kByteArray},
    {SymbolId::kBytearrayIterator, LayoutId::kByteArrayIterator},
    {SymbolId::kBytes, LayoutId::kBytes},
    {SymbolId::kBytesIterator, LayoutId::kBytesIterator},
    {SymbolId::kBytesWarning, LayoutId::kBytesWarning},
    {SymbolId::kChildProcessError, LayoutId::kChildProcessError},
    {SymbolId::kClassmethod, LayoutId::kClassMethod},
    {SymbolId::kCode, LayoutId::kCode},
    {SymbolId::kComplex, LayoutId::kComplex},
    {SymbolId::kConnectionAbortedError, LayoutId::kConnectionAbortedError},
    {SymbolId::kConnectionError, LayoutId::kConnectionError},
    {SymbolId::kConnectionRefusedError, LayoutId::kConnectionRefusedError},
    {SymbolId::kConnectionResetError, LayoutId::kConnectionResetError},
    {SymbolId::kCoroutine, LayoutId::kCoroutine},
    {SymbolId::kDeprecationWarning, LayoutId::kDeprecationWarning},
    {SymbolId::kDict, LayoutId::kDict},
    {SymbolId::kDictItemIterator, LayoutId::kDictItemIterator},
    {SymbolId::kDictItems, LayoutId::kDictItems},
    {SymbolId::kDictKeyIterator, LayoutId::kDictKeyIterator},
    {SymbolId::kDictKeys, LayoutId::kDictKeys},
    {SymbolId::kDictValueIterator, LayoutId::kDictValueIterator},
    {SymbolId::kDictValues, LayoutId::kDictValues},
    {SymbolId::kEOFError, LayoutId::kEOFError},
    {SymbolId::kEllipsis, LayoutId::kEllipsis},
    {SymbolId::kException, LayoutId::kException},
    {SymbolId::kFileExistsError, LayoutId::kFileExistsError},
    {SymbolId::kFileNotFoundError, LayoutId::kFileNotFoundError},
    {SymbolId::kFloat, LayoutId::kFloat},
    {SymbolId::kFloatingPointError, LayoutId::kFloatingPointError},
    {SymbolId::kFrozenSet, LayoutId::kFrozenSet},
    {SymbolId::kFunction, LayoutId::kFunction},
    {SymbolId::kFutureWarning, LayoutId::kFutureWarning},
    {SymbolId::kGenerator, LayoutId::kGenerator},
    {SymbolId::kGeneratorExit, LayoutId::kGeneratorExit},
    {SymbolId::kImportError, LayoutId::kImportError},
    {SymbolId::kImportWarning, LayoutId::kImportWarning},
    {SymbolId::kIndentationError, LayoutId::kIndentationError},
    {SymbolId::kIndexError, LayoutId::kIndexError},
    {SymbolId::kInt, LayoutId::kInt},
    {SymbolId::kInterruptedError, LayoutId::kInterruptedError},
    {SymbolId::kIsADirectoryError, LayoutId::kIsADirectoryError},
    {SymbolId::kKeyError, LayoutId::kKeyError},
    {SymbolId::kKeyboardInterrupt, LayoutId::kKeyboardInterrupt},
    {SymbolId::kList, LayoutId::kList},
    {SymbolId::kListIterator, LayoutId::kListIterator},
    {SymbolId::kLongRangeIterator, LayoutId::kLongRangeIterator},
    {SymbolId::kLookupError, LayoutId::kLookupError},
    {SymbolId::kMappingProxy, LayoutId::kMappingProxy},
    {SymbolId::kMemoryError, LayoutId::kMemoryError},
    {SymbolId::kMemoryView, LayoutId::kMemoryView},
    {SymbolId::kMethod, LayoutId::kBoundMethod},
    {SymbolId::kModule, LayoutId::kModule},
    {SymbolId::kModuleProxy, LayoutId::kModuleProxy},
    {SymbolId::kModuleNotFoundError, LayoutId::kModuleNotFoundError},
    {SymbolId::kNameError, LayoutId::kNameError},
    {SymbolId::kNoneType, LayoutId::kNoneType},
    {SymbolId::kNotADirectoryError, LayoutId::kNotADirectoryError},
    {SymbolId::kNotImplementedError, LayoutId::kNotImplementedError},
    {SymbolId::kNotImplementedType, LayoutId::kNotImplementedType},
    {SymbolId::kOSError, LayoutId::kOSError},
    {SymbolId::kObjectTypename, LayoutId::kObject},
    {SymbolId::kOverflowError, LayoutId::kOverflowError},
    {SymbolId::kPendingDeprecationWarning,
     LayoutId::kPendingDeprecationWarning},
    {SymbolId::kPermissionError, LayoutId::kPermissionError},
    {SymbolId::kProcessLookupError, LayoutId::kProcessLookupError},
    {SymbolId::kProperty, LayoutId::kProperty},
    {SymbolId::kRange, LayoutId::kRange},
    {SymbolId::kRangeIterator, LayoutId::kRangeIterator},
    {SymbolId::kRecursionError, LayoutId::kRecursionError},
    {SymbolId::kReferenceError, LayoutId::kReferenceError},
    {SymbolId::kResourceWarning, LayoutId::kResourceWarning},
    {SymbolId::kRuntimeError, LayoutId::kRuntimeError},
    {SymbolId::kRuntimeWarning, LayoutId::kRuntimeWarning},
    {SymbolId::kSet, LayoutId::kSet},
    {SymbolId::kSetIterator, LayoutId::kSetIterator},
    {SymbolId::kSeqIterator, LayoutId::kSeqIterator},
    {SymbolId::kSlice, LayoutId::kSlice},
    {SymbolId::kStaticMethod, LayoutId::kStaticMethod},
    {SymbolId::kStopAsyncIteration, LayoutId::kStopAsyncIteration},
    {SymbolId::kStopIteration, LayoutId::kStopIteration},
    {SymbolId::kStr, LayoutId::kStr},
    {SymbolId::kStrIterator, LayoutId::kStrIterator},
    {SymbolId::kSuper, LayoutId::kSuper},
    {SymbolId::kSyntaxError, LayoutId::kSyntaxError},
    {SymbolId::kSyntaxWarning, LayoutId::kSyntaxWarning},
    {SymbolId::kSystemError, LayoutId::kSystemError},
    {SymbolId::kSystemExit, LayoutId::kSystemExit},
    {SymbolId::kTabError, LayoutId::kTabError},
    {SymbolId::kTimeoutError, LayoutId::kTimeoutError},
    {SymbolId::kTuple, LayoutId::kTuple},
    {SymbolId::kTupleIterator, LayoutId::kTupleIterator},
    {SymbolId::kType, LayoutId::kType},
    {SymbolId::kTypeError, LayoutId::kTypeError},
    {SymbolId::kTypeProxy, LayoutId::kTypeProxy},
    {SymbolId::kUnboundLocalError, LayoutId::kUnboundLocalError},
    {SymbolId::kUnderStrarray, LayoutId::kStrArray},
    {SymbolId::kUnderTraceback, LayoutId::kTraceback},
    {SymbolId::kUnicodeDecodeError, LayoutId::kUnicodeDecodeError},
    {SymbolId::kUnicodeEncodeError, LayoutId::kUnicodeEncodeError},
    {SymbolId::kUnicodeError, LayoutId::kUnicodeError},
    {SymbolId::kUnicodeTranslateError, LayoutId::kUnicodeTranslateError},
    {SymbolId::kUnicodeWarning, LayoutId::kUnicodeWarning},
    {SymbolId::kUserWarning, LayoutId::kUserWarning},
    {SymbolId::kValueError, LayoutId::kValueError},
    {SymbolId::kWarning, LayoutId::kWarning},
    {SymbolId::kZeroDivisionError, LayoutId::kZeroDivisionError},
    {SymbolId::kSentinelId, LayoutId::kSentinelId},
};

const char* const BuiltinsModule::kFrozenData = kBuiltinsModuleData;

const SymbolId BuiltinsModule::kIntrinsicIds[] = {
    SymbolId::kUnderIndex,      SymbolId::kUnderNumberCheck,
    SymbolId::kUnderSliceIndex, SymbolId::kUnderSliceIndexNotNone,
    SymbolId::kIsInstance,      SymbolId::kLen,
    SymbolId::kSentinelId,
};

static void patchTypeDict(Thread* thread, const Type& base_type,
                          const Dict& patch) {
  HandleScope scope(thread);
  Tuple patch_data(&scope, patch.data());
  for (word i = Dict::Bucket::kFirst;
       Dict::Bucket::nextItem(*patch_data, &i);) {
    Str name(&scope, Dict::Bucket::key(*patch_data, i));
    Object patch_obj(&scope, Dict::Bucket::value(*patch_data, i));

    // Copy function entries if the method already exists as a native builtin.
    Object base_obj(&scope, typeAt(base_type, name));
    if (!base_obj.isError()) {
      CHECK(patch_obj.isFunction(), "Python should only annotate functions");
      Function patch_fn(&scope, *patch_obj);
      CHECK(base_obj.isFunction(),
            "Python annotation of non-function native object");
      Function base_fn(&scope, *base_obj);

      copyFunctionEntries(thread, base_fn, patch_fn);
    }
    typeAtPut(thread, base_type, name, patch_obj);
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

RawObject BuiltinsModule::bin(Thread* thread, Frame* frame, word nargs) {
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

RawObject BuiltinsModule::delattr(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Object name(&scope, args.get(1));
  Object result(&scope, delAttribute(thread, self, name));
  return *result;
}

RawObject BuiltinsModule::dunderBuildClass(Thread* thread, Frame* frame,
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

    if (bases.length() == 0 && name != runtime->symbols()->ObjectTypename()) {
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

    Dict patch_type(&scope, runtime->newDict());
    Object result(&scope, thread->runClassFunction(body, patch_type));
    if (result.isError()) return *result;
    patchTypeDict(thread, type, patch_type);
    // TODO(T53997177): Centralize type initialization
    typeAddDocstring(thread, type);
    // A bootstrap type initialization is complete at this point.

    Object ctor(&scope, NoneType::object());
    // Use __new__ as _ctor if __init__ is undefined.
    if (type.isBuiltin() &&
        typeAtById(thread, type, SymbolId::kDunderInit).isErrorNotFound()) {
      Object dunder_new(&scope, typeAtById(thread, type, SymbolId::kDunderNew));
      if (!dunder_new.isErrorNotFound()) {
        ctor = *dunder_new;
      }
    }
    if (ctor.isNoneType()) {
      ctor = runtime->lookupNameInModule(thread, SymbolId::kUnderBuiltins,
                                         SymbolId::kUnderTypeDunderCall);
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
      &scope,
      runtime->attributeAtById(thread, metaclass, SymbolId::kDunderPrepare));
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

RawObject BuiltinsModule::callable(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object arg(&scope, args.get(0));
  return Bool::fromBool(thread->runtime()->isCallable(thread, arg));
}

RawObject BuiltinsModule::chr(Thread* thread, Frame* frame, word nargs) {
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
  return thread->invokeFunction6(SymbolId::kBuiltins, SymbolId::kCompile,
                                 source, filename, mode_str, flags_int,
                                 dont_inherit, optimize_int);
}

RawObject BuiltinsModule::id(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  // NOTE: This pins a handle until the runtime exits.
  // TODO(emacs): Either determine that this function is used so little that it
  // does not matter or add a section to the GC to clean up handles created by
  // id().
  return thread->runtime()->newIntFromCPtr(
      ApiHandle::newReference(thread, args.get(0)));
}

RawObject BuiltinsModule::oct(Thread* thread, Frame* frame, word nargs) {
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

RawObject BuiltinsModule::ord(Thread* thread, Frame* frame, word nargs) {
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

RawObject BuiltinsModule::dunderImport(Thread* thread, Frame* frame,
                                       word nargs) {
  CHECK(!thread->runtime()->findOrCreateImportlibModule(thread).isNoneType(),
        "failed to initialize importlib");
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object name(&scope, args.get(0));
  Object globals(&scope, args.get(1));
  Object locals(&scope, args.get(2));
  Object fromlist(&scope, args.get(3));
  Object level(&scope, args.get(4));
  return thread->invokeFunction5(SymbolId::kUnderFrozenImportlib,
                                 SymbolId::kDunderImport, name, globals, locals,
                                 fromlist, level);
}

// TODO(T39322942): Turn this into the Range constructor (__init__ or __new__)
RawObject BuiltinsModule::getattr(Thread* thread, Frame* frame, word nargs) {
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

RawObject BuiltinsModule::hasattr(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Object name(&scope, args.get(1));
  return hasAttribute(thread, self, name);
}

RawObject BuiltinsModule::hash(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object object(&scope, args.get(0));
  return Interpreter::hash(thread, object);
}

RawObject BuiltinsModule::hex(Thread* thread, Frame* frame, word nargs) {
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

RawObject BuiltinsModule::setattr(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Object name(&scope, args.get(1));
  Object value(&scope, args.get(2));
  return setAttribute(thread, self, name, value);
}

}  // namespace py
