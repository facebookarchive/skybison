#include "exception-builtins.h"

#include "frame.h"
#include "objects.h"
#include "runtime.h"
#include "trampolines-inl.h"

namespace python {

bool givenExceptionMatches(Thread* thread, const Object& given,
                           const Object& exc) {
  HandleScope scope(thread);
  if (exc->isTuple()) {
    Tuple tuple(&scope, *exc);
    Object item(&scope, NoneType::object());
    for (word i = 0; i < tuple->length(); i++) {
      item = tuple->at(i);
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
    if (subtype->isBaseExceptionSubclass() &&
        supertype->isBaseExceptionSubclass()) {
      return runtime->isSubclass(subtype, supertype);
    }
  }
  return *given_type == *exc;
}

RawObject createException(Thread* thread, const Type& type,
                          const Object& value) {
  Frame* caller = thread->currentFrame();

  if (value->isNoneType()) {
    return Interpreter::callFunction0(thread, caller, type);
  }
  if (thread->runtime()->isInstanceOfTuple(*value)) {
    HandleScope scope(thread);
    Tuple args(&scope, *value);
    return Interpreter::callFunction(thread, caller, type, args);
  }
  return Interpreter::callFunction1(thread, caller, type, value);
}

void normalizeException(Thread* thread, Object* exc, Object* val, Object* tb) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  auto normalize = [&] {
    if (!runtime->isInstanceOfType(**exc)) return true;
    Type type(&scope, **exc);
    if (!type->isBaseExceptionSubclass()) return true;
    Object value(&scope, **val);
    Type value_type(&scope, runtime->typeOf(*value));

    // TODO(bsimmers): Extend this to support all the weird cases allowed by
    // PyObject_IsSubclass.
    if (!runtime->isSubclass(value_type, type)) {
      // value isn't an instance of type. Replace it with type(value).
      value = createException(thread, type, value);
      if (value->isError()) return false;
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
      thread->raiseWithCStr(
          LayoutId::kRecursionError,
          "maximum recursion depth exceeded while normalizing an exception");
    }

    *exc = thread->pendingExceptionType();
    *val = thread->pendingExceptionValue();
    Object new_tb(&scope, thread->pendingExceptionTraceback());
    if (!new_tb->isNoneType()) *tb = *new_tb;
    thread->clearPendingException();
  }

  if (runtime->isInstanceOfType(**exc)) {
    Type type(&scope, **exc);
    if (type->builtinBase() == LayoutId::kMemoryError) {
      UNIMPLEMENTED(
          "Cannot recover from MemoryErrors while normalizing exceptions.");
    }
    UNIMPLEMENTED(
        "Cannot recover from the recursive normalization of an exception.");
  }
}

const BuiltinAttribute BaseExceptionBuiltins::kAttributes[] = {
    {SymbolId::kArgs, RawBaseException::kArgsOffset},
    {SymbolId::kTraceback, RawBaseException::kTracebackOffset},
    {SymbolId::kDunderContext, RawBaseException::kContextOffset},
    {SymbolId::kCause, RawBaseException::kCauseOffset},
};

const BuiltinMethod BaseExceptionBuiltins::kMethods[] = {
    {SymbolId::kDunderInit, nativeTrampoline<dunderInit>},
};

void BaseExceptionBuiltins::initialize(Runtime* runtime) {
  HandleScope scope;
  Type type(&scope, runtime->addBuiltinType(
                        SymbolId::kBaseException, LayoutId::kBaseException,
                        LayoutId::kObject, kAttributes, kMethods));
}

RawObject BaseExceptionBuiltins::dunderInit(Thread* thread, Frame* frame,
                                            word nargs) {
  HandleScope scope(thread);
  if (nargs == 0) {
    return thread->raiseTypeErrorWithCStr(
        "'__init__' of 'BaseException' needs an argument");
  }
  Arguments args(frame, nargs);
  if (!thread->runtime()->isInstanceOfBaseException(args.get(0))) {
    return thread->raiseTypeErrorWithCStr(
        "'__init__' requires a 'BaseException' object");
  }
  BaseException self(&scope, args.get(0));
  Tuple tuple(&scope, thread->runtime()->newTuple(nargs - 1));
  for (word i = 1; i < nargs; i++) {
    tuple->atPut(i - 1, args.get(i));
  }
  self->setArgs(*tuple);
  return NoneType::object();
}

const BuiltinAttribute StopIterationBuiltins::kAttributes[] = {
    {SymbolId::kValue, RawStopIteration::kValueOffset},
};

const BuiltinMethod StopIterationBuiltins::kMethods[] = {
    {SymbolId::kDunderInit, nativeTrampoline<dunderInit>},
};

void StopIterationBuiltins::initialize(Runtime* runtime) {
  HandleScope scope;
  Type type(&scope, runtime->addBuiltinType(
                        SymbolId::kStopIteration, LayoutId::kStopIteration,
                        LayoutId::kException, kAttributes, kMethods));
}

RawObject StopIterationBuiltins::dunderInit(Thread* thread, Frame* frame,
                                            word nargs) {
  HandleScope scope(thread);
  if (nargs == 0) {
    return thread->raiseTypeErrorWithCStr(
        "'__init__' of 'StopIteration' needs an argument");
  }
  Arguments args(frame, nargs);
  if (!thread->runtime()->isInstanceOfStopIteration(args.get(0))) {
    return thread->raiseTypeErrorWithCStr(
        "'__init__' requires a 'StopIteration' object");
  }
  StopIteration self(&scope, args.get(0));
  RawObject result = BaseExceptionBuiltins::dunderInit(thread, frame, nargs);
  if (result->isError()) {
    return result;
  }
  Tuple tuple(&scope, self->args());
  if (tuple->length() > 0) {
    self->setValue(tuple->at(0));
  }
  return NoneType::object();
}

const BuiltinAttribute SystemExitBuiltins::kAttributes[] = {
    {SymbolId::kValue, RawSystemExit::kCodeOffset},
};

const BuiltinMethod SystemExitBuiltins::kMethods[] = {
    {SymbolId::kDunderInit, nativeTrampoline<dunderInit>},
};

void SystemExitBuiltins::initialize(Runtime* runtime) {
  HandleScope scope;
  Type type(&scope, runtime->addBuiltinType(
                        SymbolId::kSystemExit, LayoutId::kSystemExit,
                        LayoutId::kBaseException, kAttributes, kMethods));
}

RawObject SystemExitBuiltins::dunderInit(Thread* thread, Frame* frame,
                                         word nargs) {
  HandleScope scope(thread);
  if (nargs == 0) {
    return thread->raiseTypeErrorWithCStr(
        "'__init__' of 'SystemExit' needs an argument");
  }
  Arguments args(frame, nargs);
  if (!thread->runtime()->isInstanceOfSystemExit(args.get(0))) {
    return thread->raiseTypeErrorWithCStr(
        "'__init__' requires a 'SystemExit' object");
  }
  SystemExit self(&scope, args.get(0));
  RawObject result = BaseExceptionBuiltins::dunderInit(thread, frame, nargs);
  if (result->isError()) {
    return result;
  }
  Tuple tuple(&scope, self->args());
  if (tuple->length() > 0) {
    self->setCode(tuple->at(0));
  }
  return NoneType::object();
}

const BuiltinAttribute ImportErrorBuiltins::kAttributes[] = {
    {SymbolId::kMsg, RawImportError::kMsgOffset},
    {SymbolId::kName, RawImportError::kNameOffset},
    {SymbolId::kPath, RawImportError::kPathOffset},
};

void ImportErrorBuiltins::initialize(Runtime* runtime) {
  HandleScope scope;
  Type type(&scope, runtime->addBuiltinTypeWithAttrs(
                        SymbolId::kImportError, LayoutId::kImportError,
                        LayoutId::kException, kAttributes));
}

}  // namespace python
