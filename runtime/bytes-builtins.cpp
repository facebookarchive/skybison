#include "bytes-builtins.h"

#include "frame.h"
#include "runtime.h"
#include "slice-builtins.h"
#include "trampolines-inl.h"

namespace python {

RawObject asBytes(Thread* thread, const Object& obj) {
  HandleScope scope(thread);
  Object result(&scope, thread->invokeMethod1(obj, SymbolId::kDunderBytes));
  if (result->isError()) {
    if (!thread->hasPendingException()) {
      // Attribute lookup failed, try PyBytes_FromObject
      return bytesFromIterable(thread, obj);
    }
    return *result;
  }
  if (!thread->runtime()->isInstanceOfBytes(*result)) {
    return thread->raiseTypeErrorWithCStr("__bytes__ returned non-bytes");
  }
  return *result;
}

RawObject bytesFromIterable(Thread* thread, const Object& obj) {
  // TODO(T38246066): objects other than bytes (and subclasses) as buffers
  Runtime* runtime = thread->runtime();
  if (runtime->isInstanceOfBytes(*obj)) {
    UNIMPLEMENTED("strict subclass of bytes");  // TODO(T36619847)
  }
  HandleScope scope(thread);
  if (obj->isList()) {
    List list(&scope, *obj);
    Tuple tuple(&scope, list->items());
    return bytesFromTuple(thread, tuple, list->numItems());
  }
  if (obj->isTuple()) {
    Tuple tuple(&scope, *obj);
    return bytesFromTuple(thread, tuple, tuple->length());
  }
  if (!runtime->isInstanceOfStr(*obj)) {
    Object iter(&scope, thread->invokeMethod1(obj, SymbolId::kDunderIter));
    if (iter->isError()) {
      if (!thread->hasPendingException()) {
        return thread->raiseTypeErrorWithCStr("object is not iterable");
      }
      return *iter;
    }
    Frame* frame = thread->currentFrame();
    Object next(&scope, Interpreter::lookupMethod(thread, frame, iter,
                                                  SymbolId::kDunderNext));
    if (next->isError()) {
      return thread->raiseTypeErrorWithCStr("iter() returned non-iterator");
    }
    Object value(&scope, NoneType::object());
    List buffer(&scope, runtime->newList());
    for (;;) {
      value = Interpreter::callMethod1(thread, frame, next, iter);
      if (value->isError()) {
        if (thread->clearPendingStopIteration()) break;
        return *value;
      }
      runtime->listAdd(buffer, value);
    }
    Tuple tuple(&scope, buffer->items());
    return bytesFromTuple(thread, tuple, buffer->numItems());
  }

  return thread->raiseTypeErrorWithCStr("cannot convert object to bytes");
}

RawObject bytesFromTuple(Thread* thread, const Tuple& items, word size) {
  DCHECK_BOUND(size, items->length());
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Bytes result(&scope, runtime->newBytes(size, 0));

  for (word idx = 0; idx < size; idx++) {
    Object item(&scope, items->at(idx));
    if (!runtime->isInstanceOfInt(*item)) {
      Object index(&scope, thread->invokeMethod1(item, SymbolId::kDunderIndex));
      if (index->isError()) {
        if (!thread->hasPendingException()) {
          return thread->raiseTypeErrorWithCStr(
              "object cannot be interpreted as an integer");
        }
        return *index;
      }
      if (!runtime->isInstanceOfInt(*index)) {
        return thread->raiseTypeErrorWithCStr("__index__() returned non-int");
      }
      item = *index;
    }

    // item is now an instance of Int
    OptInt<byte> current_byte = RawInt::cast(*item).asInt<byte>();
    switch (current_byte.error) {
      case CastError::None:
        result->byteAtPut(idx, current_byte.value);
        continue;
      case CastError::Overflow:
      case CastError::Underflow:
        return thread->raiseValueErrorWithCStr(
            "bytes must be in range(0, 256)");
    }
  }

  return *result;
}

const BuiltinMethod BytesBuiltins::kMethods[] = {
    {SymbolId::kDunderAdd, builtinTrampolineWrapper<dunderAdd>},
    {SymbolId::kDunderEq, builtinTrampolineWrapper<dunderEq>},
    {SymbolId::kDunderGe, builtinTrampolineWrapper<dunderGe>},
    {SymbolId::kDunderGetItem, builtinTrampolineWrapper<dunderGetItem>},
    {SymbolId::kDunderGt, builtinTrampolineWrapper<dunderGt>},
    {SymbolId::kDunderLe, builtinTrampolineWrapper<dunderLe>},
    {SymbolId::kDunderLen, builtinTrampolineWrapper<dunderLen>},
    {SymbolId::kDunderLt, builtinTrampolineWrapper<dunderLt>},
    {SymbolId::kDunderNe, builtinTrampolineWrapper<dunderNe>},
};

void BytesBuiltins::initialize(Runtime* runtime) {
  HandleScope scope;
  Type type(&scope, runtime->addBuiltinTypeWithMethods(
                        SymbolId::kBytes, LayoutId::kBytes, LayoutId::kObject,
                        kMethods));
}

RawObject BytesBuiltins::dunderAdd(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
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
    Object err(&scope, sliceUnpack(thread, slice, &start, &stop, &step));
    if (err->isError()) return *err;
    word len = RawSlice::adjustIndices(self->length(), &start, &stop, step);
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
