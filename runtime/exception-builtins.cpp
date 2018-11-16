#include "exception-builtins.h"

#include "frame.h"
#include "objects.h"
#include "runtime.h"
#include "trampolines-inl.h"

namespace python {

const BuiltinAttribute BaseExceptionBuiltins::kAttributes[] = {
    {SymbolId::kArgs, BaseException::kArgsOffset},
    {SymbolId::kTraceback, BaseException::kTracebackOffset},
    {SymbolId::kDunderContext, BaseException::kContextOffset},
    {SymbolId::kCause, BaseException::kCauseOffset},
};

const BuiltinMethod BaseExceptionBuiltins::kMethods[] = {
    {SymbolId::kDunderInit, nativeTrampoline<dunderInit>},
};

void BaseExceptionBuiltins::initialize(Runtime* runtime) {
  HandleScope scope;
  Handle<Type> type(&scope,
                    runtime->addBuiltinClass(SymbolId::kBaseException,
                                             LayoutId::kBaseException,
                                             LayoutId::kObject, kAttributes));
  type->setFlag(Type::Flag::kBaseExceptionSubclass);
  for (uword i = 0; i < ARRAYSIZE(kMethods); i++) {
    runtime->classAddBuiltinFunction(type, kMethods[i].name,
                                     kMethods[i].address);
  }
}

Object* BaseExceptionBuiltins::dunderInit(Thread* thread, Frame* frame,
                                          word nargs) {
  HandleScope scope(thread);
  if (nargs == 0) {
    return thread->throwTypeErrorFromCStr(
        "'__init__' of 'BaseException' needs an argument");
  }
  Arguments args(frame, nargs);
  if (!thread->runtime()->hasSubClassFlag(args.get(0),
                                          Type::Flag::kBaseExceptionSubclass)) {
    return thread->throwTypeErrorFromCStr(
        "'__init__' requires a 'BaseException' object");
  }
  UncheckedHandle<BaseException> self(&scope, args.get(0));
  Handle<ObjectArray> tuple(&scope,
                            thread->runtime()->newObjectArray(nargs - 1));
  for (word i = 1; i < nargs; i++) {
    tuple->atPut(i - 1, args.get(i));
  }
  self->setArgs(*tuple);
  return None::object();
}

const BuiltinAttribute StopIterationBuiltins::kAttributes[] = {
    {SymbolId::kValue, StopIteration::kValueOffset},
};

const BuiltinMethod StopIterationBuiltins::kMethods[] = {
    {SymbolId::kDunderInit, nativeTrampoline<dunderInit>},
};

void StopIterationBuiltins::initialize(Runtime* runtime) {
  HandleScope scope;
  Handle<Type> type(
      &scope, runtime->addBuiltinClass(SymbolId::kStopIteration,
                                       LayoutId::kStopIteration,
                                       LayoutId::kException, kAttributes));
  type->setFlag(Type::Flag::kStopIterationSubclass);
  for (uword i = 0; i < ARRAYSIZE(kMethods); i++) {
    runtime->classAddBuiltinFunction(type, kMethods[i].name,
                                     kMethods[i].address);
  }
}

Object* StopIterationBuiltins::dunderInit(Thread* thread, Frame* frame,
                                          word nargs) {
  HandleScope scope(thread);
  if (nargs == 0) {
    return thread->throwTypeErrorFromCStr(
        "'__init__' of 'StopIteration' needs an argument");
  }
  Arguments args(frame, nargs);
  if (!thread->runtime()->hasSubClassFlag(args.get(0),
                                          Type::Flag::kStopIterationSubclass)) {
    return thread->throwTypeErrorFromCStr(
        "'__init__' requires a 'StopIteration' object");
  }
  UncheckedHandle<StopIteration> self(&scope, args.get(0));
  Object* result = BaseExceptionBuiltins::dunderInit(thread, frame, nargs);
  if (result->isError()) {
    return result;
  }
  Handle<ObjectArray> tuple(&scope, self->args());
  if (tuple->length() > 0) {
    self->setValue(tuple->at(0));
  }
  return None::object();
}

const BuiltinAttribute SystemExitBuiltins::kAttributes[] = {
    {SymbolId::kValue, SystemExit::kCodeOffset},
};

const BuiltinMethod SystemExitBuiltins::kMethods[] = {
    {SymbolId::kDunderInit, nativeTrampoline<dunderInit>},
};

void SystemExitBuiltins::initialize(Runtime* runtime) {
  HandleScope scope;
  Handle<Type> type(&scope, runtime->addBuiltinClass(
                                SymbolId::kSystemExit, LayoutId::kSystemExit,
                                LayoutId::kBaseException, kAttributes));
  type->setFlag(Type::Flag::kSystemExitSubclass);
  for (uword i = 0; i < ARRAYSIZE(kMethods); i++) {
    runtime->classAddBuiltinFunction(type, kMethods[i].name,
                                     kMethods[i].address);
  }
}

Object* SystemExitBuiltins::dunderInit(Thread* thread, Frame* frame,
                                       word nargs) {
  HandleScope scope(thread);
  if (nargs == 0) {
    return thread->throwTypeErrorFromCStr(
        "'__init__' of 'SystemExit' needs an argument");
  }
  Arguments args(frame, nargs);
  if (!thread->runtime()->hasSubClassFlag(args.get(0),
                                          Type::Flag::kSystemExitSubclass)) {
    return thread->throwTypeErrorFromCStr(
        "'__init__' requires a 'SystemExit' object");
  }
  UncheckedHandle<SystemExit> self(&scope, args.get(0));
  Object* result = BaseExceptionBuiltins::dunderInit(thread, frame, nargs);
  if (result->isError()) {
    return result;
  }
  Handle<ObjectArray> tuple(&scope, self->args());
  if (tuple->length() > 0) {
    self->setCode(tuple->at(0));
  }
  return None::object();
}

const BuiltinAttribute ImportErrorBuiltins::kAttributes[] = {
    {SymbolId::kMsg, ImportError::kMsgOffset},
    {SymbolId::kName, ImportError::kNameOffset},
    {SymbolId::kPath, ImportError::kPathOffset},
};

void ImportErrorBuiltins::initialize(Runtime* runtime) {
  HandleScope scope;
  Handle<Type> type(&scope, runtime->addBuiltinClass(
                                SymbolId::kImportError, LayoutId::kImportError,
                                LayoutId::kException, kAttributes));
}

}  // namespace python
