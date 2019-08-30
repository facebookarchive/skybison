#include "exception-builtins.h"

#include <unistd.h>
#include <cinttypes>

#include "builtins-module.h"
#include "debugging.h"
#include "frame.h"
#include "int-builtins.h"
#include "module-builtins.h"
#include "objects.h"
#include "runtime.h"
#include "sys-module.h"
#include "tuple-builtins.h"

namespace python {

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
      return runtime->isSubclass(subtype, supertype);
    }
  }
  return *given_type == *exc;
}

RawObject createException(Thread* thread, const Type& type,
                          const Object& value) {
  Frame* caller = thread->currentFrame();

  if (value.isNoneType()) {
    return Interpreter::callFunction0(thread, caller, type);
  }
  if (thread->runtime()->isInstanceOfTuple(*value)) {
    HandleScope scope(thread);
    Tuple args(&scope, tupleUnderlying(thread, value));
    return Interpreter::callFunction(thread, caller, type, args);
  }
  return Interpreter::callFunction1(thread, caller, type, value);
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
    if (!runtime->isSubclass(value_type, type)) {
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
    Module sys(&scope, runtime->findModuleById(SymbolId::kSys));
    Str key(&scope, runtime->symbols()->LastType());
    moduleAtPut(thread, sys, key, type);
    key = runtime->symbols()->LastValue();
    moduleAtPut(thread, sys, key, value);
    key = runtime->symbols()->LastTraceback();
    moduleAtPut(thread, sys, key, tb);
  }

  Object hook(&scope, runtime->lookupNameInModule(thread, SymbolId::kSys,
                                                  SymbolId::kExcepthook));
  if (hook.isError()) {
    writeStderr(thread, "sys.excepthook is missing\n");
    if (displayException(thread, value, tb).isError()) {
      thread->clearPendingException();
    }
    return;
  }

  Object result(&scope,
                Interpreter::callFunction3(thread, thread->currentFrame(), hook,
                                           type, value, tb));
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
  Object result(&scope,
                runtime->attributeAtById(thread, value, SymbolId::kMsg));
  if (result.isError()) return fail();
  *message = *result;

  result = runtime->attributeAtById(thread, value, SymbolId::kFilename);
  if (result.isError()) return fail();
  if (result.isNoneType()) {
    *filename = runtime->newStrFromCStr("<string>");
  } else if (runtime->isInstanceOfStr(*result)) {
    *filename = *result;
  } else {
    return false;
  }

  result = runtime->attributeAtById(thread, value, SymbolId::kLineno);
  if (result.isError()) return fail();
  if (runtime->isInstanceOfInt(*result)) {
    Int ival(&scope, intUnderlying(thread, result));
    if (ival.numDigits() > 1) return false;
    *lineno = ival.asWord();
  } else {
    return false;
  }

  result = runtime->attributeAtById(thread, value, SymbolId::kOffset);
  if (result.isError()) return fail();
  if (result.isNoneType()) {
    *offset = -1;
  } else if (runtime->isInstanceOfInt(*result)) {
    Int ival(&scope, intUnderlying(thread, result));
    if (ival.numDigits() > 1) return false;
    *offset = ival.asWord();
  } else {
    return false;
  }

  result = runtime->attributeAtById(thread, value, SymbolId::kText);
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
  return thread->invokeMethod2(file, SymbolId::kWrite, str);
}

static RawObject fileWriteObjectStr(Thread* thread, const Object& file,
                                    const Object& str) {
  return thread->invokeMethod2(file, SymbolId::kWrite, str);
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
  word length = text_str.charLength();
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
  Object tb(&scope, exc.traceback());
  if (tb.isStr()) {
    // TODO(T39919701): Delete this once we can print real tracebacks.
    MAY_RAISE(fileWriteObjectStr(thread, file, tb));
  } else if (!tb.isNoneType()) {
    // TODO(T40171960): Print the traceback
    MAY_RAISE(fileWriteString(thread, file, "<traceback>\n"));
  }

  if (runtime->attributeAtById(thread, value, SymbolId::kPrintFileAndLine)
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

  Object module(
      &scope, runtime->attributeAtById(thread, type, SymbolId::kDunderModule));
  if (module.isError() || !runtime->isInstanceOfStr(*module)) {
    if (module.isError()) thread->clearPendingException();
    MAY_RAISE(fileWriteString(thread, file, "<unknown>"));
  } else {
    Str module_str(&scope, *module);
    if (!module_str.equals(runtime->symbols()->Builtins())) {
      MAY_RAISE(fileWriteObjectStr(thread, file, module_str));
      MAY_RAISE(fileWriteString(thread, file, "."));
    }
  }

  MAY_RAISE(fileWriteObjectStr(thread, file, type_name));
  MAY_RAISE(fileWriteString(thread, file, ": "));
  Object str_obj(&scope, thread->invokeFunction1(SymbolId::kBuiltins,
                                                 SymbolId::kStr, value));
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
  runtime->setAdd(thread, seen, value);

  if (runtime->isInstanceOfBaseException(*value)) {
    HandleScope scope(thread);
    BaseException exc(&scope, *value);
    Object cause(&scope, exc.cause());
    Object context(&scope, exc.context());
    if (!cause.isNoneType()) {
      Object cause_hash(&scope, Interpreter::hash(thread, cause));
      if (cause_hash.isErrorException()) return *cause_hash;
      if (!runtime->setIncludes(thread, seen, cause, cause_hash)) {
        MAY_RAISE(printExceptionChain(thread, file, cause, seen));
        MAY_RAISE(
            fileWriteString(thread, file,
                            "\nThe above exception was the direct cause of the "
                            "following exception:\n\n"));
      }
    } else if (!context.isNoneType() &&
               exc.suppressContext() != RawBool::trueObj()) {
      Object context_hash(&scope, Interpreter::hash(thread, context));
      if (context_hash.isErrorException()) return *context_hash;
      if (!runtime->setIncludes(thread, seen, context, context_hash)) {
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
    dump(value);
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
    runtime->atExit();
    runtime->freeApiHandles();
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
    Int arg_int(&scope, intUnderlying(thread, arg));
    // We could convert and check for overflow error, but the overflow error
    // should get cleared anyway.
    do_exit(arg_int.asWordSaturated());
  }

  // The calls below can't have an exception pending
  thread->clearPendingException();

  Object result(&scope, thread->invokeMethod1(arg, SymbolId::kDunderRepr));
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
    fwrite(buf.get(), 1, result_str.charLength(), stderr);
    fputc('\n', stderr);
  } else {
    Object file(&scope, sys_stderr_cell.value());
    fileWriteObjectStr(thread, file, result_str);
    thread->clearPendingException();
    fileWriteString(thread, file, "\n");
  }
  do_exit(EXIT_FAILURE);
}

const BuiltinAttribute BaseExceptionBuiltins::kAttributes[] = {
    {SymbolId::kArgs, RawBaseException::kArgsOffset},
    {SymbolId::kDunderTraceback, RawBaseException::kTracebackOffset},
    {SymbolId::kDunderContext, RawBaseException::kContextOffset},
    {SymbolId::kDunderCause, RawBaseException::kCauseOffset},
    {SymbolId::kDunderSuppressContext,
     RawBaseException::kSuppressContextOffset},
    {SymbolId::kSentinelId, -1},
};

const BuiltinMethod BaseExceptionBuiltins::kBuiltinMethods[] = {
    {SymbolId::kDunderInit, dunderInit},
    {SymbolId::kSentinelId, nullptr},
};

RawObject BaseExceptionBuiltins::dunderInit(Thread* thread, Frame* frame,
                                            word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  if (!thread->runtime()->isInstanceOfBaseException(args.get(0))) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "'__init__' requires a 'BaseException' object");
  }
  BaseException self(&scope, args.get(0));
  self.setArgs(args.get(1));
  self.setCause(Unbound::object());
  self.setContext(Unbound::object());
  self.setTraceback(Unbound::object());
  self.setSuppressContext(RawBool::falseObj());
  return NoneType::object();
}

const BuiltinAttribute StopIterationBuiltins::kAttributes[] = {
    {SymbolId::kValue, RawStopIteration::kValueOffset},
    {SymbolId::kSentinelId, -1},
};

const BuiltinMethod StopIterationBuiltins::kBuiltinMethods[] = {
    {SymbolId::kDunderInit, dunderInit},
    {SymbolId::kSentinelId, nullptr},
};

RawObject StopIterationBuiltins::dunderInit(Thread* thread, Frame* frame,
                                            word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  if (!thread->runtime()->isInstanceOfStopIteration(args.get(0))) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "'__init__' requires a 'StopIteration' object");
  }
  StopIteration self(&scope, args.get(0));
  RawObject result = BaseExceptionBuiltins::dunderInit(thread, frame, nargs);
  if (result.isError()) {
    return result;
  }
  Tuple tuple(&scope, self.args());
  if (tuple.length() > 0) {
    self.setValue(tuple.at(0));
  }
  return NoneType::object();
}

const BuiltinAttribute SystemExitBuiltins::kAttributes[] = {
    {SymbolId::kValue, RawSystemExit::kCodeOffset},
    {SymbolId::kSentinelId, -1},
};

const BuiltinMethod SystemExitBuiltins::kBuiltinMethods[] = {
    {SymbolId::kDunderInit, dunderInit},
    {SymbolId::kSentinelId, nullptr},
};

const BuiltinAttribute SyntaxErrorBuiltins::kAttributes[] = {
    {SymbolId::kFilename, RawSyntaxError::kFilenameOffset},
    {SymbolId::kLineno, RawSyntaxError::kLinenoOffset},
    {SymbolId::kMsg, RawSyntaxError::kMsgOffset},
    {SymbolId::kOffset, RawSyntaxError::kOffsetOffset},
    {SymbolId::kPrintFileAndLine, RawSyntaxError::kPrintFileAndLineOffset},
    {SymbolId::kText, RawSyntaxError::kTextOffset},
    {SymbolId::kSentinelId, -1},
};

RawObject SystemExitBuiltins::dunderInit(Thread* thread, Frame* frame,
                                         word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  if (!thread->runtime()->isInstanceOfSystemExit(args.get(0))) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "'__init__' requires a 'SystemExit' object");
  }
  SystemExit self(&scope, args.get(0));
  RawObject result = BaseExceptionBuiltins::dunderInit(thread, frame, nargs);
  if (result.isError()) {
    return result;
  }
  Tuple tuple(&scope, self.args());
  if (tuple.length() > 0) {
    self.setCode(tuple.at(0));
  }
  return NoneType::object();
}

const BuiltinAttribute ImportErrorBuiltins::kAttributes[] = {
    {SymbolId::kMsg, RawImportError::kMsgOffset},
    {SymbolId::kName, RawImportError::kNameOffset},
    {SymbolId::kPath, RawImportError::kPathOffset},
    {SymbolId::kSentinelId, -1},
};

static const BuiltinAttribute kUnicodeErrorBaseAttributes[] = {
    {SymbolId::kEncoding, RawUnicodeErrorBase::kEncodingOffset},
    {SymbolId::kObjectTypename, RawUnicodeErrorBase::kObjectOffset},
    {SymbolId::kStart, RawUnicodeErrorBase::kStartOffset},
    {SymbolId::kEnd, RawUnicodeErrorBase::kEndOffset},
    {SymbolId::kReason, RawUnicodeErrorBase::kReasonOffset},
    {SymbolId::kSentinelId, -1},
};

const BuiltinAttribute* const UnicodeDecodeErrorBuiltins::kAttributes =
    kUnicodeErrorBaseAttributes;

const BuiltinAttribute* const UnicodeEncodeErrorBuiltins::kAttributes =
    kUnicodeErrorBaseAttributes;

const BuiltinAttribute* const UnicodeTranslateErrorBuiltins::kAttributes =
    kUnicodeErrorBaseAttributes;

}  // namespace python
