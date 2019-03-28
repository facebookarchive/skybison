#include "bytes-builtins.h"

#include "bytearray-builtins.h"
#include "frame.h"
#include "int-builtins.h"
#include "runtime.h"
#include "slice-builtins.h"
#include "trampolines-inl.h"

namespace python {

RawObject callDunderBytes(Thread* thread, const Object& obj) {
  HandleScope scope(thread);
  Object result(&scope, thread->invokeMethod1(obj, SymbolId::kDunderBytes));
  if (result.isError()) {
    if (!thread->hasPendingException()) {
      // Attribute lookup failed, return None
      return RawNoneType::object();
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
  if (obj.isList()) {
    List list(&scope, *obj);
    Tuple tuple(&scope, list.items());
    return bytesFromTuple(thread, tuple, list.numItems());
  }
  if (obj.isTuple()) {
    Tuple tuple(&scope, *obj);
    return bytesFromTuple(thread, tuple, tuple.length());
  }
  if (!runtime->isInstanceOfStr(*obj)) {
    Object iter(&scope, thread->invokeMethod1(obj, SymbolId::kDunderIter));
    if (iter.isError()) {
      if (!thread->hasPendingException()) {
        return thread->raiseTypeErrorWithCStr("object is not iterable");
      }
      return *iter;
    }
    Frame* frame = thread->currentFrame();
    Object next(&scope, Interpreter::lookupMethod(thread, frame, iter,
                                                  SymbolId::kDunderNext));
    if (next.isError()) {
      return thread->raiseTypeErrorWithCStr("iter() returned non-iterator");
    }
    Object value(&scope, NoneType::object());
    List buffer(&scope, runtime->newList());
    for (;;) {
      value = Interpreter::callMethod1(thread, frame, next, iter);
      if (value.isError()) {
        if (thread->clearPendingStopIteration()) break;
        return *value;
      }
      runtime->listAdd(buffer, value);
    }
    Tuple tuple(&scope, buffer.items());
    return bytesFromTuple(thread, tuple, buffer.numItems());
  }

  return thread->raiseTypeErrorWithCStr("cannot convert object to bytes");
}

RawObject bytesFromTuple(Thread* thread, const Tuple& items, word size) {
  DCHECK_BOUND(size, items.length());
  HandleScope scope(thread);
  Bytes result(&scope, thread->runtime()->newBytes(size, 0));

  for (word idx = 0; idx < size; idx++) {
    Object item(&scope, items.at(idx));
    item = intFromIndex(thread, item);
    if (item.isError()) {
      return *item;
    }

    // item is now an instance of Int
    Int index(&scope, *item);
    OptInt<byte> current_byte = index.asInt<byte>();
    switch (current_byte.error) {
      case CastError::None:
        result.byteAtPut(idx, current_byte.value);
        continue;
      case CastError::Overflow:
      case CastError::Underflow:
        return thread->raiseValueErrorWithCStr(
            "bytes must be in range(0, 256)");
    }
  }

  return *result;
}

RawObject bytesHex(Thread* thread, const Bytes& bytes, word length) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  ByteArray buffer(&scope, runtime->newByteArray());
  runtime->byteArrayEnsureCapacity(thread, buffer, length * 2);
  for (word i = 0; i < length; i++) {
    writeByteAsHexDigits(thread, buffer, bytes.byteAt(i));
  }
  return runtime->newStrFromByteArray(buffer);
}

static RawObject bytesReprWithDelimiter(Thread* thread, const Bytes& self,
                                        byte delimiter) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  ByteArray buffer(&scope, runtime->newByteArray());
  word len = self.length();
  // Each byte will be mapped to one or more ASCII characters. Add 3 to the
  // length for the 2-character prefix (b') and the 1-character suffix (').
  // We expect mostly ASCII bytes, so we usually will not have to resize again.
  runtime->byteArrayEnsureCapacity(thread, buffer, len + 3);
  runtime->byteArrayExtend(thread, buffer, {'b', delimiter});
  for (word i = 0; i < len; i++) {
    byte current = self.byteAt(i);
    if (current == delimiter || current == '\\') {
      runtime->byteArrayExtend(thread, buffer, {'\\', current});
    } else if (current == '\t') {
      runtime->byteArrayExtend(thread, buffer, {'\\', 't'});
    } else if (current == '\n') {
      runtime->byteArrayExtend(thread, buffer, {'\\', 'n'});
    } else if (current == '\r') {
      runtime->byteArrayExtend(thread, buffer, {'\\', 'r'});
    } else if (current < ' ' || current >= 0x7f) {
      runtime->byteArrayExtend(thread, buffer, {'\\', 'x'});
      writeByteAsHexDigits(thread, buffer, current);
    } else {
      byteArrayAdd(thread, runtime, buffer, current);
    }
  }
  byteArrayAdd(thread, runtime, buffer, delimiter);
  return runtime->newStrFromByteArray(buffer);
}

RawObject bytesReprSingleQuotes(Thread* thread, const Bytes& self) {
  return bytesReprWithDelimiter(thread, self, '\'');
}

RawObject bytesReprSmartQuotes(Thread* thread, const Bytes& self) {
  word len = self.length();
  bool has_single_quote = false;
  for (word i = 0; i < len; i++) {
    switch (self.byteAt(i)) {
      case '\'':
        has_single_quote = true;
        break;
      case '"':
        return bytesReprWithDelimiter(thread, self, '\'');
      default:
        break;
    }
  }
  return bytesReprWithDelimiter(thread, self, has_single_quote ? '"' : '\'');
}

const BuiltinMethod BytesBuiltins::kBuiltinMethods[] = {
    {SymbolId::kDunderAdd, dunderAdd},
    {SymbolId::kDunderEq, dunderEq},
    {SymbolId::kDunderGe, dunderGe},
    {SymbolId::kDunderGetItem, dunderGetItem},
    {SymbolId::kDunderGt, dunderGt},
    {SymbolId::kDunderLe, dunderLe},
    {SymbolId::kDunderLen, dunderLen},
    {SymbolId::kDunderLt, dunderLt},
    {SymbolId::kDunderMul, dunderMul},
    {SymbolId::kDunderNe, dunderNe},
    {SymbolId::kDunderRepr, dunderRepr},
    {SymbolId::kHex, hex},
    {SymbolId::kSentinelId, nullptr},
};

RawObject BytesBuiltins::dunderAdd(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfBytes(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kBytes);
  }
  Object other_obj(&scope, args.get(1));
  if (!runtime->isInstanceOfBytes(*other_obj)) {
    return thread->raiseRequiresType(other_obj, SymbolId::kBytes);
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
    return thread->raiseRequiresType(self_obj, SymbolId::kBytes);
  }
  Object other_obj(&scope, args.get(1));
  if (!runtime->isInstanceOfBytes(*other_obj)) {
    return NotImplementedType::object();
  }
  Bytes self(&scope, *self_obj);
  Bytes other(&scope, *other_obj);
  return Bool::fromBool(self.compare(*other) == 0);
}

RawObject BytesBuiltins::dunderGe(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfBytes(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kBytes);
  }
  Object other_obj(&scope, args.get(1));
  if (!runtime->isInstanceOfBytes(*other_obj)) {
    return NotImplementedType::object();
  }
  Bytes self(&scope, *self_obj);
  Bytes other(&scope, *other_obj);
  return Bool::fromBool(self.compare(*other) >= 0);
}

RawObject BytesBuiltins::dunderGetItem(Thread* thread, Frame* frame,
                                       word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfBytes(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kBytes);
  }
  Bytes self(&scope, *self_obj);
  Object index(&scope, args.get(1));
  // TODO(T27897506): use __index__ to get index
  if (runtime->isInstanceOfInt(*index)) {
    // TODO(T38780562): strict subclass of int
    if (!index.isSmallInt()) {
      return thread->raiseIndexErrorWithCStr(
          "cannot fit index into an index-sized integer");
    }
    word idx = RawSmallInt::cast(*index).value();
    word len = self.length();
    if (idx < 0) idx += len;
    if (idx < 0 || idx >= len) {
      return thread->raiseIndexErrorWithCStr("index out of range");
    }
    return RawSmallInt::fromWord(static_cast<word>(self.byteAt(idx)));
  }
  if (index.isSlice()) {
    Slice slice(&scope, *index);
    word start, stop, step;
    Object err(&scope, sliceUnpack(thread, slice, &start, &stop, &step));
    if (err.isError()) return *err;
    word len = RawSlice::adjustIndices(self.length(), &start, &stop, step);
    // TODO(T36997048): intern 1-element byte arrays
    Bytes result(&scope, runtime->newBytes(len, 0));
    for (word i = 0, idx = start; i < len; i++, idx += step) {
      result.byteAtPut(i, self.byteAt(idx));
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
    return thread->raiseRequiresType(self_obj, SymbolId::kBytes);
  }
  Object other_obj(&scope, args.get(1));
  if (!runtime->isInstanceOfBytes(*other_obj)) {
    return NotImplementedType::object();
  }
  Bytes self(&scope, *self_obj);
  Bytes other(&scope, *other_obj);
  return Bool::fromBool(self.compare(*other) > 0);
}

RawObject BytesBuiltins::dunderLe(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfBytes(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kBytes);
  }
  Object other_obj(&scope, args.get(1));
  if (!runtime->isInstanceOfBytes(*other_obj)) {
    return NotImplementedType::object();
  }
  Bytes self(&scope, *self_obj);
  Bytes other(&scope, *other_obj);
  return Bool::fromBool(self.compare(*other) <= 0);
}

RawObject BytesBuiltins::dunderLen(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfBytes(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kBytes);
  }

  Bytes self(&scope, *self_obj);
  return SmallInt::fromWord(self.length());
}

RawObject BytesBuiltins::dunderLt(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfBytes(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kBytes);
  }
  Object other_obj(&scope, args.get(1));
  if (!runtime->isInstanceOfBytes(*other_obj)) {
    return NotImplementedType::object();
  }
  Bytes self(&scope, *self_obj);
  Bytes other(&scope, *other_obj);
  return Bool::fromBool(self.compare(*other) < 0);
}

RawObject BytesBuiltins::dunderMul(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfBytes(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kBytes);
  }
  Object count_obj(&scope, args.get(1));
  count_obj = intFromIndex(thread, count_obj);
  if (count_obj.isError()) return *count_obj;
  Bytes self(&scope, *self_obj);
  Int count_int(&scope, *count_obj);
  word count = count_int.asWordSaturated();
  if (!SmallInt::isValid(count)) {
    return thread->raiseOverflowErrorWithCStr(
        "cannot fit count into an index-sized integer");
  }
  word length = self.length();
  if (count <= 0 || length == 0) {
    return runtime->newBytes(0, 0);
  }
  if (count == 1) {
    return *self;
  }
  word new_length;
  if (__builtin_mul_overflow(length, count, &new_length) ||
      !SmallInt::isValid(new_length)) {
    return thread->raiseOverflowErrorWithCStr("repeated bytes are too long");
  }
  return runtime->bytesRepeat(thread, self, length, count);
}

RawObject BytesBuiltins::dunderNe(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfBytes(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kBytes);
  }
  Object other_obj(&scope, args.get(1));
  if (!runtime->isInstanceOfBytes(*other_obj)) {
    return NotImplementedType::object();
  }
  Bytes self(&scope, *self_obj);
  Bytes other(&scope, *other_obj);
  return Bool::fromBool(self.compare(*other) != 0);
}

RawObject underBytesNew(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  // TODO(wmeehan): implement bytes subclasses
  // Type type(&scope, args.get(0));
  Object source(&scope, args.get(1));
  Runtime* runtime = thread->runtime();
  // if source is an integer, interpret it as the length of a zero-filled bytes
  if (runtime->isInstanceOfInt(*source)) {
    Int src(&scope, *source);
    word size = src.asWordSaturated();
    if (!SmallInt::isValid(size)) {
      return thread->raiseOverflowErrorWithCStr(
          "cannot fit into an index-sized integer");
    }
    if (size < 0) {
      return thread->raiseValueErrorWithCStr("negative count");
    }
    return runtime->newBytes(size, 0);
  }
  if (source.isBytes()) {
    return *source;
  }
  // last option: source is an iterator that produces bytes
  return bytesFromIterable(thread, source);
}

RawObject BytesBuiltins::dunderRepr(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfBytes(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kBytes);
  }
  Bytes self(&scope, *self_obj);
  return bytesReprSmartQuotes(thread, self);
}

RawObject BytesBuiltins::hex(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object obj(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfBytes(*obj)) {
    return thread->raiseRequiresType(obj, SymbolId::kBytes);
  }
  Bytes self(&scope, *obj);
  return bytesHex(thread, self, self.length());
}

RawObject BytesBuiltins::join(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Bytes self(&scope, args.get(0));
  Object iterable(&scope, args.get(1));
  if (iterable.isList()) {
    List list(&scope, *iterable);
    Tuple src(&scope, list.items());
    return thread->runtime()->bytesJoin(thread, self, src, list.numItems());
  }
  if (iterable.isTuple()) {
    Tuple src(&scope, *iterable);
    return thread->runtime()->bytesJoin(thread, self, src, src.length());
  }
  // Slow path: collect items into list in Python and call again
  return NoneType::object();
}

}  // namespace python
