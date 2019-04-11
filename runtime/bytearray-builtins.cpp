#include "bytearray-builtins.h"

#include "bytes-builtins.h"
#include "int-builtins.h"
#include "runtime.h"
#include "slice-builtins.h"

namespace python {

RawObject byteArrayAsBytes(Thread* thread, Runtime* runtime,
                           const ByteArray& array) {
  HandleScope scope(thread);
  Bytes bytes(&scope, array.bytes());
  return runtime->bytesSubseq(thread, bytes, 0, array.numItems());
}

void writeByteAsHexDigits(Thread* thread, const ByteArray& array, byte value) {
  const byte* hex_digits = reinterpret_cast<const byte*>("0123456789abcdef");
  const byte bytes[] = {hex_digits[value >> 4], hex_digits[value & 0xf]};
  thread->runtime()->byteArrayExtend(thread, array, bytes);
}

const BuiltinAttribute ByteArrayBuiltins::kAttributes[] = {
    {SymbolId::kInvalid, ByteArray::kBytesOffset},
    {SymbolId::kInvalid, ByteArray::kNumItemsOffset},
    {SymbolId::kSentinelId, -1},
};

const BuiltinMethod ByteArrayBuiltins::kBuiltinMethods[] = {
    {SymbolId::kDunderAdd, dunderAdd},
    {SymbolId::kDunderGetitem, dunderGetItem},
    {SymbolId::kDunderIadd, dunderIadd},
    {SymbolId::kDunderImul, dunderImul},
    {SymbolId::kDunderInit, dunderInit},
    {SymbolId::kDunderLen, dunderLen},
    {SymbolId::kDunderMul, dunderMul},
    {SymbolId::kDunderNew, dunderNew},
    {SymbolId::kDunderRepr, dunderRepr},
    {SymbolId::kHex, hex},
    {SymbolId::kSentinelId, nullptr},
};

RawObject ByteArrayBuiltins::dunderAdd(Thread* thread, Frame* frame,
                                       word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfByteArray(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kByteArray);
  }
  Object other_obj(&scope, args.get(1));
  word other_len;
  if (runtime->isInstanceOfByteArray(*other_obj)) {
    ByteArray array(&scope, *other_obj);
    other_len = array.numItems();
    other_obj = array.bytes();
  } else if (runtime->isInstanceOfBytes(*other_obj)) {
    other_len = Bytes::cast(*other_obj).length();
  } else {
    return thread->raiseTypeErrorWithCStr(
        "can only concatenate bytearray or bytes to bytearray");
  }

  ByteArray self(&scope, *self_obj);
  Bytes self_bytes(&scope, self.bytes());
  word self_len = self.numItems();
  Bytes other_bytes(&scope, *other_obj);

  ByteArray result(&scope, runtime->newByteArray());
  runtime->byteArrayEnsureCapacity(thread, result, self_len + other_len);
  runtime->byteArrayIadd(thread, result, self_bytes, self_len);
  runtime->byteArrayIadd(thread, result, other_bytes, other_len);
  return *result;
}

RawObject ByteArrayBuiltins::dunderGetItem(Thread* thread, Frame* frame,
                                           word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfByteArray(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kByteArray);
  }
  ByteArray self(&scope, *self_obj);
  Object index(&scope, args.get(1));
  if (runtime->isInstanceOfInt(*index)) {
    // TODO(T38780562): Handle Int subclasses
    if (!index.isInt()) {
      UNIMPLEMENTED("subclass of int");
    }
    if (!index.isSmallInt()) {
      return thread->raiseIndexErrorWithCStr(
          "cannot fit index into an index-sized integer");
    }
    word idx = SmallInt::cast(*index).value();
    word len = self.numItems();
    if (idx < 0) idx += len;
    if (idx < 0 || idx >= len) {
      return thread->raiseIndexErrorWithCStr("index out of range");
    }
    return SmallInt::fromWord(self.byteAt(idx));
  }
  if (index.isSlice()) {
    Slice slice(&scope, *index);
    word start, stop, step;
    Object err(&scope, sliceUnpack(thread, slice, &start, &stop, &step));
    if (err.isError()) return *err;
    word len = Slice::adjustIndices(self.numItems(), &start, &stop, step);
    ByteArray result(&scope, runtime->newByteArray());
    runtime->byteArrayEnsureCapacity(thread, result, len);
    result.setNumItems(len);
    for (word i = 0, idx = start; i < len; i++, idx += step) {
      result.byteAtPut(i, self.byteAt(idx));
    }
    return *result;
  }
  return thread->raiseTypeErrorWithCStr(
      "bytearray indices must either be slice or provide '__index__'");
}

RawObject ByteArrayBuiltins::dunderIadd(Thread* thread, Frame* frame,
                                        word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfByteArray(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kByteArray);
  }
  ByteArray self(&scope, *self_obj);
  Object other_obj(&scope, args.get(1));
  word other_len;
  if (runtime->isInstanceOfByteArray(*other_obj)) {
    ByteArray array(&scope, *other_obj);
    other_len = array.numItems();
    other_obj = array.bytes();
  } else if (runtime->isInstanceOfBytes(*other_obj)) {
    other_len = Bytes::cast(*other_obj).length();
  } else {
    return thread->raiseTypeErrorWithCStr(
        "can only concatenate bytearray or bytes to bytearray");
  }
  Bytes other(&scope, *other_obj);
  runtime->byteArrayIadd(thread, self, other, other_len);
  return *self;
}

RawObject ByteArrayBuiltins::dunderImul(Thread* thread, Frame* frame,
                                        word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfByteArray(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kByteArray);
  }
  ByteArray self(&scope, *self_obj);
  Object count_obj(&scope, args.get(1));
  count_obj = intFromIndex(thread, count_obj);
  if (count_obj.isError()) return *count_obj;
  Int count_int(&scope, *count_obj);
  word count = count_int.asWordSaturated();
  if (!SmallInt::isValid(count)) {
    return thread->raiseOverflowErrorWithCStr(
        "cannot fit count into an index-sized integer");
  }
  word length = self.numItems();
  if (count <= 0 || length == 0) {
    self.downsize(0);
    return *self;
  }
  if (count == 1) {
    return *self;
  }
  word new_length;
  if (__builtin_mul_overflow(length, count, &new_length) ||
      !SmallInt::isValid(new_length)) {
    return thread->raiseMemoryError();
  }
  Bytes source(&scope, self.bytes());
  self.setBytes(runtime->bytesRepeat(thread, source, length, count));
  self.setNumItems(self.capacity());
  return *self;
}

RawObject ByteArrayBuiltins::dunderInit(Thread* thread, Frame* frame,
                                        word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfByteArray(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kByteArray);
  }
  ByteArray self(&scope, *self_obj);
  // always clear the existing contents of the array
  if (self.numItems() > 0) {
    self.downsize(0);
  }
  Object source(&scope, args.get(1));
  Object encoding(&scope, args.get(2));
  Object errors(&scope, args.get(3));
  if (source.isUnbound()) {
    if (encoding.isUnbound() && errors.isUnbound()) {
      return NoneType::object();
    }
    return thread->raiseTypeErrorWithCStr(
        "encoding or errors without sequence argument");
  }
  if (runtime->isInstanceOfStr(*source)) {
    if (encoding.isUnbound()) {
      return thread->raiseTypeErrorWithCStr(
          "string argument without an encoding");
    }
    UNIMPLEMENTED("string encoding");  // return NoneType::value();
  }
  if (!encoding.isUnbound() || !errors.isUnbound()) {
    return thread->raiseTypeErrorWithCStr(
        "encoding or errors without a string argument");
  }
  if (runtime->isInstanceOfInt(*source)) {
    // TODO(T38780562): Handle Int subclasses
    if (!source.isInt()) {
      UNIMPLEMENTED("int subclassing");
    }
    if (!source.isSmallInt()) {
      return thread->raiseOverflowErrorWithCStr(
          "cannot fit count into an index-sized integer");
    }
    word count = RawSmallInt::cast(*source).value();
    if (count < 0) {
      return thread->raiseValueErrorWithCStr("negative count");
    }
    runtime->byteArrayEnsureCapacity(thread, self, count);
    self.setNumItems(count);
  } else if (runtime->isInstanceOfBytes(*source)) {  // TODO(T38246066)
    Bytes bytes(&scope, *source);
    runtime->byteArrayIadd(thread, self, bytes, bytes.length());
  } else if (runtime->isInstanceOfByteArray(*source)) {
    ByteArray array(&scope, *source);
    Bytes bytes(&scope, array.bytes());
    runtime->byteArrayIadd(thread, self, bytes, array.numItems());
  } else {
    Object maybe_bytes(
        &scope, thread->invokeFunction1(SymbolId::kBuiltins,
                                        SymbolId::kUnderBytesNew, source));
    if (maybe_bytes.isError()) return *maybe_bytes;
    Bytes bytes(&scope, *maybe_bytes);
    runtime->byteArrayIadd(thread, self, bytes, bytes.length());
  }
  return NoneType::object();
}

RawObject ByteArrayBuiltins::dunderLen(Thread* thread, Frame* frame,
                                       word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfByteArray(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kByteArray);
  }
  ByteArray self(&scope, *self_obj);
  return SmallInt::fromWord(self.numItems());
}

RawObject ByteArrayBuiltins::dunderMul(Thread* thread, Frame* frame,
                                       word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfByteArray(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kByteArray);
  }
  ByteArray self(&scope, *self_obj);
  Object count_obj(&scope, args.get(1));
  count_obj = intFromIndex(thread, count_obj);
  if (count_obj.isError()) return *count_obj;
  Int count_int(&scope, *count_obj);
  word count = count_int.asWordSaturated();
  if (!SmallInt::isValid(count)) {
    return thread->raiseOverflowErrorWithCStr(
        "cannot fit count into an index-sized integer");
  }
  word length = self.numItems();
  if (count <= 0 || length == 0) {
    return runtime->newByteArray();
  }
  word new_length;
  if (__builtin_mul_overflow(length, count, &new_length) ||
      !SmallInt::isValid(new_length)) {
    return thread->raiseMemoryError();
  }
  Bytes source(&scope, self.bytes());
  ByteArray result(&scope, runtime->newByteArray());
  result.setBytes(runtime->bytesRepeat(thread, source, length, count));
  result.setNumItems(result.capacity());
  return *result;
}

RawObject ByteArrayBuiltins::dunderNew(Thread* thread, Frame* frame,
                                       word nargs) {
  Arguments args(frame, nargs);
  if (!args.get(0).isType()) {
    return thread->raiseTypeErrorWithCStr("not a type object");
  }
  HandleScope scope(thread);
  Type type(&scope, args.get(0));
  if (type.builtinBase() != LayoutId::kByteArray) {
    return thread->raiseTypeErrorWithCStr("not a subtype of bytearray");
  }
  Layout layout(&scope, type.instanceLayout());
  Runtime* runtime = thread->runtime();
  ByteArray result(&scope, runtime->newInstance(layout));
  result.setNumItems(0);
  result.setBytes(runtime->newBytes(0, 0));
  return *result;
}

RawObject ByteArrayBuiltins::dunderRepr(Thread* thread, Frame* frame,
                                        word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfByteArray(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kByteArray);
  }

  ByteArray self(&scope, *self_obj);
  word length = self.numItems();
  word affix_length = 14;  // strlen("bytearray(b'')") == 14
  if (length > (kMaxWord - affix_length) / 4) {
    return thread->raiseOverflowErrorWithCStr(
        "bytearray object is too large to make repr");
  }

  // Figure out which quote to use; single is preferred
  bool has_single_quote = false;
  bool has_double_quote = false;
  for (word i = 0; i < length; i++) {
    byte current = self.byteAt(i);
    if (current == '\'') {
      has_single_quote = true;
    } else if (current == '"') {
      has_double_quote = true;
      break;
    }
  }
  byte quote = (has_single_quote && !has_double_quote) ? '"' : '\'';

  // Each byte will be mapped to one or more ASCII characters.
  ByteArray buffer(&scope, runtime->newByteArray());
  runtime->byteArrayEnsureCapacity(thread, buffer, length + affix_length);
  const byte bytearray_str[] = {'b', 'y', 't', 'e', 'a', 'r',
                                'r', 'a', 'y', '(', 'b', quote};
  runtime->byteArrayExtend(thread, buffer, bytearray_str);
  for (word i = 0; i < length; i++) {
    byte current = self.byteAt(i);
    if (current == quote || current == '\\') {
      const byte bytes[] = {'\\', current};
      runtime->byteArrayExtend(thread, buffer, bytes);
    } else if (current == '\t') {
      const byte bytes[] = {'\\', 't'};
      runtime->byteArrayExtend(thread, buffer, bytes);
    } else if (current == '\n') {
      const byte bytes[] = {'\\', 'n'};
      runtime->byteArrayExtend(thread, buffer, bytes);
    } else if (current == '\r') {
      const byte bytes[] = {'\\', 'r'};
      runtime->byteArrayExtend(thread, buffer, bytes);
    } else if (current < ' ' || current >= 0x7f) {
      const byte bytes[] = {'\\', 'x'};
      runtime->byteArrayExtend(thread, buffer, bytes);
      writeByteAsHexDigits(thread, buffer, current);
    } else {
      byteArrayAdd(thread, runtime, buffer, current);
    }
  }

  const byte quote_with_paren[] = {quote, ')'};
  runtime->byteArrayExtend(thread, buffer, quote_with_paren);
  return runtime->newStrFromByteArray(buffer);
}

RawObject ByteArrayBuiltins::hex(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object obj(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfByteArray(*obj)) {
    return thread->raiseRequiresType(obj, SymbolId::kByteArray);
  }
  ByteArray self(&scope, *obj);
  Bytes bytes(&scope, self.bytes());
  return bytesHex(thread, bytes, self.numItems());
}

RawObject ByteArrayBuiltins::join(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  ByteArray sep(&scope, args.get(0));
  Object iterable(&scope, args.get(1));
  Object joined(&scope, NoneType::object());
  Runtime* runtime = thread->runtime();
  if (iterable.isList()) {
    List list(&scope, *iterable);
    Tuple src(&scope, list.items());
    joined = runtime->bytesJoin(thread, sep, src, list.numItems());
  } else if (iterable.isTuple()) {
    Tuple src(&scope, *iterable);
    joined = runtime->bytesJoin(thread, sep, src, src.length());
  }
  // Check for error or slow path
  if (!joined.isBytes()) return *joined;
  ByteArray result(&scope, runtime->newByteArray());
  result.setBytes(*joined);
  result.setNumItems(Bytes::cast(*joined).length());
  return *result;
}

}  // namespace python
