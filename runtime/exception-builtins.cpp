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
  type->setFlag(Type::Flag::kBaseExceptionSubclass);
}

RawObject BaseExceptionBuiltins::dunderInit(Thread* thread, Frame* frame,
                                            word nargs) {
  HandleScope scope(thread);
  if (nargs == 0) {
    return thread->raiseTypeErrorWithCStr(
        "'__init__' of 'RawBaseException' needs an argument");
  }
  Arguments args(frame, nargs);
  if (!thread->runtime()->hasSubClassFlag(args.get(0),
                                          Type::Flag::kBaseExceptionSubclass)) {
    return thread->raiseTypeErrorWithCStr(
        "'__init__' requires a 'RawBaseException' object");
  }
  UncheckedHandle<RawBaseException> self(&scope, args.get(0));
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
  type->setFlag(Type::Flag::kStopIterationSubclass);
}

RawObject StopIterationBuiltins::dunderInit(Thread* thread, Frame* frame,
                                            word nargs) {
  HandleScope scope(thread);
  if (nargs == 0) {
    return thread->raiseTypeErrorWithCStr(
        "'__init__' of 'RawStopIteration' needs an argument");
  }
  Arguments args(frame, nargs);
  if (!thread->runtime()->hasSubClassFlag(args.get(0),
                                          Type::Flag::kStopIterationSubclass)) {
    return thread->raiseTypeErrorWithCStr(
        "'__init__' requires a 'RawStopIteration' object");
  }
  UncheckedHandle<RawStopIteration> self(&scope, args.get(0));
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
  type->setFlag(Type::Flag::kSystemExitSubclass);
}

RawObject SystemExitBuiltins::dunderInit(Thread* thread, Frame* frame,
                                         word nargs) {
  HandleScope scope(thread);
  if (nargs == 0) {
    return thread->raiseTypeErrorWithCStr(
        "'__init__' of 'RawSystemExit' needs an argument");
  }
  Arguments args(frame, nargs);
  if (!thread->runtime()->hasSubClassFlag(args.get(0),
                                          Type::Flag::kSystemExitSubclass)) {
    return thread->raiseTypeErrorWithCStr(
        "'__init__' requires a 'RawSystemExit' object");
  }
  UncheckedHandle<RawSystemExit> self(&scope, args.get(0));
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
