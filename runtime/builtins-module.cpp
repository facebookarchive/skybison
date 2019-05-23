#include "builtins-module.h"

#include <cerrno>

#include "bytearray-builtins.h"
#include "bytes-builtins.h"
#include "complex-builtins.h"
#include "dict-builtins.h"
#include "exception-builtins.h"
#include "frame.h"
#include "frozen-modules.h"
#include "globals.h"
#include "handles.h"
#include "int-builtins.h"
#include "interpreter.h"
#include "list-builtins.h"
#include "marshal.h"
#include "objects.h"
#include "runtime.h"
#include "str-builtins.h"
#include "thread.h"
#include "trampolines-inl.h"
#include "tuple-builtins.h"
#include "type-builtins.h"

namespace python {

RawObject getAttribute(Thread* thread, const Object& self, const Object& name) {
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfStr(*name)) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "getattr(): attribute name must be string");
  }
  return runtime->attributeAt(thread, self, name);
}

RawObject hasAttribute(Thread* thread, const Object& self, const Object& name) {
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfStr(*name)) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "hasattr(): attribute name must be string");
  }

  HandleScope scope(thread);
  Object result(&scope, runtime->attributeAt(thread, self, name));
  if (!result.isError()) {
    return Bool::trueObj();
  }

  Type given(&scope, thread->pendingExceptionType());
  Type exc(&scope, runtime->typeAt(LayoutId::kAttributeError));
  if (givenExceptionMatches(thread, given, exc)) {
    thread->clearPendingException();
    return Bool::falseObj();
  }

  return Error::notFound();
}

RawObject setAttribute(Thread* thread, const Object& self, const Object& name,
                       const Object& value) {
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfStr(*name)) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "setattr(): attribute name must be string");
  }
  HandleScope scope(thread);
  Object result(&scope, thread->invokeMethod3(self, SymbolId::kDunderSetattr,
                                              name, value));
  if (result.isError()) return *result;
  return NoneType::object();
}

const BuiltinMethod BuiltinsModule::kBuiltinMethods[] = {
    {SymbolId::kCallable, callable},
    {SymbolId::kChr, chr},
    {SymbolId::kCompile, compile},
    {SymbolId::kDivmod, divmod},
    {SymbolId::kDunderImport, dunderImport},
    {SymbolId::kExec, exec},
    {SymbolId::kGetattr, getattr},
    {SymbolId::kHasattr, hasattr},
    {SymbolId::kOrd, ord},
    {SymbolId::kSetattr, setattr},
    {SymbolId::kUnderAddress, underAddress},
    {SymbolId::kUnderBoundMethod, underBoundMethod},
    {SymbolId::kUnderByteArrayCheck, underByteArrayCheck},
    {SymbolId::kUnderByteArrayJoin, ByteArrayBuiltins::join},
    {SymbolId::kUnderBytesCheck, underBytesCheck},
    {SymbolId::kUnderBytesFromInts, underBytesFromInts},
    {SymbolId::kUnderBytesGetitem, underBytesGetItem},
    {SymbolId::kUnderBytesGetitemSlice, underBytesGetItemSlice},
    {SymbolId::kUnderBytesJoin, BytesBuiltins::join},
    {SymbolId::kUnderBytesMaketrans, underBytesMaketrans},
    {SymbolId::kUnderBytesRepeat, underBytesRepeat},
    {SymbolId::kUnderComplexImag, complexGetImag},
    {SymbolId::kUnderComplexReal, complexGetReal},
    {SymbolId::kUnderDictCheck, underDictCheck},
    {SymbolId::kUnderDictUpdateMapping, underDictUpdateMapping},
    {SymbolId::kUnderFloatCheck, underFloatCheck},
    {SymbolId::kUnderFrozenSetCheck, underFrozenSetCheck},
    {SymbolId::kUnderGetMemberByte, underGetMemberByte},
    {SymbolId::kUnderGetMemberChar, underGetMemberChar},
    {SymbolId::kUnderGetMemberDouble, underGetMemberDouble},
    {SymbolId::kUnderGetMemberFloat, underGetMemberFloat},
    {SymbolId::kUnderGetMemberInt, underGetMemberInt},
    {SymbolId::kUnderGetMemberLong, underGetMemberLong},
    {SymbolId::kUnderGetMemberPyObject, underGetMemberPyObject},
    {SymbolId::kUnderGetMemberShort, underGetMemberShort},
    {SymbolId::kUnderGetMemberString, underGetMemberString},
    {SymbolId::kUnderGetMemberUByte, underGetMemberUByte},
    {SymbolId::kUnderGetMemberUInt, underGetMemberUInt},
    {SymbolId::kUnderGetMemberULong, underGetMemberULong},
    {SymbolId::kUnderGetMemberUShort, underGetMemberUShort},
    {SymbolId::kUnderIntCheck, underIntCheck},
    {SymbolId::kUnderIntFromBytes, underIntFromBytes},
    {SymbolId::kUnderIntFromByteArray, underIntFromByteArray},
    {SymbolId::kUnderIntFromInt, underIntFromInt},
    {SymbolId::kUnderIntFromStr, underIntFromStr},
    {SymbolId::kUnderListCheck, underListCheck},
    {SymbolId::kUnderListDelitem, underListDelItem},
    {SymbolId::kUnderListDelslice, underListDelSlice},
    {SymbolId::kUnderListSort, underListSort},
    {SymbolId::kUnderPyObjectOffset, underPyObjectOffset},
    {SymbolId::kUnderReprEnter, underReprEnter},
    {SymbolId::kUnderReprLeave, underReprLeave},
    {SymbolId::kUnderSetMemberDouble, underSetMemberDouble},
    {SymbolId::kUnderSetMemberFloat, underSetMemberFloat},
    {SymbolId::kUnderSetMemberIntegral, underSetMemberIntegral},
    {SymbolId::kUnderSetMemberPyObject, underSetMemberPyObject},
    {SymbolId::kUnderSetCheck, underSetCheck},
    {SymbolId::kUnderSliceCheck, underSliceCheck},
    {SymbolId::kUnderStrCheck, underStrCheck},
    {SymbolId::kUnderStrEscapeNonAscii, underStrEscapeNonAscii},
    {SymbolId::kUnderStrFind, underStrFind},
    {SymbolId::kUnderStrFromStr, underStrFromStr},
    {SymbolId::kUnderStrReplace, underStrReplace},
    {SymbolId::kUnderStrRFind, underStrRFind},
    {SymbolId::kUnderStrSplitlines, underStrSplitlines},
    {SymbolId::kUnderStructseqGetAttr, underStructseqGetAttr},
    {SymbolId::kUnderStructseqSetAttr, underStructseqSetAttr},
    {SymbolId::kUnderTupleCheck, underTupleCheck},
    {SymbolId::kUnderTypeCheck, underTypeCheck},
    {SymbolId::kUnderTypeCheckExact, underTypeCheckExact},
    {SymbolId::kUnderTypeIsSubclass, underTypeIsSubclass},
    {SymbolId::kUnderUnimplemented, underUnimplemented},
    {SymbolId::kSentinelId, nullptr},
};

const BuiltinType BuiltinsModule::kBuiltinTypes[] = {
    {SymbolId::kArithmeticError, LayoutId::kArithmeticError},
    {SymbolId::kAssertionError, LayoutId::kAssertionError},
    {SymbolId::kAttributeError, LayoutId::kAttributeError},
    {SymbolId::kBaseException, LayoutId::kBaseException},
    {SymbolId::kBlockingIOError, LayoutId::kBlockingIOError},
    {SymbolId::kBool, LayoutId::kBool},
    {SymbolId::kBrokenPipeError, LayoutId::kBrokenPipeError},
    {SymbolId::kBufferError, LayoutId::kBufferError},
    {SymbolId::kByteArray, LayoutId::kByteArray},
    {SymbolId::kByteArrayIterator, LayoutId::kByteArrayIterator},
    {SymbolId::kBytes, LayoutId::kBytes},
    {SymbolId::kBytesWarning, LayoutId::kBytesWarning},
    {SymbolId::kChildProcessError, LayoutId::kChildProcessError},
    {SymbolId::kClassmethod, LayoutId::kClassMethod},
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
    {SymbolId::kLookupError, LayoutId::kLookupError},
    {SymbolId::kMemoryError, LayoutId::kMemoryError},
    {SymbolId::kMemoryView, LayoutId::kMemoryView},
    {SymbolId::kModule, LayoutId::kModule},
    {SymbolId::kModuleNotFoundError, LayoutId::kModuleNotFoundError},
    {SymbolId::kNameError, LayoutId::kNameError},
    {SymbolId::kNoneType, LayoutId::kNoneType},
    {SymbolId::kNotADirectoryError, LayoutId::kNotADirectoryError},
    {SymbolId::kNotImplementedError, LayoutId::kNotImplementedError},
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
    {SymbolId::kUnboundLocalError, LayoutId::kUnboundLocalError},
    {SymbolId::kUnderStrArray, LayoutId::kStrArray},
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

static bool isPass(const Code& code) {
  HandleScope scope;
  Bytes bytes(&scope, code.code());
  // const_loaded is the index into the consts array that is returned
  word const_loaded = bytes.byteAt(1);
  return bytes.length() == 4 && bytes.byteAt(0) == LOAD_CONST &&
         Tuple::cast(code.consts()).at(const_loaded).isNoneType() &&
         bytes.byteAt(2) == RETURN_VALUE && bytes.byteAt(3) == 0;
}

void copyFunctionEntries(Thread* thread, const Function& base,
                         const Function& patch) {
  HandleScope scope(thread);
  Str method_name(&scope, base.name());
  Code patch_code(&scope, patch.code());
  Code base_code(&scope, base.code());
  CHECK(isPass(patch_code),
        "Redefinition of native code method '%s' in managed code",
        method_name.toCStr());
  CHECK(!base_code.code().isNoneType(),
        "Useless declaration of native code method %s in managed code",
        method_name.toCStr());
  patch_code.setCode(base_code.code());
  patch.setEntry(base.entry());
  patch.setEntryKw(base.entryKw());
  patch.setEntryEx(base.entryEx());
  patch.setIsInterpreted(false);
}

static void patchTypeDict(Thread* thread, const Dict& base, const Dict& patch) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Tuple patch_data(&scope, patch.data());
  for (word i = Dict::Bucket::kFirst;
       Dict::Bucket::nextItem(*patch_data, &i);) {
    Str key(&scope, Dict::Bucket::key(*patch_data, i));
    Object patch_value_cell(&scope, Dict::Bucket::value(*patch_data, i));
    DCHECK(patch_value_cell.isValueCell(),
           "Values in type dict should be ValueCell");
    Object patch_obj(&scope, ValueCell::cast(*patch_value_cell).value());

    // Copy function entries if the method already exists as a native builtin.
    Object base_obj(&scope, runtime->typeDictAt(thread, base, key));
    if (!base_obj.isError()) {
      CHECK(patch_obj.isFunction(), "Python should only annotate functions");
      Function patch_fn(&scope, *patch_obj);
      CHECK(base_obj.isFunction(),
            "Python annotation of non-function native object");
      Function base_fn(&scope, *base_obj);

      copyFunctionEntries(thread, base_fn, patch_fn);
    }
    runtime->typeDictAtPut(thread, base, key, patch_obj);
  }
}

static RawObject calculateMetaclass(Thread* thread, const Type& metaclass_type,
                                    const Tuple& bases) {
  HandleScope scope(thread);
  Type result(&scope, *metaclass_type);
  Runtime* runtime = thread->runtime();
  for (word i = 0, num_bases = bases.length(); i < num_bases; i++) {
    Type base_type(&scope, runtime->typeOf(bases.at(i)));
    if (runtime->isSubclass(base_type, result)) {
      result = *base_type;
    } else if (!runtime->isSubclass(result, base_type)) {
      return thread->raiseWithFmt(
          LayoutId::kTypeError,
          "metaclass conflict: the metaclass of a derived class must be a "
          "(non-strict) subclass of the metaclasses of all its bases");
    }
  }
  return *result;
}

// Wraps every value of the dict with ValueCell.
static void wrapInValueCells(Thread* thread, const Dict& dict) {
  HandleScope scope(thread);
  Tuple data(&scope, dict.data());
  Runtime* runtime = thread->runtime();
  for (word i = Dict::Bucket::kFirst; Dict::Bucket::nextItem(*data, &i);) {
    DCHECK(!Dict::Bucket::value(*data, i).isValueCell(),
           "ValueCell in Python code");
    ValueCell cell(&scope, runtime->newValueCell());
    cell.setValue(Dict::Bucket::value(*data, i));
    Dict::Bucket::set(*data, i, Dict::Bucket::hash(*data, i),
                      Dict::Bucket::key(*data, i), *cell);
  }
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
    // A bootstrap class initialization uses the existing class dictionary.
    CHECK(frame->previousFrame() != nullptr, "must have a caller frame");
    Dict globals(&scope, frame->previousFrame()->globals());
    Object type_obj(&scope, runtime->moduleDictAt(thread, globals, name));
    CHECK(type_obj.isType(),
          "Name '%s' is not bound to a type object. "
          "You may need to add it to the builtins module.",
          Str::cast(*name).toCStr());
    Type type(&scope, *type_obj);
    Dict type_dict(&scope, type.dict());

    Dict patch_type(&scope, runtime->newDict());
    Object result(&scope, thread->runClassFunction(body, patch_type));
    if (result.isError()) return *result;
    patchTypeDict(thread, type_dict, patch_type);
    // A bootstrap type initialization is complete at this point.
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
      runtime->attributeAtId(thread, metaclass, SymbolId::kDunderPrepare));
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
  wrapInValueCells(thread, type_dict);

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
  Int num(&scope, intUnderlying(thread, arg));
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

static RawObject compileToBytecode(Thread* thread, const char* source,
                                   const Str& filename) {
  HandleScope scope(thread);
  unique_c_ptr<char[]> filename_c_str(filename.toCStr());
  std::unique_ptr<char[]> bytecode_str(
      Runtime::compileFromCStr(source, filename_c_str.get()));
  Marshal::Reader reader(&scope, thread->runtime(), bytecode_str.get());
  reader.readLong();  // magic
  reader.readLong();  // mtime
  reader.readLong();  // size
  return reader.readObject();
}

static RawObject compileBytes(Thread* thread, const Bytes& source,
                              const Str& filename) {
  word bytes_len = source.length();
  unique_c_ptr<byte[]> source_bytes(
      static_cast<byte*>(std::malloc(bytes_len + 1)));
  source.copyTo(source_bytes.get(), bytes_len);
  source_bytes[bytes_len] = '\0';
  return compileToBytecode(thread, reinterpret_cast<char*>(source_bytes.get()),
                           filename);
}

static RawObject compileStr(Thread* thread, const Str& source,
                            const Str& filename) {
  unique_c_ptr<char[]> source_str(source.toCStr());
  return compileToBytecode(thread, source_str.get(), filename);
}

RawObject BuiltinsModule::compile(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  // TODO(T40808881): Add compile support for bytearray, buffer, and subclasses
  Object data(&scope, args.get(0));
  if (!data.isStr() && !data.isBytes()) {
    return thread->raiseWithFmt(
        LayoutId::kTypeError,
        "compile() currently only supports a str or bytes source");
  }
  Str filename(&scope, args.get(1));
  Str mode(&scope, args.get(2));
  // TODO(emacs): Refactor into sane argument-fetching code
  if (args.get(3) != SmallInt::fromWord(0)) {  // not the default
    return thread->raiseWithFmt(
        LayoutId::kTypeError,
        "compile() does not yet support user-supplied flags");
  }
  // TODO(T40872645): Add support for compiler flag forwarding
  if (args.get(4) == Bool::falseObj()) {
    return thread->raiseWithFmt(
        LayoutId::kTypeError,
        "compile() does not yet support compiler flag forwarding");
  }
  if (args.get(5) != SmallInt::fromWord(-1)) {  // not the default
    return thread->raiseWithFmt(
        LayoutId::kTypeError,
        "compile() does not yet support user-supplied optimize");
  }
  // Note: mode doesn't actually do anything yet.
  if (!mode.equalsCStr("exec") && !mode.equalsCStr("eval") &&
      !mode.equalsCStr("single")) {
    return thread->raiseWithFmt(
        LayoutId::kValueError,
        "Expected mode to be 'exec', 'eval', or 'single' in 'compile'");
  }

  if (data.isStr()) {
    Str source_str(&scope, *data);
    return compileStr(thread, source_str, filename);
  }
  Bytes source_bytes(&scope, bytesUnderlying(thread, data));
  return compileBytes(thread, source_bytes, filename);
}

RawObject BuiltinsModule::divmod(Thread*, Frame*, word) {
  UNIMPLEMENTED("divmod(a, b)");
}

RawObject BuiltinsModule::exec(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object source_obj(&scope, args.get(0));
  if (!source_obj.isCode() && !source_obj.isStr()) {
    return thread->raiseWithFmt(
        LayoutId::kTypeError, "Expected 'source' to be str or code in 'exec'");
  }
  // Per the docs:
  //   In all cases, if the optional parts are omitted, the code is executed in
  //   the current scope. If only globals is provided, it must be a dictionary,
  //   which will be used for both the global and the local variables.
  Object globals_obj(&scope, args.get(1));
  Object locals(&scope, args.get(2));
  Runtime* runtime = thread->runtime();
  if (globals_obj.isNoneType() &&
      locals.isNoneType()) {  // neither globals nor locals are provided
    Frame* caller_frame = frame->previousFrame();
    globals_obj = caller_frame->globals();
    DCHECK(globals_obj.isDict(),
           "Expected caller_frame->globals() to be dict in 'exec'");
    if (caller_frame->globals() != caller_frame->implicitGlobals()) {
      // TODO(T37888835): Fix 1 argument case
      // globals == implicitGlobals when code is being executed in a module
      // context. If we're not in a module context, this case is unimplemented.
      UNIMPLEMENTED("exec() with 1 argument not at the module level");
    }
    locals = *globals_obj;
  } else if (!globals_obj.isNoneType()) {  // only globals is provided
    if (!runtime->isInstanceOfDict(*globals_obj)) {
      return thread->raiseWithFmt(LayoutId::kTypeError,
                                  "Expected 'globals' to be dict in 'exec'");
    }
    locals = *globals_obj;
  } else {  // both globals and locals are provided
    if (!runtime->isInstanceOfDict(*globals_obj)) {
      return thread->raiseWithFmt(LayoutId::kTypeError,
                                  "Expected 'globals' to be dict in 'exec'");
    }
    if (!runtime->isMapping(thread, locals)) {
      return thread->raiseWithFmt(
          LayoutId::kTypeError, "Expected 'locals' to be a mapping in 'exec'");
    }
    // TODO(T37888835): Fix 3 argument case
    UNIMPLEMENTED("exec() with both globals and locals");
  }
  if (source_obj.isStr()) {
    Str source(&scope, *source_obj);
    Str filename(&scope, runtime->newStrFromCStr("<exec string>"));
    source_obj = compileStr(thread, source, filename);
    DCHECK(source_obj.isCode(), "compileStr must return code object");
  }
  Code code(&scope, *source_obj);
  if (code.numFreevars() != 0) {
    return thread->raiseWithFmt(
        LayoutId::kTypeError,
        "Expected 'source' not to have free variables in 'exec'");
  }
  Dict globals(&scope, *globals_obj);
  return thread->exec(code, globals, locals);
}

RawObject BuiltinsModule::ord(Thread* thread, Frame* frame_frame, word nargs) {
  Arguments args(frame_frame, nargs);
  RawObject arg = args.get(0);
  if (!arg.isStr()) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "Unsupported type in builtin 'ord'");
  }
  auto str = Str::cast(arg);
  word num_bytes;
  int32_t codepoint = str.codePointAt(0, &num_bytes);
  if (num_bytes != str.length()) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "Builtin 'ord' expects string of length 1");
  }
  return SmallInt::fromWord(codepoint);
}

RawObject BuiltinsModule::dunderImport(Thread* thread, Frame* frame,
                                       word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object name(&scope, args.get(0));
  Object globals(&scope, args.get(1));
  Object locals(&scope, args.get(2));
  Object fromlist(&scope, args.get(3));
  Object level(&scope, args.get(4));

  Runtime* runtime = thread->runtime();
  if (runtime->isInstanceOfInt(*level)) {
    Int level_int(&scope, intUnderlying(thread, level));
    if (level_int.isZero()) {
      Object cached_module(&scope, runtime->findModule(name));
      if (!cached_module.isNoneType()) {
        return *cached_module;
      }
    }
  }

  Object importlib_obj(
      &scope, runtime->findModuleById(SymbolId::kUnderFrozenImportlib));
  // We may need to load and create `_frozen_importlib` if it doesn't exist.
  if (importlib_obj.isNoneType()) {
    runtime->createImportlibModule(thread);
    importlib_obj = runtime->findModuleById(SymbolId::kUnderFrozenImportlib);
  }
  Module importlib(&scope, *importlib_obj);

  Object dunder_import(
      &scope, runtime->moduleAtById(importlib, SymbolId::kDunderImport));
  if (dunder_import.isError()) return *dunder_import;

  return thread->invokeFunction5(SymbolId::kUnderFrozenImportlib,
                                 SymbolId::kDunderImport, name, globals, locals,
                                 fromlist, level);
}

RawObject BuiltinsModule::underByteArrayCheck(Thread* thread, Frame* frame,
                                              word nargs) {
  Arguments args(frame, nargs);
  return Bool::fromBool(thread->runtime()->isInstanceOfByteArray(args.get(0)));
}

RawObject BuiltinsModule::underBytesCheck(Thread* thread, Frame* frame,
                                          word nargs) {
  Arguments args(frame, nargs);
  return Bool::fromBool(thread->runtime()->isInstanceOfBytes(args.get(0)));
}

RawObject BuiltinsModule::underBytesFromInts(Thread* thread, Frame* frame,
                                             word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object src(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  // TODO(T38246066): buffers other than bytes, bytearray
  if (runtime->isInstanceOfBytes(*src)) {
    return *src;
  }
  if (runtime->isInstanceOfByteArray(*src)) {
    ByteArray source(&scope, *src);
    return byteArrayAsBytes(thread, runtime, source);
  }
  if (src.isList()) {
    List source(&scope, *src);
    Tuple items(&scope, source.items());
    return runtime->bytesFromTuple(thread, items, source.numItems());
  }
  if (src.isTuple()) {
    Tuple source(&scope, *src);
    return runtime->bytesFromTuple(thread, source, source.length());
  }
  if (runtime->isInstanceOfStr(*src)) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "cannot convert '%T' object to bytes", &src);
  }
  // Slow path: iterate over source in Python, collect into list, and call again
  return NoneType::object();
}

RawObject BuiltinsModule::underBytesGetItem(Thread* thread, Frame* frame,
                                            word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  Bytes self(&scope, bytesUnderlying(thread, self_obj));
  Object key_obj(&scope, args.get(1));
  Int key(&scope, intUnderlying(thread, key_obj));
  word index = key.asWordSaturated();
  if (!SmallInt::isValid(index)) {
    return thread->raiseWithFmt(LayoutId::kIndexError,
                                "cannot fit '%T' into an index-sized integer",
                                &key_obj);
  }
  word length = self.length();
  if (index < 0) {
    index += length;
  }
  if (index < 0 || index >= length) {
    return thread->raiseWithFmt(LayoutId::kIndexError, "index out of range");
  }
  return SmallInt::fromWord(self.byteAt(index));
}

RawObject BuiltinsModule::underBytesGetItemSlice(Thread* thread, Frame* frame,
                                                 word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  Bytes self(&scope, bytesUnderlying(thread, self_obj));
  Object obj(&scope, args.get(1));
  Int start(&scope, intUnderlying(thread, obj));
  obj = args.get(2);
  Int stop(&scope, intUnderlying(thread, obj));
  obj = args.get(3);
  Int step(&scope, intUnderlying(thread, obj));
  return thread->runtime()->bytesSlice(thread, self, start.asWord(),
                                       stop.asWord(), step.asWord());
}

RawObject BuiltinsModule::underBytesMaketrans(Thread* thread, Frame* frame,
                                              word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object from_obj(&scope, args.get(0));
  Object to_obj(&scope, args.get(1));
  word length;
  Runtime* runtime = thread->runtime();
  if (runtime->isInstanceOfBytes(*from_obj)) {
    Bytes bytes(&scope, bytesUnderlying(thread, from_obj));
    length = bytes.length();
    from_obj = *bytes;
  } else if (runtime->isInstanceOfByteArray(*from_obj)) {
    ByteArray array(&scope, *from_obj);
    length = array.numItems();
    from_obj = array.bytes();
  } else {
    UNIMPLEMENTED("bytes-like other than bytes or bytearray");
  }
  if (runtime->isInstanceOfBytes(*to_obj)) {
    Bytes bytes(&scope, bytesUnderlying(thread, to_obj));
    DCHECK(bytes.length() == length, "lengths should already be the same");
    to_obj = *bytes;
  } else if (runtime->isInstanceOfByteArray(*to_obj)) {
    ByteArray array(&scope, *to_obj);
    DCHECK(array.numItems() == length, "lengths should already be the same");
    to_obj = array.bytes();
  } else {
    UNIMPLEMENTED("bytes-like other than bytes or bytearray");
  }
  Bytes from(&scope, *from_obj);
  Bytes to(&scope, *to_obj);
  byte table[BytesBuiltins::kTranslationTableLength];
  for (word i = 0; i < BytesBuiltins::kTranslationTableLength; i++) {
    table[i] = i;
  }
  for (word i = 0; i < length; i++) {
    table[from.byteAt(i)] = to.byteAt(i);
  }
  return runtime->newBytesWithAll(table);
}

RawObject BuiltinsModule::underBytesRepeat(Thread* thread, Frame* frame,
                                           word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  Bytes self(&scope, bytesUnderlying(thread, self_obj));
  Object count_obj(&scope, args.get(1));
  Int count_int(&scope, intUnderlying(thread, count_obj));
  word count = count_int.asWordSaturated();
  if (!SmallInt::isValid(count)) {
    return thread->raiseWithFmt(LayoutId::kOverflowError,
                                "cannot fit '%T' into an index-sized integer",
                                &count_obj);
  }
  // NOTE: unlike __mul__, we raise a value error for negative count
  if (count < 0) {
    return thread->raiseWithFmt(LayoutId::kValueError, "negative count");
  }
  return thread->runtime()->bytesRepeat(thread, self, self.length(), count);
}

RawObject BuiltinsModule::underDictCheck(Thread* thread, Frame* frame,
                                         word nargs) {
  Arguments args(frame, nargs);
  return Bool::fromBool(thread->runtime()->isInstanceOfDict(args.get(0)));
}

RawObject BuiltinsModule::underFloatCheck(Thread* thread, Frame* frame,
                                          word nargs) {
  Arguments args(frame, nargs);
  return Bool::fromBool(thread->runtime()->isInstanceOfFloat(args.get(0)));
}

RawObject BuiltinsModule::underFrozenSetCheck(Thread* thread, Frame* frame,
                                              word nargs) {
  Arguments args(frame, nargs);
  return Bool::fromBool(thread->runtime()->isInstanceOfFrozenSet(args.get(0)));
}

RawObject BuiltinsModule::underGetMemberByte(Thread* thread, Frame* frame,
                                             word nargs) {
  Arguments args(frame, nargs);
  auto addr = Int::cast(args.get(0)).asCPtr();
  char value = 0;
  std::memcpy(&value, reinterpret_cast<void*>(addr), 1);
  return thread->runtime()->newInt(value);
}

RawObject BuiltinsModule::underGetMemberChar(Thread*, Frame* frame,
                                             word nargs) {
  Arguments args(frame, nargs);
  auto addr = Int::cast(args.get(0)).asCPtr();
  return SmallStr::fromCodePoint(*reinterpret_cast<byte*>(addr));
}

RawObject BuiltinsModule::underGetMemberDouble(Thread* thread, Frame* frame,
                                               word nargs) {
  Arguments args(frame, nargs);
  auto addr = Int::cast(args.get(0)).asCPtr();
  double value = 0.0;
  std::memcpy(&value, reinterpret_cast<void*>(addr), sizeof(value));
  return thread->runtime()->newFloat(value);
}

RawObject BuiltinsModule::underGetMemberFloat(Thread* thread, Frame* frame,
                                              word nargs) {
  Arguments args(frame, nargs);
  auto addr = Int::cast(args.get(0)).asCPtr();
  float value = 0.0;
  std::memcpy(&value, reinterpret_cast<void*>(addr), sizeof(value));
  return thread->runtime()->newFloat(value);
}

RawObject BuiltinsModule::underGetMemberInt(Thread* thread, Frame* frame,
                                            word nargs) {
  Arguments args(frame, nargs);
  auto addr = Int::cast(args.get(0)).asCPtr();
  int value = 0;
  std::memcpy(&value, reinterpret_cast<void*>(addr), sizeof(value));
  return thread->runtime()->newInt(value);
}

RawObject BuiltinsModule::underGetMemberLong(Thread* thread, Frame* frame,
                                             word nargs) {
  Arguments args(frame, nargs);
  auto addr = Int::cast(args.get(0)).asCPtr();
  long value = 0;
  std::memcpy(&value, reinterpret_cast<void*>(addr), sizeof(value));
  return thread->runtime()->newInt(value);
}

RawObject BuiltinsModule::underGetMemberPyObject(Thread* thread, Frame* frame,
                                                 word nargs) {
  Arguments args(frame, nargs);
  auto addr = Int::cast(args.get(0)).asCPtr();
  auto pyobject = reinterpret_cast<PyObject**>(addr);
  if (*pyobject == nullptr) {
    if (args.get(1).isNoneType()) return NoneType::object();
    HandleScope scope(thread);
    Str name(&scope, args.get(1));
    return thread->raiseWithFmt(LayoutId::kAttributeError,
                                "Object attribute '%S' is nullptr", &name);
  }
  return ApiHandle::fromPyObject(*pyobject)->asObject();
}

RawObject BuiltinsModule::underGetMemberShort(Thread* thread, Frame* frame,
                                              word nargs) {
  Arguments args(frame, nargs);
  auto addr = Int::cast(args.get(0)).asCPtr();
  short value = 0;
  std::memcpy(&value, reinterpret_cast<void*>(addr), sizeof(value));
  return thread->runtime()->newInt(value);
}

RawObject BuiltinsModule::underGetMemberString(Thread* thread, Frame* frame,
                                               word nargs) {
  Arguments args(frame, nargs);
  auto addr = Int::cast(args.get(0)).asCPtr();
  return thread->runtime()->newStrFromCStr(*reinterpret_cast<char**>(addr));
}

RawObject BuiltinsModule::underGetMemberUByte(Thread* thread, Frame* frame,
                                              word nargs) {
  Arguments args(frame, nargs);
  auto addr = Int::cast(args.get(0)).asCPtr();
  unsigned char value = 0;
  std::memcpy(&value, reinterpret_cast<void*>(addr), sizeof(value));
  return thread->runtime()->newIntFromUnsigned(value);
}

RawObject BuiltinsModule::underGetMemberUInt(Thread* thread, Frame* frame,
                                             word nargs) {
  Arguments args(frame, nargs);
  auto addr = Int::cast(args.get(0)).asCPtr();
  unsigned int value = 0;
  std::memcpy(&value, reinterpret_cast<void*>(addr), sizeof(value));
  return thread->runtime()->newIntFromUnsigned(value);
}

RawObject BuiltinsModule::underGetMemberULong(Thread* thread, Frame* frame,
                                              word nargs) {
  Arguments args(frame, nargs);
  auto addr = Int::cast(args.get(0)).asCPtr();
  unsigned long value = 0;
  std::memcpy(&value, reinterpret_cast<void*>(addr), sizeof(value));
  return thread->runtime()->newIntFromUnsigned(value);
}

RawObject BuiltinsModule::underGetMemberUShort(Thread* thread, Frame* frame,
                                               word nargs) {
  Arguments args(frame, nargs);
  auto addr = Int::cast(args.get(0)).asCPtr();
  unsigned short value = 0;
  std::memcpy(&value, reinterpret_cast<void*>(addr), sizeof(value));
  return thread->runtime()->newIntFromUnsigned(value);
}

RawObject BuiltinsModule::underIntCheck(Thread* thread, Frame* frame,
                                        word nargs) {
  Arguments args(frame, nargs);
  return Bool::fromBool(thread->runtime()->isInstanceOfInt(args.get(0)));
}

static RawObject intOrUserSubclass(Thread* thread, const Type& type,
                                   const Object& value) {
  DCHECK(value.isSmallInt() || value.isLargeInt(),
         "builtin value should have type int");
  DCHECK(type.builtinBase() == LayoutId::kInt, "type must subclass int");
  if (type.isBuiltin()) return *value;
  HandleScope scope(thread);
  Layout layout(&scope, type.instanceLayout());
  UserIntBase instance(&scope, thread->runtime()->newInstance(layout));
  instance.setValue(*value);
  return *instance;
}

static RawObject intFromBytes(Thread* /* t */, const Bytes& bytes, word length,
                              word base) {
  DCHECK_BOUND(length, bytes.length());
  DCHECK(base == 0 || (base >= 2 && base <= 36), "invalid base");
  if (length == 0) {
    return Error::error();
  }
  unique_c_ptr<char[]> str(reinterpret_cast<char*>(std::malloc(length + 1)));
  bytes.copyTo(reinterpret_cast<byte*>(str.get()), length);
  str[length] = '\0';
  char* end;
  errno = 0;
  const word result = std::strtoll(str.get(), &end, base);
  const int saved_errno = errno;
  if (end != str.get() + length || saved_errno == EINVAL) {
    return Error::error();
  }
  if (SmallInt::isValid(result) && saved_errno != ERANGE) {
    return SmallInt::fromWord(result);
  }
  UNIMPLEMENTED("LargeInt from bytes-like");
}

RawObject BuiltinsModule::underIntFromByteArray(Thread* thread, Frame* frame,
                                                word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Type type(&scope, args.get(0));
  ByteArray array(&scope, args.get(1));
  Bytes bytes(&scope, array.bytes());
  Object base_obj(&scope, args.get(2));
  Int base_int(&scope, intUnderlying(thread, base_obj));
  DCHECK(base_int.numDigits() == 1, "invalid base");
  word base = base_int.asWord();
  Object result(&scope, intFromBytes(thread, bytes, array.numItems(), base));
  if (result.isError()) {
    Runtime* runtime = thread->runtime();
    Bytes truncated(&scope, byteArrayAsBytes(thread, runtime, array));
    Str repr(&scope, bytesReprSmartQuotes(thread, truncated));
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "invalid literal for int() with base %w: %S",
                                base, &repr);
  }
  return intOrUserSubclass(thread, type, result);
}

RawObject BuiltinsModule::underIntFromBytes(Thread* thread, Frame* frame,
                                            word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Type type(&scope, args.get(0));
  Object bytes_obj(&scope, args.get(1));
  Bytes bytes(&scope, bytesUnderlying(thread, bytes_obj));
  Object base_obj(&scope, args.get(2));
  Int base_int(&scope, intUnderlying(thread, base_obj));
  DCHECK(base_int.numDigits() == 1, "invalid base");
  word base = base_int.asWord();
  Object result(&scope, intFromBytes(thread, bytes, bytes.length(), base));
  if (result.isError()) {
    Str repr(&scope, bytesReprSmartQuotes(thread, bytes));
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "invalid literal for int() with base %w: %S",
                                base, &repr);
  }
  return intOrUserSubclass(thread, type, result);
}

RawObject BuiltinsModule::underIntFromInt(Thread* thread, Frame* frame,
                                          word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Type type(&scope, args.get(0));
  Object value(&scope, args.get(1));
  if (value.isBool()) {
    value = convertBoolToInt(*value);
  } else if (!value.isSmallInt() && !value.isLargeInt()) {
    value = intUnderlying(thread, value);
  }
  return intOrUserSubclass(thread, type, value);
}

static RawObject intFromStr(Thread* /* t */, const Str& str, word base) {
  DCHECK(base == 0 || (base >= 2 && base <= 36), "invalid base");
  if (str.length() == 0) {
    return Error::error();
  }
  unique_c_ptr<char> c_str(str.toCStr());  // for strtol()
  char* end_ptr;
  errno = 0;
  long res = std::strtol(c_str.get(), &end_ptr, base);
  int saved_errno = errno;
  bool is_complete = (*end_ptr == '\0');
  if (!is_complete || (res == 0 && saved_errno == EINVAL)) {
    return Error::error();
  }
  if (SmallInt::isValid(res) && saved_errno != ERANGE) {
    return SmallInt::fromWord(res);
  }
  UNIMPLEMENTED("LargeInt from str");
}

RawObject BuiltinsModule::underIntFromStr(Thread* thread, Frame* frame,
                                          word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Type type(&scope, args.get(0));
  Str str(&scope, args.get(1));
  Object base_obj(&scope, args.get(2));
  Int base_int(&scope, intUnderlying(thread, base_obj));
  DCHECK(base_int.numDigits() == 1, "invalid base");
  word base = base_int.asWord();
  Object result(&scope, intFromStr(thread, str, base));
  if (result.isError()) {
    Str repr(&scope, thread->invokeMethod1(str, SymbolId::kDunderRepr));
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "invalid literal for int() with base %w: %S",
                                base, &repr);
  }
  return intOrUserSubclass(thread, type, result);
}

RawObject BuiltinsModule::underListCheck(Thread* thread, Frame* frame,
                                         word nargs) {
  Arguments args(frame, nargs);
  return Bool::fromBool(thread->runtime()->isInstanceOfList(args.get(0)));
}

RawObject BuiltinsModule::underListDelItem(Thread* thread, Frame* frame,
                                           word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  List self(&scope, args.get(0));
  word length = self.numItems();
  Object index_obj(&scope, args.get(1));
  Int index_int(&scope, intUnderlying(thread, index_obj));
  word idx = index_int.asWordSaturated();
  if (idx < 0) {
    idx += length;
  }
  if (idx < 0 || idx >= length) {
    return thread->raiseWithFmt(LayoutId::kIndexError,
                                "list assignment index out of range");
  }
  listPop(thread, self, idx);
  return NoneType::object();
}

RawObject BuiltinsModule::underListDelSlice(Thread* thread, Frame* frame,
                                            word nargs) {
  // This function deletes elements that are specified by a slice by copying.
  // It compacts to the left elements in the slice range and then copies
  // elements after the slice into the free area.  The list element count is
  // decremented and elements in the unused part of the list are overwritten
  // with None.
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  List list(&scope, args.get(0));

  Object start_obj(&scope, args.get(1));
  Int start_int(&scope, intUnderlying(thread, start_obj));
  word start = start_int.asWord();

  Object stop_obj(&scope, args.get(2));
  Int stop_int(&scope, intUnderlying(thread, stop_obj));
  word stop = stop_int.asWord();

  Object step_obj(&scope, args.get(3));
  Int step_int(&scope, intUnderlying(thread, step_obj));
  // Lossy truncation of step to a word is expected.
  word step = step_int.asWordSaturated();

  word slice_length = Slice::length(start, stop, step);
  DCHECK(slice_length >= 0, "slice length should be positive");
  if (slice_length == 0) {
    // Nothing to delete
    return NoneType::object();
  }
  if (slice_length == list.numItems()) {
    // Delete all the items
    list.clearFrom(0);
    return NoneType::object();
  }
  if (step < 0) {
    // Adjust step to make iterating easier
    start = start + step * (slice_length - 1);
    step = -step;
  }
  DCHECK(start >= 0, "start should be positive");
  DCHECK(start < list.numItems(), "start should be in bounds");
  DCHECK(step <= list.numItems() || slice_length == 1,
         "Step should be in bounds or only one element should be sliced");
  // Sliding compaction of elements out of the slice to the left
  // Invariant: At each iteration of the loop, `fast` is the index of an
  // element addressed by the slice.
  // Invariant: At each iteration of the inner loop, `slow` is the index of a
  // location to where we are relocating a slice addressed element. It is *not*
  // addressed by the slice.
  word fast = start;
  for (word i = 1; i < slice_length; i++) {
    DCHECK_INDEX(fast, list.numItems());
    word slow = fast + 1;
    fast += step;
    for (; slow < fast; slow++) {
      list.atPut(slow - i, list.at(slow));
    }
  }
  // Copy elements into the space where the deleted elements were
  for (word i = fast + 1; i < list.numItems(); i++) {
    list.atPut(i - slice_length, list.at(i));
  }
  word new_length = list.numItems() - slice_length;
  DCHECK(new_length >= 0, "new_length must be positive");
  // Untrack all deleted elements
  list.clearFrom(new_length);
  return NoneType::object();
}

RawObject BuiltinsModule::underListSort(Thread* thread, Frame* frame_frame,
                                        word nargs) {
  Arguments args(frame_frame, nargs);
  HandleScope scope(thread);
  CHECK(thread->runtime()->isInstanceOfList(args.get(0)),
        "Unsupported argument type for 'ls'");
  List list(&scope, args.get(0));
  return listSort(thread, list);
}

RawObject BuiltinsModule::underPyObjectOffset(Thread* thread, Frame* frame,
                                              word nargs) {
  // TODO(eelizondo): Remove the HandleScope
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object instance_obj(&scope, args.get(0));
  Int instance(&scope, ApiHandle::getExtensionPtrAttr(thread, instance_obj));
  auto addr = reinterpret_cast<uword>(instance.asCPtr());
  addr += RawInt::cast(args.get(1)).asWord();
  return thread->runtime()->newIntFromCPtr(bit_cast<void*>(addr));
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

RawObject BuiltinsModule::setattr(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Object name(&scope, args.get(1));
  Object value(&scope, args.get(2));
  return setAttribute(thread, self, name, value);
}

RawObject BuiltinsModule::underAddress(Thread* thread, Frame* frame,
                                       word nargs) {
  Arguments args(frame, nargs);
  return thread->runtime()->newInt(args.get(0).raw());
}

RawObject BuiltinsModule::underBoundMethod(Thread* thread, Frame* frame,
                                           word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object function(&scope, args.get(0));
  Object owner(&scope, args.get(1));
  return thread->runtime()->newBoundMethod(function, owner);
}

RawObject BuiltinsModule::underPatch(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  if (nargs != 1) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "_patch expects 1 argument");
  }

  Object patch_fn_obj(&scope, args.get(0));
  if (!patch_fn_obj.isFunction()) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "_patch expects function argument");
  }
  Function patch_fn(&scope, *patch_fn_obj);
  Str fn_name(&scope, patch_fn.name());
  Runtime* runtime = thread->runtime();
  Object module_name(&scope, patch_fn.module());
  Module module(&scope, runtime->findModule(module_name));
  Object base_fn_obj(&scope, runtime->moduleAt(module, fn_name));
  if (!base_fn_obj.isFunction()) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "_patch can only patch functions");
  }
  Function base_fn(&scope, *base_fn_obj);
  copyFunctionEntries(thread, base_fn, patch_fn);
  return *patch_fn;
}

RawObject BuiltinsModule::underReprEnter(Thread* thread, Frame* frame,
                                         word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object obj(&scope, args.get(0));
  return thread->reprEnter(obj);
}

RawObject BuiltinsModule::underReprLeave(Thread* thread, Frame* frame,
                                         word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object obj(&scope, args.get(0));
  thread->reprLeave(obj);
  return NoneType::object();
}

RawObject BuiltinsModule::underSetMemberDouble(Thread*, Frame* frame,
                                               word nargs) {
  Arguments args(frame, nargs);
  auto addr = Int::cast(args.get(0)).asCPtr();
  double value = RawFloat::cast(args.get(1)).value();
  std::memcpy(reinterpret_cast<void*>(addr), &value, sizeof(value));
  return NoneType::object();
}

RawObject BuiltinsModule::underSetMemberFloat(Thread*, Frame* frame,
                                              word nargs) {
  Arguments args(frame, nargs);
  auto addr = Int::cast(args.get(0)).asCPtr();
  float value = RawFloat::cast(args.get(1)).value();
  std::memcpy(reinterpret_cast<void*>(addr), &value, sizeof(value));
  return NoneType::object();
}

RawObject BuiltinsModule::underSetMemberIntegral(Thread*, Frame* frame,
                                                 word nargs) {
  Arguments args(frame, nargs);
  auto addr = Int::cast(args.get(0)).asCPtr();
  auto value = RawInt::cast(args.get(1)).asWord();
  auto num_bytes = RawInt::cast(args.get(2)).asWord();
  std::memcpy(reinterpret_cast<void*>(addr), &value, num_bytes);
  return NoneType::object();
}

RawObject BuiltinsModule::underSetMemberPyObject(Thread* thread, Frame* frame,
                                                 word nargs) {
  Arguments args(frame, nargs);
  auto addr = Int::cast(args.get(0)).asCPtr();
  PyObject* value = ApiHandle::borrowedReference(thread, args.get(1));
  std::memcpy(reinterpret_cast<void*>(addr), &value, sizeof(value));
  return NoneType::object();
}

RawObject BuiltinsModule::underSetCheck(Thread* thread, Frame* frame,
                                        word nargs) {
  Arguments args(frame, nargs);
  return Bool::fromBool(thread->runtime()->isInstanceOfSet(args.get(0)));
}

RawObject BuiltinsModule::underSliceCheck(Thread*, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  return Bool::fromBool(args.get(0).isSlice());
}

RawObject BuiltinsModule::underStrCheck(Thread* thread, Frame* frame,
                                        word nargs) {
  Arguments args(frame, nargs);
  return Bool::fromBool(thread->runtime()->isInstanceOfStr(args.get(0)));
}

RawObject BuiltinsModule::underStrEscapeNonAscii(Thread* thread, Frame* frame,
                                                 word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  CHECK(thread->runtime()->isInstanceOfStr(args.get(0)),
        "_str_escape_non_ascii expected str instance");
  Str obj(&scope, args.get(0));
  return strEscapeNonASCII(thread, obj);
}

RawObject BuiltinsModule::underStrFind(Thread* thread, Frame* frame,
                                       word nargs) {
  Runtime* runtime = thread->runtime();
  Arguments args(frame, nargs);
  DCHECK(runtime->isInstanceOfStr(args.get(0)),
         "_str_find requires 'str' instance");
  DCHECK(runtime->isInstanceOfStr(args.get(1)),
         "_str_find requires 'str' instance");
  HandleScope scope(thread);
  Str haystack(&scope, args.get(0));
  Str needle(&scope, args.get(1));
  Object start_obj(&scope, args.get(2));
  Object end_obj(&scope, args.get(3));
  word start = 0;
  if (!start_obj.isNoneType()) {
    Int start_int(&scope, intUnderlying(thread, start_obj));
    start = start_int.asWordSaturated();
  }
  word end = kMaxWord;
  if (!end_obj.isNoneType()) {
    Int end_int(&scope, intUnderlying(thread, end_obj));
    end = end_int.asWordSaturated();
  }
  return strFind(haystack, needle, start, end);
}

RawObject BuiltinsModule::underStrFromStr(Thread* thread, Frame* frame,
                                          word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Type type(&scope, args.get(0));
  DCHECK(type.builtinBase() == LayoutId::kStr, "type must subclass str");
  Object value_obj(&scope, args.get(1));
  Str value(&scope, strUnderlying(thread, value_obj));
  if (type.isBuiltin()) return *value;
  Layout type_layout(&scope, type.instanceLayout());
  UserStrBase instance(&scope, thread->runtime()->newInstance(type_layout));
  instance.setValue(*value);
  return *instance;
}

RawObject BuiltinsModule::underStrReplace(Thread* thread, Frame* frame,
                                          word nargs) {
  Runtime* runtime = thread->runtime();
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Object oldstr_obj(&scope, args.get(1));
  Object newstr_obj(&scope, args.get(2));
  Str self(&scope, strUnderlying(thread, self_obj));
  Str oldstr(&scope, strUnderlying(thread, oldstr_obj));
  Str newstr(&scope, strUnderlying(thread, newstr_obj));
  Object count_obj(&scope, args.get(3));
  Int count(&scope, intUnderlying(thread, count_obj));
  return runtime->strReplace(thread, self, oldstr, newstr,
                             count.asWordSaturated());
}

RawObject BuiltinsModule::underStrRFind(Thread* thread, Frame* frame,
                                        word nargs) {
  Runtime* runtime = thread->runtime();
  Arguments args(frame, nargs);
  DCHECK(runtime->isInstanceOfStr(args.get(0)),
         "_str_rfind requires 'str' instance");
  DCHECK(runtime->isInstanceOfStr(args.get(1)),
         "_str_rfind requires 'str' instance");
  HandleScope scope(thread);
  Str haystack(&scope, args.get(0));
  Str needle(&scope, args.get(1));
  Object start_obj(&scope, args.get(2));
  Object end_obj(&scope, args.get(3));
  word start = 0;
  if (!start_obj.isNoneType()) {
    Int start_int(&scope, intUnderlying(thread, start_obj));
    start = start_int.asWordSaturated();
  }
  word end = kMaxWord;
  if (!end_obj.isNoneType()) {
    Int end_int(&scope, intUnderlying(thread, end_obj));
    end = end_int.asWordSaturated();
  }
  return strRFind(haystack, needle, start, end);
}

RawObject BuiltinsModule::underStrSplitlines(Thread* thread, Frame* frame,
                                             word nargs) {
  Runtime* runtime = thread->runtime();
  Arguments args(frame, nargs);
  DCHECK(runtime->isInstanceOfStr(args.get(0)),
         "_str_splitlines requires 'str' instance");
  DCHECK(runtime->isInstanceOfInt(args.get(1)),
         "_str_splitlines requires 'int' instance");
  HandleScope scope(thread);
  Str self(&scope, args.get(0));
  Object keepends_obj(&scope, args.get(1));
  Int keepends_int(&scope, intUnderlying(thread, keepends_obj));
  bool keepends = !keepends_int.isZero();
  return strSplitlines(thread, self, keepends);
}

RawObject BuiltinsModule::underTupleCheck(Thread* thread, Frame* frame,
                                          word nargs) {
  Arguments args(frame, nargs);
  return Bool::fromBool(thread->runtime()->isInstanceOfTuple(args.get(0)));
}

RawObject BuiltinsModule::underTypeCheck(Thread* thread, Frame* frame,
                                         word nargs) {
  Arguments args(frame, nargs);
  return Bool::fromBool(thread->runtime()->isInstanceOfType(args.get(0)));
}

RawObject BuiltinsModule::underTypeCheckExact(Thread*, Frame* frame,
                                              word nargs) {
  Arguments args(frame, nargs);
  return Bool::fromBool(args.get(0).isType());
}

RawObject BuiltinsModule::underTypeIsSubclass(Thread* thread, Frame* frame,
                                              word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Type subclass(&scope, args.get(0));
  Type superclass(&scope, args.get(1));
  return Bool::fromBool(thread->runtime()->isSubclass(subclass, superclass));
}

RawObject BuiltinsModule::underUnimplemented(Thread* thread, Frame* frame,
                                             word) {
  python::Utils::printTraceback();

  // Attempt to identify the calling function.
  HandleScope scope(thread);
  Object function_obj(&scope, frame->previousFrame()->function());
  if (!function_obj.isError()) {
    Function function(&scope, *function_obj);
    Str function_name(&scope, function.name());
    unique_c_ptr<char> name_cstr(function_name.toCStr());
    fprintf(stderr, "\n'_unimplemented' called in function '%s'.\n",
            name_cstr.get());
  } else {
    fputs("\n'_unimplemented' called.\n", stderr);
  }

  std::abort();
}

}  // namespace python
