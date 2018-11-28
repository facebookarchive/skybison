#include "exception-builtins.h"

#include "frame.h"
#include "objects.h"
#include "runtime.h"
#include "trampolines-inl.h"

namespace python {

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
