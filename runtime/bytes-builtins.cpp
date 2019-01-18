#include "bytes-builtins.h"

#include "frame.h"
#include "runtime.h"
#include "trampolines-inl.h"

namespace python {

const BuiltinMethod BytesBuiltins::kMethods[] = {
    {SymbolId::kDunderAdd, nativeTrampoline<dunderAdd>},
    {SymbolId::kDunderEq, nativeTrampoline<dunderEq>},
    {SymbolId::kDunderGe, nativeTrampoline<dunderGe>},
    {SymbolId::kDunderGetItem, nativeTrampoline<dunderGetItem>},
    {SymbolId::kDunderGt, nativeTrampoline<dunderGt>},
    {SymbolId::kDunderLe, nativeTrampoline<dunderLe>},
    {SymbolId::kDunderLen, nativeTrampoline<dunderLen>},
    {SymbolId::kDunderLt, nativeTrampoline<dunderLt>},
    {SymbolId::kDunderNe, nativeTrampoline<dunderNe>},
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

RawObject BytesBuiltins::dunderEq(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  if (nargs != 2) {
    return thread->raiseTypeError(
        runtime->newStrFromFormat("expected 1 argument, got %ld", nargs - 1));
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfBytes(*self_obj)) {
    return thread->raiseTypeErrorWithCStr("'__eq__' requires a 'bytes' object");
  }
  Object other_obj(&scope, args.get(1));
  if (!runtime->isInstanceOfBytes(*other_obj)) {
    return runtime->notImplemented();
  }
  Bytes self(&scope, *self_obj);
  Bytes other(&scope, *other_obj);
  return Bool::fromBool(self->compare(*other) == 0);
}

RawObject BytesBuiltins::dunderGe(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  if (nargs != 2) {
    return thread->raiseTypeError(
        runtime->newStrFromFormat("expected 1 argument, got %ld", nargs - 1));
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfBytes(*self_obj)) {
    return thread->raiseTypeErrorWithCStr("'__ge__' requires a 'bytes' object");
  }
  Object other_obj(&scope, args.get(1));
  if (!runtime->isInstanceOfBytes(*other_obj)) {
    return runtime->notImplemented();
  }
  Bytes self(&scope, *self_obj);
  Bytes other(&scope, *other_obj);
  return Bool::fromBool(self->compare(*other) >= 0);
}

RawObject BytesBuiltins::dunderGetItem(Thread* thread, Frame* frame,
                                       word nargs) {
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
        "'__getitem__' requires a 'bytes' object");
  }
  Bytes self(&scope, *self_obj);
  Object index(&scope, args.get(1));
  // TODO(T27897506): use __index__ to get index
  if (runtime->isInstanceOfInt(*index)) {
    // TODO(T38780562): strict subclass of int
    if (!index->isSmallInt()) {
      return thread->raiseIndexErrorWithCStr(
          "cannot fit index into an index-sized integer");
    }
    word idx = RawSmallInt::cast(*index)->value();
    word len = self->length();
    if (idx < 0) idx += len;
    if (idx < 0 || idx >= len) {
      return thread->raiseIndexErrorWithCStr("index out of range");
    }
    return RawSmallInt::fromWord(static_cast<word>(self->byteAt(idx)));
  }
  if (index->isSlice()) {
    Slice slice(&scope, *index);
    word start, stop, step;
    slice->unpack(&start, &stop, &step);
    word len = RawSlice::adjustIndices(self->length(), &start, &stop, step);
    if (start == 0 && step == 1) {
      return *slice;
    }
    // TODO(T36997048): intern 1-element byte arrays
    Bytes result(&scope, runtime->newBytes(len, 0));
    for (word i = 0, idx = start; i < len; i++, idx += step) {
      result->byteAtPut(i, self->byteAt(idx));
    }
    return *result;
  }
  return thread->raiseTypeErrorWithCStr(
      "indices must either be slices or provide '__index__'");
}

RawObject BytesBuiltins::dunderGt(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  if (nargs != 2) {
    return thread->raiseTypeError(
        runtime->newStrFromFormat("expected 1 argument, got %ld", nargs - 1));
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfBytes(*self_obj)) {
    return thread->raiseTypeErrorWithCStr("'__gt__' requires a 'bytes' object");
  }
  Object other_obj(&scope, args.get(1));
  if (!runtime->isInstanceOfBytes(*other_obj)) {
    return runtime->notImplemented();
  }
  Bytes self(&scope, *self_obj);
  Bytes other(&scope, *other_obj);
  return Bool::fromBool(self->compare(*other) > 0);
}

RawObject BytesBuiltins::dunderLe(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  if (nargs != 2) {
    return thread->raiseTypeError(
        runtime->newStrFromFormat("expected 1 argument, got %ld", nargs - 1));
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfBytes(*self_obj)) {
    return thread->raiseTypeErrorWithCStr("'__le__' requires a 'bytes' object");
  }
  Object other_obj(&scope, args.get(1));
  if (!runtime->isInstanceOfBytes(*other_obj)) {
    return runtime->notImplemented();
  }
  Bytes self(&scope, *self_obj);
  Bytes other(&scope, *other_obj);
  return Bool::fromBool(self->compare(*other) <= 0);
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

RawObject BytesBuiltins::dunderLt(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  if (nargs != 2) {
    return thread->raiseTypeError(
        runtime->newStrFromFormat("expected 1 argument, got %ld", nargs - 1));
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfBytes(*self_obj)) {
    return thread->raiseTypeErrorWithCStr("'__lt__' requires a 'bytes' object");
  }
  Object other_obj(&scope, args.get(1));
  if (!runtime->isInstanceOfBytes(*other_obj)) {
    return runtime->notImplemented();
  }
  Bytes self(&scope, *self_obj);
  Bytes other(&scope, *other_obj);
  return Bool::fromBool(self->compare(*other) < 0);
}

RawObject BytesBuiltins::dunderNe(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  if (nargs != 2) {
    return thread->raiseTypeError(
        runtime->newStrFromFormat("expected 1 argument, got %ld", nargs - 1));
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfBytes(*self_obj)) {
    return thread->raiseTypeErrorWithCStr("'__ne__' requires a 'bytes' object");
  }
  Object other_obj(&scope, args.get(1));
  if (!runtime->isInstanceOfBytes(*other_obj)) {
    return runtime->notImplemented();
  }
  Bytes self(&scope, *self_obj);
  Bytes other(&scope, *other_obj);
  return Bool::fromBool(self->compare(*other) != 0);
}

}  // namespace python
