#include "builtins-module.h"

#include "bytearray-builtins.h"
#include "bytes-builtins.h"
#include "complex-builtins.h"
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
    return thread->raiseTypeErrorWithCStr(
        "getattr(): attribute name must be string");
  }
  return runtime->attributeAt(thread, self, name);
}

RawObject hasAttribute(Thread* thread, const Object& self, const Object& name) {
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfStr(*name)) {
    return thread->raiseTypeErrorWithCStr(
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

  return Error::object();
}

RawObject setAttribute(Thread* thread, const Object& self, const Object& name,
                       const Object& value) {
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfStr(*name)) {
    return thread->raiseTypeErrorWithCStr(
        "setattr(): attribute name must be string");
  }
  return runtime->attributeAtPut(thread, self, name, value);
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
    {SymbolId::kIsInstance, isinstance},
    {SymbolId::kIsSubclass, issubclass},
    {SymbolId::kOrd, ord},
    {SymbolId::kSetattr, setattr},
    {SymbolId::kUnderAddress, underAddress},
    {SymbolId::kUnderByteArrayJoin, ByteArrayBuiltins::join},
    {SymbolId::kUnderBytesFromInts, underBytesFromInts},
    {SymbolId::kUnderBytesGetitem, underBytesGetItem},
    {SymbolId::kUnderBytesGetitemSlice, underBytesGetItemSlice},
    {SymbolId::kUnderBytesJoin, BytesBuiltins::join},
    {SymbolId::kUnderBytesMaketrans, underBytesMaketrans},
    {SymbolId::kUnderBytesRepeat, underBytesRepeat},
    {SymbolId::kUnderComplexImag, complexGetImag},
    {SymbolId::kUnderComplexReal, complexGetReal},
    {SymbolId::kUnderListSort, underListSort},
    {SymbolId::kUnderReprEnter, underReprEnter},
    {SymbolId::kUnderReprLeave, underReprLeave},
    {SymbolId::kUnderStrEscapeNonAscii, underStrEscapeNonAscii},
    {SymbolId::kUnderStrFind, underStrFind},
    {SymbolId::kUnderStrReplace, underStrReplace},
    {SymbolId::kUnderStrRFind, underStrRFind},
    {SymbolId::kUnderStructseqGetAttr, underStructseqGetAttr},
    {SymbolId::kUnderStructseqSetAttr, underStructseqSetAttr},
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
    {SymbolId::kLargeInt, LayoutId::kLargeInt},
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
    {SymbolId::kSmallInt, LayoutId::kSmallInt},
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
         RawTuple::cast(code.consts()).at(const_loaded).isNoneType() &&
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
  base.setCode(*patch_code);
  patch.setEntry(base.entry());
  patch.setEntryKw(base.entryKw());
  patch.setEntryEx(base.entryEx());
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
    Object patch_obj(&scope, RawValueCell::cast(*patch_value_cell).value());

    // Copy function entries if the method already exists as a native builtin.
    Object base_obj(&scope, runtime->typeDictAt(base, key));
    if (!base_obj.isError()) {
      CHECK(patch_obj.isFunction(), "Python should only annotate functions");
      Function patch_fn(&scope, *patch_obj);
      CHECK(base_obj.isFunction(),
            "Python annotation of non-function native object");
      Function base_fn(&scope, *base_obj);

      copyFunctionEntries(thread, base_fn, patch_fn);
    }
    runtime->typeDictAtPut(base, key, patch_obj);
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
      return thread->raiseTypeErrorWithCStr(
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
    return thread->raiseTypeErrorWithCStr(
        "__build_class__: func must be a function");
  }
  Function body(&scope, *body_obj);
  Object name(&scope, args.get(1));
  if (!runtime->isInstanceOfStr(*name)) {
    return thread->raiseTypeErrorWithCStr(
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
    Object type_obj(&scope, runtime->moduleDictAt(globals, name));
    CHECK(type_obj.isType(),
          "Name '%s' is not bound to a type object. "
          "You may need to add it to the builtins module.",
          RawStr::cast(*name).toCStr());
    Type type(&scope, *type_obj);
    Dict type_dict(&scope, type.dict());

    Dict patch_type(&scope, runtime->newDict());
    thread->runClassFunction(body, patch_type);
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
      return thread->raiseTypeError(runtime->newStrFromFmt(
          "%S.__prepare__() must return a mapping, not %T",
          &metaclass_type_name, &dict_obj));
    }
    return thread->raiseTypeError(runtime->newStrFromFmt(
        "<metaclass>.__prepare__() must return a mapping, not %T", &dict_obj));
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

RawObject BuiltinsModule::chr(Thread* thread, Frame* frame_frame, word nargs) {
  Arguments args(frame_frame, nargs);
  RawObject arg = args.get(0);
  if (!arg.isSmallInt()) {
    return thread->raiseTypeErrorWithCStr("Unsupported type in builtin 'chr'");
  }
  word w = RawSmallInt::cast(arg).value();
  const char s[2]{static_cast<char>(w), 0};
  return SmallStr::fromCStr(s);
}

static RawObject compileToBytecode(Thread* thread, const char* source) {
  HandleScope scope(thread);
  std::unique_ptr<char[]> bytecode_str(Runtime::compileFromCStr(source));
  Marshal::Reader reader(&scope, thread->runtime(), bytecode_str.get());
  reader.readLong();  // magic
  reader.readLong();  // mtime
  reader.readLong();  // size
  return reader.readObject();
}

static RawObject compileBytes(Thread* thread, const Bytes& source) {
  word bytes_len = source.length();
  unique_c_ptr<byte[]> source_bytes(
      static_cast<byte*>(std::malloc(bytes_len + 1)));
  source.copyTo(source_bytes.get(), bytes_len);
  source_bytes[bytes_len] = '\0';
  return compileToBytecode(thread, reinterpret_cast<char*>(source_bytes.get()));
}

static RawObject compileStr(Thread* thread, const Str& source) {
  unique_c_ptr<char[]> source_str(source.toCStr());
  return compileToBytecode(thread, source_str.get());
}

RawObject BuiltinsModule::compile(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  // TODO(T40808881): Add compile support for bytearray, buffer, and subclasses
  Object data(&scope, args.get(0));
  if (!data.isStr() && !data.isBytes()) {
    return thread->raiseTypeErrorWithCStr(
        "compile() currently only supports a str or bytes source");
  }
  Str filename(&scope, args.get(1));
  Str mode(&scope, args.get(2));
  // TODO(emacs): Refactor into sane argument-fetching code
  if (args.get(3) != SmallInt::fromWord(0)) {  // not the default
    return thread->raiseTypeErrorWithCStr(
        "compile() does not yet support user-supplied flags");
  }
  // TODO(T40872645): Add support for compiler flag forwarding
  if (args.get(4) == Bool::falseObj()) {
    return thread->raiseTypeErrorWithCStr(
        "compile() does not yet support compiler flag forwarding");
  }
  if (args.get(5) != SmallInt::fromWord(-1)) {  // not the default
    return thread->raiseTypeErrorWithCStr(
        "compile() does not yet support user-supplied optimize");
  }
  // Note: mode doesn't actually do anything yet.
  if (!mode.equalsCStr("exec") && !mode.equalsCStr("eval") &&
      !mode.equalsCStr("single")) {
    return thread->raiseValueErrorWithCStr(
        "Expected mode to be 'exec', 'eval', or 'single' in 'compile'");
  }

  Object code_obj(&scope, NoneType::object());
  if (data.isStr()) {
    Str source_str(&scope, *data);
    code_obj = compileStr(thread, source_str);
  } else {
    Bytes source_bytes(&scope, *data);
    code_obj = compileBytes(thread, source_bytes);
  }
  Code code(&scope, *code_obj);
  code.setFilename(*filename);
  return *code;
}

RawObject BuiltinsModule::divmod(Thread*, Frame*, word) {
  UNIMPLEMENTED("divmod(a, b)");
}

RawObject BuiltinsModule::exec(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object source_obj(&scope, args.get(0));
  if (!source_obj.isCode() && !source_obj.isStr()) {
    return thread->raiseTypeErrorWithCStr(
        "Expected 'source' to be str or code in 'exec'");
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
      return thread->raiseTypeErrorWithCStr(
          "Expected 'globals' to be dict in 'exec'");
    }
    locals = *globals_obj;
  } else {  // both globals and locals are provided
    if (!runtime->isInstanceOfDict(*globals_obj)) {
      return thread->raiseTypeErrorWithCStr(
          "Expected 'globals' to be dict in 'exec'");
    }
    if (!runtime->isMapping(thread, locals)) {
      return thread->raiseTypeErrorWithCStr(
          "Expected 'locals' to be a mapping in 'exec'");
    }
    // TODO(T37888835): Fix 3 argument case
    UNIMPLEMENTED("exec() with both globals and locals");
  }
  if (source_obj.isStr()) {
    Str source(&scope, *source_obj);
    source_obj = compileStr(thread, source);
    DCHECK(source_obj.isCode(), "compileStr must return code object");
  }
  Code code(&scope, *source_obj);
  if (code.numFreevars() != 0) {
    return thread->raiseTypeErrorWithCStr(
        "Expected 'source' not to have free variables in 'exec'");
  }
  Dict globals(&scope, *globals_obj);
  return thread->exec(code, globals, locals);
}

static RawObject isinstanceImpl(Thread* thread, const Object& obj,
                                const Object& type_obj) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  if (runtime->isInstanceOfType(*type_obj)) {
    Type type(&scope, *type_obj);
    return Bool::fromBool(runtime->isInstance(obj, type));
  }

  if (runtime->isInstanceOfTuple(*type_obj)) {
    Tuple types(&scope, *type_obj);
    Object elem(&scope, NoneType::object());
    Object result(&scope, NoneType::object());
    for (word i = 0, len = types.length(); i < len; i++) {
      elem = types.at(i);
      result = isinstanceImpl(thread, obj, elem);
      if (result.isError() || result == Bool::trueObj()) return *result;
    }
    return Bool::falseObj();
  }

  return thread->raiseTypeErrorWithCStr(
      "isinstance() arg 2 must be a type or tuple of types");
}

// TODO(mpage): isinstance (somewhat unsurprisingly at this point I guess) is
// actually far more complicated than one might expect. This is enough to get
// richards working.
RawObject BuiltinsModule::isinstance(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object obj(&scope, args.get(0));
  Object type(&scope, args.get(1));
  return isinstanceImpl(thread, obj, type);
}

RawObject BuiltinsModule::issubclass(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object type_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfType(*type_obj)) {
    return thread->raiseTypeErrorWithCStr("issubclass() arg 1 must be a class");
  }
  Type type(&scope, *type_obj);
  Object classinfo(&scope, args.get(1));
  if (runtime->isInstanceOfType(*classinfo)) {
    Type possible_superclass(&scope, *classinfo);
    return Bool::fromBool(runtime->isSubclass(type, possible_superclass));
  }
  // If classinfo is not a tuple, then throw a TypeError.
  if (!classinfo.isTuple()) {
    return thread->raiseTypeErrorWithCStr(
        "issubclass() arg 2 must be a class of tuple of classes");
  }
  // If classinfo is a tuple, try each of the values, and return
  // True if the first argument is a subclass of any of them.
  Tuple tuple_of_types(&scope, *classinfo);
  for (word i = 0; i < tuple_of_types.length(); i++) {
    // If any argument is not a type, then throw TypeError.
    if (!runtime->isInstanceOfType(tuple_of_types.at(i))) {
      return thread->raiseTypeErrorWithCStr(
          "issubclass() arg 2 must be a class of tuple of classes");
    }
    Type possible_superclass(&scope, tuple_of_types.at(i));
    // If any of the types are a superclass, return true.
    if (runtime->isSubclass(type, possible_superclass)) return Bool::trueObj();
  }
  // None of the types in the tuple were a superclass, so return false.
  return Bool::falseObj();
}

RawObject BuiltinsModule::ord(Thread* thread, Frame* frame_frame, word nargs) {
  Arguments args(frame_frame, nargs);
  RawObject arg = args.get(0);
  if (!arg.isStr()) {
    return thread->raiseTypeErrorWithCStr("Unsupported type in builtin 'ord'");
  }
  auto str = RawStr::cast(arg);
  word num_bytes;
  int32 codepoint = str.codePointAt(0, &num_bytes);
  if (num_bytes != str.length()) {
    return thread->raiseTypeErrorWithCStr(
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
  if (level.isInt() && RawInt::cast(*level).isZero()) {
    Object cached_module(&scope, runtime->findModule(name));
    if (!cached_module.isNoneType()) {
      return *cached_module;
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
    return thread->raiseTypeError(
        runtime->newStrFromFmt("cannot convert '%T' object to bytes", &src));
  }
  // Slow path: iterate over source in Python, collect into list, and call again
  return NoneType::object();
}

RawObject BuiltinsModule::underBytesGetItem(Thread* thread, Frame* frame,
                                            word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Bytes self(&scope, args.get(0));
  Int key(&scope, args.get(1));
  word index = key.asWordSaturated();
  if (!SmallInt::isValid(index)) {
    return thread->raiseIndexError(thread->runtime()->newStrFromFmt(
        "cannot fit '%T' into an index-sized integer", &key));
  }
  word length = self.length();
  if (index < 0) {
    index += length;
  }
  if (index < 0 || index >= length) {
    return thread->raiseIndexErrorWithCStr("index out of range");
  }
  return SmallInt::fromWord(self.byteAt(index));
}

RawObject BuiltinsModule::underBytesGetItemSlice(Thread* thread, Frame* frame,
                                                 word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Bytes self(&scope, args.get(0));
  Int start(&scope, args.get(1));
  Int stop(&scope, args.get(2));
  Int step(&scope, args.get(3));
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
    Bytes bytes(&scope, *from_obj);
    length = bytes.length();
  } else if (runtime->isInstanceOfByteArray(*from_obj)) {
    ByteArray array(&scope, *from_obj);
    length = array.numItems();
    from_obj = array.bytes();
  } else {
    UNIMPLEMENTED("bytes-like other than bytes or bytearray");
  }
  if (runtime->isInstanceOfBytes(*to_obj)) {
    Bytes bytes(&scope, *to_obj);
    DCHECK(bytes.length() == length, "lengths should already be the same");
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
  Bytes self(&scope, args.get(0));
  Int num(&scope, args.get(1));
  word count = num.asWordSaturated();
  if (!SmallInt::isValid(count)) {
    return thread->raiseOverflowError(thread->runtime()->newStrFromFmt(
        "cannot fit '%T' into an index-sized integer", &num));
  }
  // NOTE: unlike __mul__, we raise a value error for negative count
  if (count < 0) {
    return thread->raiseValueErrorWithCStr("negative count");
  }
  return thread->runtime()->bytesRepeat(thread, self, self.length(), count);
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

RawObject BuiltinsModule::underPatch(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("_patch expects 1 argument");
  }

  Object patch_fn_obj(&scope, args.get(0));
  if (!patch_fn_obj.isFunction()) {
    return thread->raiseTypeErrorWithCStr("_patch expects function argument");
  }
  Function patch_fn(&scope, *patch_fn_obj);
  Str fn_name(&scope, patch_fn.name());
  Runtime* runtime = thread->runtime();
  Object module_name(&scope, patch_fn.module());
  Module module(&scope, runtime->findModule(module_name));
  Object base_fn_obj(&scope, runtime->moduleAt(module, fn_name));
  if (!base_fn_obj.isFunction()) {
    return thread->raiseTypeErrorWithCStr("_patch can only patch functions");
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
  word start =
      start_obj.isNoneType() ? 0 : RawInt::cast(*start_obj).asWordSaturated();
  word end = end_obj.isNoneType() ? kMaxWord
                                  : RawInt::cast(*end_obj).asWordSaturated();
  return strFind(haystack, needle, start, end);
}

RawObject BuiltinsModule::underStrReplace(Thread* thread, Frame* frame,
                                          word nargs) {
  Runtime* runtime = thread->runtime();
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  if (!args.get(0).isStr()) UNIMPLEMENTED("str subclass");
  if (!args.get(1).isStr()) UNIMPLEMENTED("str subclass");
  if (!args.get(2).isStr()) UNIMPLEMENTED("str subclass");
  Str self(&scope, args.get(0));
  Str oldstr(&scope, args.get(1));
  Str newstr(&scope, args.get(2));
  Int count(&scope, args.get(3));
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
  word start =
      start_obj.isNoneType() ? 0 : RawInt::cast(*start_obj).asWordSaturated();
  word end = end_obj.isNoneType() ? kMaxWord
                                  : RawInt::cast(*end_obj).asWordSaturated();
  return strRFind(haystack, needle, start, end);
}

RawObject BuiltinsModule::underUnimplemented(Thread*, Frame*, word) {
  fputs("Unimplemented function called at:\n", stderr);
  python::Utils::printTraceback();
  std::abort();
}

}  // namespace python
