#include "bytes-builtins.h"

#include "frame.h"
#include "runtime.h"
#include "trampolines-inl.h"

namespace python {

const BuiltinMethod BytesBuiltins::kMethods[] = {
    {SymbolId::kDunderAdd, nativeTrampoline<dunderAdd>},
    {SymbolId::kDunderLen, nativeTrampoline<dunderLen>},
};

void BytesBuiltins::initialize(Runtime* runtime) {
  HandleScope scope;
  Type type(&scope, runtime->addBuiltinTypeWithMethods(
                        SymbolId::kBytes, LayoutId::kBytes, LayoutId::kObject,
                        kMethods));
}

RawObject BytesBuiltins::dunderAdd(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  if (nargs != 2) {
    return thread->raiseTypeError(
        runtime->newStrFromFormat("expected 1 argument, got %ld", nargs - 1));
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfBytes(*self_obj)) {
    return thread->raiseTypeErrorWithCStr(
        "'__add__' requires a 'bytes' object");
  }
  Object other_obj(&scope, args.get(1));
  if (!runtime->isInstanceOfBytes(*other_obj)) {
    return thread->raiseTypeErrorWithCStr(
        "can only concatenate bytes to bytes");
  }
  Bytes self(&scope, *self_obj);
  Bytes other(&scope, *other_obj);
  return runtime->bytesConcat(thread, self, other);
}

RawObject BytesBuiltins::dunderLen(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  if (nargs != 1) {
    return thread->raiseTypeError(
        runtime->newStrFromFormat("expected 0 arguments, got %ld", nargs - 1));
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfBytes(*self_obj)) {
    return thread->raiseTypeErrorWithCStr(
        "'__len__' requires a 'bytes' object");
  }

  Bytes self(&scope, *self_obj);
  return SmallInt::fromWord(self->length());
}

}  // namespace python
