#include "exception-builtins.h"

#include <unistd.h>

#include <cerrno>
#include <cinttypes>

#include "builtins-module.h"
#include "builtins.h"
#include "frame.h"
#include "module-builtins.h"
#include "object-builtins.h"
#include "objects.h"
#include "runtime.h"
#include "set-builtins.h"
#include "sys-module.h"
#include "traceback-builtins.h"
#include "tuple-builtins.h"
#include "type-builtins.h"

namespace py {

static const BuiltinAttribute kBaseExceptionAttributes[] = {
    {ID(args), RawBaseException::kArgsOffset},
    {ID(__traceback__), RawBaseException::kTracebackOffset},
    {ID(__cause__), RawBaseException::kCauseOffset},
    {ID(__context__), RawBaseException::kContextOffset},
    {ID(__suppress_context__), RawBaseException::kSuppressContextOffset},
};

static const BuiltinAttribute kImportErrorAttributes[] = {
    {ID(msg), RawImportError::kMsgOffset},
    {ID(name), RawImportError::kNameOffset},
    {ID(path), RawImportError::kPathOffset},
};

static const BuiltinAttribute kStopIterationAttributes[] = {
    {ID(value), RawStopIteration::kValueOffset},
};

static const BuiltinAttribute kSyntaxErrorAttributes[] = {
    {ID(filename), RawSyntaxError::kFilenameOffset},
    {ID(lineno), RawSyntaxError::kLinenoOffset},
    {ID(msg), RawSyntaxError::kMsgOffset},
    {ID(offset), RawSyntaxError::kOffsetOffset},
    {ID(print_file_and_line), RawSyntaxError::kPrintFileAndLineOffset},
    {ID(text), RawSyntaxError::kTextOffset},
};

static const BuiltinAttribute kSystemExitAttributes[] = {
    {ID(value), RawSystemExit::kCodeOffset},
};

static const BuiltinAttribute kUnicodeErrorBaseAttributes[] = {
    {ID(encoding), RawUnicodeErrorBase::kEncodingOffset},
    {ID(object), RawUnicodeErrorBase::kObjectOffset},
    {ID(start), RawUnicodeErrorBase::kStartOffset},
    {ID(end), RawUnicodeErrorBase::kEndOffset},
    {ID(reason), RawUnicodeErrorBase::kReasonOffset},
};

struct ExceptionTypeSpec {
  SymbolId name;
  LayoutId layout_id;
  LayoutId superclass_id;
  View<BuiltinAttribute> attributes;
};

static const ExceptionTypeSpec kExceptionSpecs[] = {
    {ID(BaseException), LayoutId::kBaseException, LayoutId::kObject,
     kBaseExceptionAttributes},
    {ID(Exception), LayoutId::kException, LayoutId::kBaseException,
     kNoAttributes},
    {ID(KeyboardInterrupt), LayoutId::kKeyboardInterrupt,
     LayoutId::kBaseException, kNoAttributes},
    {ID(GeneratorExit), LayoutId::kGeneratorExit, LayoutId::kBaseException,
     kNoAttributes},
    {ID(SystemExit), LayoutId::kSystemExit, LayoutId::kBaseException,
     kSystemExitAttributes},
    {ID(ArithmeticError), LayoutId::kArithmeticError, LayoutId::kException,
     kNoAttributes},
    {ID(AssertionError), LayoutId::kAssertionError, LayoutId::kException,
     kNoAttributes},
    {ID(AttributeError), LayoutId::kAttributeError, LayoutId::kException,
     kNoAttributes},
    {ID(BufferError), LayoutId::kBufferError, LayoutId::kException,
     kNoAttributes},
    {ID(EOFError), LayoutId::kEOFError, LayoutId::kException, kNoAttributes},
    {ID(ImportError), LayoutId::kImportError, LayoutId::kException,
     kImportErrorAttributes},
    {ID(LookupError), LayoutId::kLookupError, LayoutId::kException,
     kNoAttributes},
    {ID(MemoryError), LayoutId::kMemoryError, LayoutId::kException,
     kNoAttributes},
    {ID(NameError), LayoutId::kNameError, LayoutId::kException, kNoAttributes},
    {ID(OSError), LayoutId::kOSError, LayoutId::kException, kNoAttributes},
    {ID(ReferenceError), LayoutId::kReferenceError, LayoutId::kException,
     kNoAttributes},
    {ID(RuntimeError), LayoutId::kRuntimeError, LayoutId::kException,
     kNoAttributes},
    {ID(StopIteration), LayoutId::kStopIteration, LayoutId::kException,
     kStopIterationAttributes},
    {ID(StopAsyncIteration), LayoutId::kStopAsyncIteration,
     LayoutId::kException, kNoAttributes},
    {ID(SyntaxError), LayoutId::kSyntaxError, LayoutId::kException,
     kSyntaxErrorAttributes},
    {ID(SystemError), LayoutId::kSystemError, LayoutId::kException,
     kNoAttributes},
    {ID(TypeError), LayoutId::kTypeError, LayoutId::kException, kNoAttributes},
    {ID(ValueError), LayoutId::kValueError, LayoutId::kException,
     kNoAttributes},
    {ID(Warning), LayoutId::kWarning, LayoutId::kException, kNoAttributes},
    {ID(FloatingPointError), LayoutId::kFloatingPointError,
     LayoutId::kArithmeticError, kNoAttributes},
    {ID(OverflowError), LayoutId::kOverflowError, LayoutId::kArithmeticError,
     kNoAttributes},
    {ID(ZeroDivisionError), LayoutId::kZeroDivisionError,
     LayoutId::kArithmeticError, kNoAttributes},
    {ID(ModuleNotFoundError), LayoutId::kModuleNotFoundError,
     LayoutId::kImportError, kNoAttributes},
    {ID(IndexError), LayoutId::kIndexError, LayoutId::kLookupError,
     kNoAttributes},
    {ID(KeyError), LayoutId::kKeyError, LayoutId::kLookupError, kNoAttributes},
    {ID(UnboundLocalError), LayoutId::kUnboundLocalError, LayoutId::kNameError,
     kNoAttributes},
    {ID(BlockingIOError), LayoutId::kBlockingIOError, LayoutId::kOSError,
     kNoAttributes},
    {ID(ChildProcessError), LayoutId::kChildProcessError, LayoutId::kOSError,
     kNoAttributes},
    {ID(ConnectionError), LayoutId::kConnectionError, LayoutId::kOSError,
     kNoAttributes},
    {ID(FileExistsError), LayoutId::kFileExistsError, LayoutId::kOSError,
     kNoAttributes},
    {ID(FileNotFoundError), LayoutId::kFileNotFoundError, LayoutId::kOSError,
     kNoAttributes},
    {ID(InterruptedError), LayoutId::kInterruptedError, LayoutId::kOSError,
     kNoAttributes},
    {ID(IsADirectoryError), LayoutId::kIsADirectoryError, LayoutId::kOSError,
     kNoAttributes},
    {ID(NotADirectoryError), LayoutId::kNotADirectoryError, LayoutId::kOSError,
     kNoAttributes},
    {ID(PermissionError), LayoutId::kPermissionError, LayoutId::kOSError,
     kNoAttributes},
    {ID(ProcessLookupError), LayoutId::kProcessLookupError, LayoutId::kOSError,
     kNoAttributes},
    {ID(TimeoutError), LayoutId::kTimeoutError, LayoutId::kOSError,
     kNoAttributes},
    {ID(BrokenPipeError), LayoutId::kBrokenPipeError,
     LayoutId::kConnectionError, kNoAttributes},
    {ID(ConnectionAbortedError), LayoutId::kConnectionAbortedError,
     LayoutId::kConnectionError, kNoAttributes},
    {ID(ConnectionRefusedError), LayoutId::kConnectionRefusedError,
     LayoutId::kConnectionError, kNoAttributes},
    {ID(ConnectionResetError), LayoutId::kConnectionResetError,
     LayoutId::kConnectionError, kNoAttributes},
    {ID(NotImplementedError), LayoutId::kNotImplementedError,
     LayoutId::kRuntimeError, kNoAttributes},
    {ID(RecursionError), LayoutId::kRecursionError, LayoutId::kRuntimeError,
     kNoAttributes},
    {ID(IndentationError), LayoutId::kIndentationError, LayoutId::kSyntaxError,
     kNoAttributes},
    {ID(TabError), LayoutId::kTabError, LayoutId::kIndentationError,
     kNoAttributes},
    {ID(UserWarning), LayoutId::kUserWarning, LayoutId::kWarning,
     kNoAttributes},
    {ID(DeprecationWarning), LayoutId::kDeprecationWarning, LayoutId::kWarning,
     kNoAttributes},
    {ID(PendingDeprecationWarning), LayoutId::kPendingDeprecationWarning,
     LayoutId::kWarning, kNoAttributes},
    {ID(SyntaxWarning), LayoutId::kSyntaxWarning, LayoutId::kWarning,
     kNoAttributes},
    {ID(RuntimeWarning), LayoutId::kRuntimeWarning, LayoutId::kWarning,
     kNoAttributes},
    {ID(FutureWarning), LayoutId::kFutureWarning, LayoutId::kWarning,
     kNoAttributes},
    {ID(ImportWarning), LayoutId::kImportWarning, LayoutId::kWarning,
     kNoAttributes},
    {ID(UnicodeWarning), LayoutId::kUnicodeWarning, LayoutId::kWarning,
     kNoAttributes},
    {ID(BytesWarning), LayoutId::kBytesWarning, LayoutId::kWarning,
     kNoAttributes},
    {ID(ResourceWarning), LayoutId::kResourceWarning, LayoutId::kWarning,
     kNoAttributes},
    {ID(UnicodeError), LayoutId::kUnicodeError, LayoutId::kValueError,
     kNoAttributes},
    {ID(UnicodeDecodeError), LayoutId::kUnicodeDecodeError,
     LayoutId::kUnicodeError, kUnicodeErrorBaseAttributes},
    {ID(UnicodeEncodeError), LayoutId::kUnicodeEncodeError,
     LayoutId::kUnicodeError, kUnicodeErrorBaseAttributes},
    {ID(UnicodeTranslateError), LayoutId::kUnicodeTranslateError,
     LayoutId::kUnicodeError, kUnicodeErrorBaseAttributes},
};

LayoutId errorLayoutFromErrno(int errno_value) {
  switch (errno_value) {
    case EACCES:
      return LayoutId::kPermissionError;
    case EAGAIN:  // duplicates EWOULDBLOCK
      return LayoutId::kBlockingIOError;
    case EALREADY:
      return LayoutId::kBlockingIOError;
    case EINPROGRESS:
      return LayoutId::kBlockingIOError;
    case ECHILD:
      return LayoutId::kChildProcessError;
    case ECONNABORTED:
      return LayoutId::kConnectionAbortedError;
    case ECONNREFUSED:
      return LayoutId::kConnectionRefusedError;
    case ECONNRESET:
      return LayoutId::kConnectionResetError;
    case EEXIST:
      return LayoutId::kFileExistsError;
    case ENOENT:
      return LayoutId::kFileNotFoundError;
    case EINTR:
      return LayoutId::kInterruptedError;
    case EISDIR:
      return LayoutId::kIsADirectoryError;
    case ENOTDIR:
      return LayoutId::kNotADirectoryError;
    case EPERM:
      return LayoutId::kPermissionError;
    case EPIPE:
      return LayoutId::kBrokenPipeError;
    case ESRCH:
      return LayoutId::kProcessLookupError;
    case ETIMEDOUT:
      return LayoutId::kTimeoutError;
    default:
      return LayoutId::kOSError;
  }
}

bool givenExceptionMatches(Thread* thread, const Object& given,
                           const Object& exc) {
  HandleScope scope(thread);
  if (exc.isTuple()) {
    Tuple tuple(&scope, *exc);
    Object item(&scope, NoneType::object());
    for (word i = 0; i < tuple.length(); i++) {
      item = tuple.at(i);
      if (givenExceptionMatches(thread, given, item)) {
        return true;
      }
    }
    return false;
  }
  Runtime* runtime = thread->runtime();
  Object given_type(&scope, *given);
  if (runtime->isInstanceOfBaseException(*given_type)) {
    given_type = runtime->typeOf(*given);
  }
  if (runtime->isInstanceOfType(*given_type) &&
      runtime->isInstanceOfType(*exc)) {
    Type subtype(&scope, *given_type);
    Type supertype(&scope, *exc);
    if (subtype.isBaseExceptionSubclass() &&
        supertype.isBaseExceptionSubclass()) {
      return typeIsSubclass(subtype, supertype);
    }
  }
  return *given_type == *exc;
}

RawObject createException(Thread* thread, const Type& type,
                          const Object& value) {
  if (value.isNoneType()) {
    return Interpreter::call0(thread, type);
  }
  if (thread->runtime()->isInstanceOfTuple(*value)) {
    HandleScope scope(thread);
    thread->stackPush(*type);
    Tuple args(&scope, tupleUnderlying(*value));
    word length = args.length();
    for (word i = 0; i < length; i++) {
      thread->stackPush(args.at(i));
    }
    return Interpreter::call(thread, /*nargs=*/length);
  }
  return Interpreter::call1(thread, type, value);
}

void normalizeException(Thread* thread, Object* exc, Object* val,
                        Object* traceback) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  auto normalize = [&] {
    if (!runtime->isInstanceOfType(**exc)) return true;
    Type type(&scope, **exc);
    if (!type.isBaseExceptionSubclass()) return true;
    Object value(&scope, **val);
    Type value_type(&scope, runtime->typeOf(*value));

    // TODO(bsimmers): Extend this to support all the weird cases allowed by
    // PyObject_IsSubclass.
    if (!typeIsSubclass(value_type, type)) {
      // value isn't an instance of type. Replace it with type(value).
      value = createException(thread, type, value);
      if (value.isError()) return false;
      *val = *value;
    } else if (*value_type != *type) {
      // value_type is more specific than type, so use it instead.
      *exc = *value_type;
    }

    return true;
  };

  // If a new exception is raised during normalization, attempt to normalize
  // that exception. If this process repeats too many times, give up and throw a
  // RecursionError. If even that exception fails to normalize, abort.
  const int normalize_limit = 32;
  for (word i = 0; i <= normalize_limit; i++) {
    if (normalize()) return;

    if (i == normalize_limit - 1) {
      thread->raiseWithFmt(
          LayoutId::kRecursionError,
          "maximum recursion depth exceeded while normalizing an exception");
    }

    *exc = thread->pendingExceptionType();
    *val = thread->pendingExceptionValue();
    Object new_tb(&scope, thread->pendingExceptionTraceback());
    if (!new_tb.isNoneType()) *traceback = *new_tb;
    thread->clearPendingException();
  }

  if (runtime->isInstanceOfType(**exc)) {
    Type type(&scope, **exc);
    if (type.builtinBase() == LayoutId::kMemoryError) {
      UNIMPLEMENTED(
          "Cannot recover from MemoryErrors while normalizing exceptions.");
    }
    UNIMPLEMENTED(
        "Cannot recover from the recursive normalization of an exception.");
  }
}

static void printPendingExceptionImpl(Thread* thread, bool set_sys_last_vars) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object type(&scope, thread->pendingExceptionType());
  Object system_exit(&scope, runtime->typeAt(LayoutId::kSystemExit));
  if (givenExceptionMatches(thread, type, system_exit)) {
    handleSystemExit(thread);
  }

  Object value(&scope, thread->pendingExceptionValue());
  Object tb(&scope, thread->pendingExceptionTraceback());
  thread->clearPendingException();
  if (type.isNoneType()) return;

  normalizeException(thread, &type, &value, &tb);
  BaseException exc(&scope, *value);
  exc.setTraceback(*tb);

  if (set_sys_last_vars) {
    Module sys(&scope, runtime->findModuleById(ID(sys)));
    moduleAtPutById(thread, sys, ID(last_type), type);
    moduleAtPutById(thread, sys, ID(last_value), value);
    moduleAtPutById(thread, sys, ID(last_traceback), tb);
  }

  Object hook(&scope,
              runtime->lookupNameInModule(thread, ID(sys), ID(excepthook)));
  if (hook.isError()) {
    writeStderr(thread, "sys.excepthook is missing\n");
    if (displayException(thread, value, tb).isError()) {
      thread->clearPendingException();
    }
    return;
  }

  Object result(&scope, Interpreter::call3(thread, hook, type, value, tb));
  if (!result.isError()) return;
  Object type2(&scope, thread->pendingExceptionType());
  if (givenExceptionMatches(thread, type2, system_exit)) {
    handleSystemExit(thread);
  }
  Object value2(&scope, thread->pendingExceptionValue());
  Object tb2(&scope, thread->pendingExceptionTraceback());
  thread->clearPendingException();
  normalizeException(thread, &type2, &value2, &tb2);
  writeStderr(thread, "Error in sys.excepthook:\n");
  if (displayException(thread, value2, tb2).isError()) {
    thread->clearPendingException();
  }
  writeStderr(thread, "\nOriginal exception was:\n");
  if (displayException(thread, value, tb).isError()) {
    thread->clearPendingException();
  }
}

void printPendingException(Thread* thread) {
  printPendingExceptionImpl(thread, false);
}

void printPendingExceptionWithSysLastVars(Thread* thread) {
  printPendingExceptionImpl(thread, true);
}

// If value has all the attributes of a well-formed SyntaxError, return true and
// populate all of the given parameters. In that case, filename will be a str
// and text will be None or a str. Otherwise, return false and the contents of
// all out parameters are unspecified.
static bool parseSyntaxError(Thread* thread, const Object& value,
                             Object* message, Object* filename, word* lineno,
                             word* offset, Object* text) {
  auto fail = [thread] {
    thread->clearPendingException();
    return false;
  };

  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object result(&scope, runtime->attributeAtById(thread, value, ID(msg)));
  if (result.isError()) return fail();
  *message = *result;

  result = runtime->attributeAtById(thread, value, ID(filename));
  if (result.isError()) return fail();
  if (result.isNoneType()) {
    *filename = runtime->newStrFromCStr("<string>");
  } else if (runtime->isInstanceOfStr(*result)) {
    *filename = *result;
  } else {
    return false;
  }

  result = runtime->attributeAtById(thread, value, ID(lineno));
  if (result.isError()) return fail();
  if (runtime->isInstanceOfInt(*result)) {
    Int ival(&scope, intUnderlying(*result));
    if (ival.numDigits() > 1) return false;
    *lineno = ival.asWord();
  } else {
    return false;
  }

  result = runtime->attributeAtById(thread, value, ID(offset));
  if (result.isError()) return fail();
  if (result.isNoneType()) {
    *offset = -1;
  } else if (runtime->isInstanceOfInt(*result)) {
    Int ival(&scope, intUnderlying(*result));
    if (ival.numDigits() > 1) return false;
    *offset = ival.asWord();
  } else {
    return false;
  }

  result = runtime->attributeAtById(thread, value, ID(text));
  if (result.isError()) return fail();
  if (result.isNoneType() || runtime->isInstanceOfStr(*result)) {
    *text = *result;
  } else {
    return false;
  }

  return true;
}

static RawObject fileWriteString(Thread* thread, const Object& file,
                                 const char* c_str) {
  HandleScope scope(thread);
  Str str(&scope, thread->runtime()->newStrFromCStr(c_str));
  return thread->invokeMethod2(file, ID(write), str);
}

static RawObject fileWriteObjectStr(Thread* thread, const Object& file,
                                    const Object& str) {
  return thread->invokeMethod2(file, ID(write), str);
}

// Used to wrap an expression that may return an Error that should be forwarded,
// or a value that should be ignored otherwise.
//
// TODO(bsimmers): Most of the functions that use this should be rewritten in
// Python once we have enough library support to do so, then we can delete the
// macro.
#define MAY_RAISE(expr)                                                        \
  {                                                                            \
    RawObject result = (expr);                                                 \
    if (result.isError()) return result;                                       \
  }

// Print the source code snippet from a SyntaxError, with a ^ indicating the
// position of the error.
static RawObject printErrorText(Thread* thread, const Object& file, word offset,
                                const Object& text_obj) {
  HandleScope scope(thread);
  Str text_str(&scope, *text_obj);
  word length = text_str.length();
  // This is gross, but it greatly simplifies the string scanning done by the
  // rest of the function, and makes maintaining compatibility with CPython
  // easier.
  unique_c_ptr<char[]> text_owner(text_str.toCStr());
  const char* text = text_owner.get();

  // Adjust text and offset to not print any lines before the one that has the
  // cursor.
  if (offset >= 0) {
    if (offset > 0 && offset == length && text[offset - 1] == '\n') {
      offset--;
    }
    for (;;) {
      const char* newline = std::strchr(text, '\n');
      if (newline == nullptr || newline - text >= offset) break;
      word adjust = newline + 1 - text;
      offset -= adjust;
      length -= adjust;
      text += adjust;
    }
    while (*text == ' ' || *text == '\t' || *text == '\f') {
      text++;
      offset--;
    }
  }

  MAY_RAISE(fileWriteString(thread, file, "    "));
  MAY_RAISE(fileWriteString(thread, file, text));
  if (*text == '\0' || text[length - 1] != '\n') {
    MAY_RAISE(fileWriteString(thread, file, "\n"));
  }
  if (offset == -1) return NoneType::object();
  MAY_RAISE(fileWriteString(thread, file, "    "));
  while (--offset > 0) MAY_RAISE(fileWriteString(thread, file, " "));
  MAY_RAISE(fileWriteString(thread, file, "^\n"));
  return NoneType::object();
}

// Print the traceback, type, and message of a single exception.
static RawObject printSingleException(Thread* thread, const Object& file,
                                      const Object& value_in) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object value(&scope, *value_in);
  Type type(&scope, runtime->typeOf(*value));
  Str type_name(&scope, type.name());

  if (!runtime->isInstanceOfBaseException(*value)) {
    MAY_RAISE(fileWriteString(
        thread, file,
        "TypeError: print_exception(): Exception expected for value, "));

    MAY_RAISE(fileWriteObjectStr(thread, file, type_name));
    MAY_RAISE(fileWriteString(thread, file, " found\n"));
    return NoneType::object();
  }

  BaseException exc(&scope, *value);
  Object tb_obj(&scope, exc.traceback());
  if (tb_obj.isTraceback()) {
    Traceback traceback(&scope, *tb_obj);
    MAY_RAISE(tracebackWrite(thread, traceback, file));
  }

  if (runtime->attributeAtById(thread, value, ID(print_file_and_line))
          .isError()) {
    // Ignore the AttributeError or whatever else went wrong during lookup.
    thread->clearPendingException();
  } else {
    Object message(&scope, NoneType::object());
    Object filename(&scope, NoneType::object());
    Object text(&scope, NoneType::object());
    word lineno, offset;
    if (parseSyntaxError(thread, value, &message, &filename, &lineno, &offset,
                         &text)) {
      value = *message;
      Str filename_str(&scope, *filename);
      unique_c_ptr<char[]> filename_c_str(filename_str.toCStr());
      Str line(&scope, runtime->newStrFromFmt("  File \"%s\", line %w\n",
                                              filename_c_str.get(), lineno));
      MAY_RAISE(fileWriteObjectStr(thread, file, line));
      if (!text.isNoneType()) {
        MAY_RAISE(printErrorText(thread, file, offset, text));
      }
    }
  }

  Object module(&scope, runtime->attributeAtById(thread, type, ID(__module__)));
  if (module.isError() || !runtime->isInstanceOfStr(*module)) {
    if (module.isError()) thread->clearPendingException();
    MAY_RAISE(fileWriteString(thread, file, "<unknown>"));
  } else {
    Str module_str(&scope, *module);
    if (!module_str.equals(runtime->symbols()->at(ID(builtins)))) {
      MAY_RAISE(fileWriteObjectStr(thread, file, module_str));
      MAY_RAISE(fileWriteString(thread, file, "."));
    }
  }

  MAY_RAISE(fileWriteObjectStr(thread, file, type_name));
  MAY_RAISE(fileWriteString(thread, file, ": "));
  Object str_obj(&scope, thread->invokeFunction1(ID(builtins), ID(str), value));
  if (str_obj.isError()) {
    thread->clearPendingException();
    MAY_RAISE(fileWriteString(thread, file, "<exception str() failed>"));
  } else {
    Str str(&scope, *str_obj);
    MAY_RAISE(fileWriteObjectStr(thread, file, str));
  }

  MAY_RAISE(fileWriteString(thread, file, "\n"));
  return NoneType::object();
}

// Print the given exception and any cause or context exceptions it chains to.
static RawObject printExceptionChain(Thread* thread, const Object& file,
                                     const Object& value, const Set& seen) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Object hash_obj(&scope, Interpreter::hash(thread, value));
  if (hash_obj.isErrorException()) return *hash_obj;
  word hash = SmallInt::cast(*hash_obj).value();
  setAdd(thread, seen, value, hash);

  if (runtime->isInstanceOfBaseException(*value)) {
    BaseException exc(&scope, *value);
    Object cause(&scope, exc.cause());
    Object context(&scope, exc.context());
    if (!cause.isNoneType()) {
      hash_obj = Interpreter::hash(thread, cause);
      if (hash_obj.isErrorException()) return *hash_obj;
      hash = SmallInt::cast(*hash_obj).value();
      if (!setIncludes(thread, seen, cause, hash)) {
        MAY_RAISE(printExceptionChain(thread, file, cause, seen));
        MAY_RAISE(
            fileWriteString(thread, file,
                            "\nThe above exception was the direct cause of the "
                            "following exception:\n\n"));
      }
    } else if (!context.isNoneType() &&
               exc.suppressContext() != RawBool::trueObj()) {
      hash_obj = Interpreter::hash(thread, context);
      if (hash_obj.isErrorException()) return *hash_obj;
      hash = SmallInt::cast(*hash_obj).value();
      if (!setIncludes(thread, seen, context, hash)) {
        MAY_RAISE(printExceptionChain(thread, file, context, seen));
        MAY_RAISE(
            fileWriteString(thread, file,
                            "\nDuring handling of the above exception, another "
                            "exception occurred:\n\n"));
      }
    }
  }

  MAY_RAISE(printSingleException(thread, file, value));
  return NoneType::object();
}

#undef MAY_RAISE

RawObject displayException(Thread* thread, const Object& value,
                           const Object& traceback) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  if (runtime->isInstanceOfBaseException(*value) && traceback.isTraceback()) {
    BaseException exc(&scope, *value);
    if (exc.traceback().isNoneType()) exc.setTraceback(*traceback);
  }

  ValueCell sys_stderr_cell(&scope, runtime->sysStderr());
  if (sys_stderr_cell.isUnbound()) {
    fputs("lost sys.stderr\n", stderr);
    return NoneType::object();
  }
  Object sys_stderr(&scope, sys_stderr_cell.value());
  if (sys_stderr.isNoneType()) {
    return NoneType::object();
  }
  Set seen(&scope, runtime->newSet());
  return printExceptionChain(thread, sys_stderr, value, seen);
}

void handleSystemExit(Thread* thread) {
  auto do_exit = [thread](int exit_code) {
    thread->clearPendingException();
    Runtime* runtime = thread->runtime();
    delete runtime;
    std::exit(exit_code);
  };

  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Object arg(&scope, thread->pendingExceptionValue());
  if (runtime->isInstanceOfSystemExit(*arg)) {
    // The exception could be raised by either native or managed code. If
    // native, there will be no SystemExit object. If managed, there will
    // be one to unpack.
    SystemExit exc(&scope, *arg);
    arg = exc.code();
  }
  if (arg.isNoneType()) do_exit(EXIT_SUCCESS);
  if (runtime->isInstanceOfInt(*arg)) {
    // We could convert and check for overflow error, but the overflow error
    // should get cleared anyway.
    do_exit(intUnderlying(*arg).asWordSaturated());
  }

  // The calls below can't have an exception pending
  thread->clearPendingException();

  Object result(&scope, thread->invokeMethod1(arg, ID(__str__)));
  if (!runtime->isInstanceOfStr(*result)) {
    // The calls below can't have an exception pending
    thread->clearPendingException();
    // No __repr__ method or __repr__ raised. Either way, we can't handle it.
    result = runtime->newStrFromCStr("");
  }

  Str result_str(&scope, *result);
  ValueCell sys_stderr_cell(&scope, runtime->sysStderr());
  if (sys_stderr_cell.isUnbound() || sys_stderr_cell.value().isNoneType()) {
    unique_c_ptr<char> buf(result_str.toCStr());
    fwrite(buf.get(), 1, result_str.length(), stderr);
    fputc('\n', stderr);
  } else {
    Object file(&scope, sys_stderr_cell.value());
    fileWriteObjectStr(thread, file, result_str);
    thread->clearPendingException();
    fileWriteString(thread, file, "\n");
  }
  do_exit(EXIT_FAILURE);
}

RawObject METH(BaseException, __init__)(Thread* thread, Frame* frame,
                                        word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfBaseException(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(BaseException));
  }
  BaseException self(&scope, *self_obj);
  Object args_obj(&scope, args.get(1));
  self.setArgs(*args_obj);
  self.setCause(Unbound::object());
  self.setContext(Unbound::object());
  self.setTraceback(Unbound::object());
  self.setSuppressContext(RawBool::falseObj());
  return NoneType::object();
}

RawObject METH(StopIteration, __init__)(Thread* thread, Frame* frame,
                                        word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfStopIteration(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(StopIteration));
  }
  StopIteration self(&scope, *self_obj);
  Object args_obj(&scope, args.get(1));
  self.setArgs(*args_obj);
  self.setCause(Unbound::object());
  self.setContext(Unbound::object());
  self.setTraceback(Unbound::object());
  self.setSuppressContext(RawBool::falseObj());
  Tuple tuple(&scope, self.args());
  if (tuple.length() > 0) self.setValue(tuple.at(0));
  return NoneType::object();
}

RawObject METH(SystemExit, __init__)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfSystemExit(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(SystemExit));
  }
  SystemExit self(&scope, *self_obj);
  RawObject result = METH(BaseException, __init__)(thread, frame, nargs);
  if (result.isError()) {
    return result;
  }
  Tuple tuple(&scope, self.args());
  if (tuple.length() > 0) {
    self.setCode(tuple.at(0));
  }
  return NoneType::object();
}

void initializeExceptionTypes(Thread* thread) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Layout layout(&scope, runtime->layoutAt(LayoutId::kNoneType));
  Type type(&scope, layout.describedType());
  for (ExceptionTypeSpec spec : kExceptionSpecs) {
    Layout super_layout(&scope, runtime->layoutAt(spec.superclass_id));
    word size = (Tuple::cast(super_layout.inObjectAttributes()).length() +
                 spec.attributes.length()) *
                kPointerSize;
    type = addBuiltinType(thread, spec.name, spec.layout_id, spec.superclass_id,
                          spec.attributes, size, /*basetype=*/true);
    layout = type.instanceLayout();
    runtime->layoutSetTupleOverflow(*layout);
  }
}

}  // namespace py
