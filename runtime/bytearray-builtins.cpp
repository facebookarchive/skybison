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
    {SymbolId::kDunderEq, dunderEq},
    {SymbolId::kDunderGe, dunderGe},
    {SymbolId::kDunderGetitem, dunderGetItem},
    {SymbolId::kDunderGt, dunderGt},
    {SymbolId::kDunderIadd, dunderIadd},
    {SymbolId::kDunderImul, dunderImul},
    {SymbolId::kDunderInit, dunderInit},
    {SymbolId::kDunderIter, dunderIter},
    {SymbolId::kDunderLe, dunderLe},
    {SymbolId::kDunderLen, dunderLen},
    {SymbolId::kDunderLt, dunderLt},
    {SymbolId::kDunderMul, dunderMul},
    {SymbolId::kDunderNe, dunderNe},
    {SymbolId::kDunderNew, dunderNew},
    {SymbolId::kDunderRepr, dunderRepr},
    {SymbolId::kHex, hex},
    {SymbolId::kLStrip, lstrip},
    {SymbolId::kRStrip, rstrip},
    {SymbolId::kStrip, strip},
    {SymbolId::kTranslate, translate},
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
    Bytes bytes(&scope, bytesUnderlying(thread, other_obj));
    other_len = bytes.length();
    other_obj = *bytes;
  } else {
    return thread->raiseWithFmt(
        LayoutId::kTypeError,
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

RawObject ByteArrayBuiltins::dunderEq(Thread* thread, Frame* frame,
                                      word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfByteArray(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kByteArray);
  }
  ByteArray self(&scope, *self_obj);
  Object other_obj(&scope, args.get(1));
  word comparison;
  if (runtime->isInstanceOfBytes(*other_obj)) {
    Bytes other(&scope, bytesUnderlying(thread, other_obj));
    comparison = self.compare(*other, other.length());
  } else if (runtime->isInstanceOfByteArray(*other_obj)) {
    ByteArray other(&scope, *other_obj);
    Bytes other_bytes(&scope, other.bytes());
    comparison = self.compare(*other_bytes, other.numItems());
  } else {
    // TODO(T38246066): allow any bytes-like object
    return NotImplementedType::object();
  }
  return Bool::fromBool(comparison == 0);
}

RawObject ByteArrayBuiltins::dunderGe(Thread* thread, Frame* frame,
                                      word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfByteArray(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kByteArray);
  }
  ByteArray self(&scope, *self_obj);
  Object other_obj(&scope, args.get(1));
  word comparison;
  if (runtime->isInstanceOfBytes(*other_obj)) {
    Bytes other(&scope, bytesUnderlying(thread, other_obj));
    comparison = self.compare(*other, other.length());
  } else if (runtime->isInstanceOfByteArray(*other_obj)) {
    ByteArray other(&scope, *other_obj);
    Bytes other_bytes(&scope, other.bytes());
    comparison = self.compare(*other_bytes, other.numItems());
  } else {
    // TODO(T38246066): allow any bytes-like object
    return NotImplementedType::object();
  }
  return Bool::fromBool(comparison >= 0);
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
  Object index_obj(&scope, args.get(1));
  if (runtime->isInstanceOfInt(*index_obj)) {
    Int index(&scope, intUnderlying(thread, index_obj));
    if (index.isLargeInt()) {
      return thread->raiseWithFmt(LayoutId::kIndexError,
                                  "cannot fit '%T' into an index-sized integer",
                                  &index_obj);
    }
    word idx = index.asWord();
    word len = self.numItems();
    if (idx < 0) idx += len;
    if (idx < 0 || idx >= len) {
      return thread->raiseWithFmt(LayoutId::kIndexError, "index out of range");
    }
    return SmallInt::fromWord(self.byteAt(idx));
  }
  if (index_obj.isSlice()) {
    Slice slice(&scope, *index_obj);
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
  return thread->raiseWithFmt(
      LayoutId::kTypeError,
      "bytearray indices must either be slice or provide '__index__'");
}

RawObject ByteArrayBuiltins::dunderGt(Thread* thread, Frame* frame,
                                      word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfByteArray(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kByteArray);
  }
  ByteArray self(&scope, *self_obj);
  Object other_obj(&scope, args.get(1));
  word comparison;
  if (runtime->isInstanceOfBytes(*other_obj)) {
    Bytes other(&scope, bytesUnderlying(thread, other_obj));
    comparison = self.compare(*other, other.length());
  } else if (runtime->isInstanceOfByteArray(*other_obj)) {
    ByteArray other(&scope, *other_obj);
    Bytes other_bytes(&scope, other.bytes());
    comparison = self.compare(*other_bytes, other.numItems());
  } else {
    // TODO(T38246066): allow any bytes-like object
    return NotImplementedType::object();
  }
  return Bool::fromBool(comparison > 0);
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
    Bytes bytes(&scope, bytesUnderlying(thread, other_obj));
    other_len = bytes.length();
    other_obj = *bytes;
  } else {
    return thread->raiseWithFmt(
        LayoutId::kTypeError,
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
  Object count_index(&scope, args.get(1));
  Object count_obj(&scope, intFromIndex(thread, count_index));
  if (count_obj.isError()) return *count_obj;
  Int count_int(&scope, intUnderlying(thread, count_obj));
  word count = count_int.asWordSaturated();
  if (!SmallInt::isValid(count)) {
    return thread->raiseWithFmt(LayoutId::kOverflowError,
                                "cannot fit '%T' into an index-sized integer",
                                &count_index);
  }
  if (count == 1) {
    return *self;
  }
  word length = self.numItems();
  if (count <= 0 || length == 0) {
    self.downsize(0);
    return *self;
  }
  word new_length;
  if (__builtin_mul_overflow(length, count, &new_length) ||
      !SmallInt::isValid(new_length)) {
    return thread->raiseMemoryError();
  }
  Bytes source(&scope, self.bytes());
  if (new_length <= self.capacity()) {
    // fits into existing backing LargeBytes - repeat in place
    for (word i = 1; i < count; i++) {
      runtime->byteArrayIadd(thread, self, source, length);
    }
    return *self;
  }
  // grows beyond existing bytes - allocate new
  self.setBytes(runtime->bytesRepeat(thread, source, length, count));
  DCHECK(self.capacity() == new_length, "unexpected result length");
  self.setNumItems(new_length);
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
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "encoding or errors without sequence argument");
  }
  if (runtime->isInstanceOfStr(*source)) {
    if (encoding.isUnbound()) {
      return thread->raiseWithFmt(LayoutId::kTypeError,
                                  "string argument without an encoding");
    }
    Object encoded(
        &scope, errors.isUnbound()
                    ? thread->invokeMethod2(source, SymbolId::kEncode, encoding)
                    : thread->invokeMethod3(source, SymbolId::kEncode, encoding,
                                            errors));
    if (encoded.isError()) {
      DCHECK(!encoded.isErrorNotFound(), "str.encode() not found");
      return *encoded;
    }
    Bytes bytes(&scope, *encoded);
    runtime->byteArrayIadd(thread, self, bytes, bytes.length());
    return NoneType::object();
  }
  if (!encoding.isUnbound() || !errors.isUnbound()) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "encoding or errors without a string argument");
  }
  if (runtime->isInstanceOfInt(*source)) {
    Int count_int(&scope, intUnderlying(thread, source));
    if (count_int.isLargeInt()) {
      return thread->raiseWithFmt(
          LayoutId::kOverflowError,
          "cannot fit count into an index-sized integer", &source);
    }
    word count = count_int.asWord();
    if (count < 0) {
      return thread->raiseWithFmt(LayoutId::kValueError, "negative count");
    }
    runtime->byteArrayEnsureCapacity(thread, self, count);
    self.setNumItems(count);
  } else if (runtime->isInstanceOfBytes(*source)) {  // TODO(T38246066)
    Bytes bytes(&scope, bytesUnderlying(thread, source));
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

RawObject ByteArrayBuiltins::dunderIter(Thread* thread, Frame* frame,
                                        word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfByteArray(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kByteArray);
  }
  ByteArray self(&scope, *self_obj);
  return runtime->newByteArrayIterator(thread, self);
}

RawObject ByteArrayBuiltins::dunderLe(Thread* thread, Frame* frame,
                                      word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfByteArray(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kByteArray);
  }
  ByteArray self(&scope, *self_obj);
  Object other_obj(&scope, args.get(1));
  word comparison;
  if (runtime->isInstanceOfBytes(*other_obj)) {
    Bytes other(&scope, bytesUnderlying(thread, other_obj));
    comparison = self.compare(*other, other.length());
  } else if (runtime->isInstanceOfByteArray(*other_obj)) {
    ByteArray other(&scope, *other_obj);
    Bytes other_bytes(&scope, other.bytes());
    comparison = self.compare(*other_bytes, other.numItems());
  } else {
    // TODO(T38246066): allow any bytes-like object
    return NotImplementedType::object();
  }
  return Bool::fromBool(comparison <= 0);
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

RawObject ByteArrayBuiltins::dunderLt(Thread* thread, Frame* frame,
                                      word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfByteArray(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kByteArray);
  }
  ByteArray self(&scope, *self_obj);
  Object other_obj(&scope, args.get(1));
  word comparison;
  if (runtime->isInstanceOfBytes(*other_obj)) {
    Bytes other(&scope, bytesUnderlying(thread, other_obj));
    comparison = self.compare(*other, other.length());
  } else if (runtime->isInstanceOfByteArray(*other_obj)) {
    ByteArray other(&scope, *other_obj);
    Bytes other_bytes(&scope, other.bytes());
    comparison = self.compare(*other_bytes, other.numItems());
  } else {
    // TODO(T38246066): allow any bytes-like object
    return NotImplementedType::object();
  }
  return Bool::fromBool(comparison < 0);
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
  Object count_index(&scope, args.get(1));
  Object count_obj(&scope, intFromIndex(thread, count_index));
  if (count_obj.isError()) return *count_obj;
  Int count_int(&scope, intUnderlying(thread, count_obj));
  word count = count_int.asWordSaturated();
  if (!SmallInt::isValid(count)) {
    return thread->raiseWithFmt(LayoutId::kOverflowError,
                                "cannot fit '%T' into an index-sized integer",
                                &count_index);
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
  Bytes repeated(&scope, runtime->bytesRepeat(thread, source, length, count));
  DCHECK(repeated.length() == new_length, "unexpected result length");
  if (repeated.isSmallBytes()) {
    runtime->byteArrayIadd(thread, result, repeated, new_length);
  } else {
    result.setBytes(*repeated);
    result.setNumItems(new_length);
  }
  return *result;
}

RawObject ByteArrayBuiltins::dunderNe(Thread* thread, Frame* frame,
                                      word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfByteArray(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kByteArray);
  }
  ByteArray self(&scope, *self_obj);
  Object other_obj(&scope, args.get(1));
  word comparison;
  if (runtime->isInstanceOfBytes(*other_obj)) {
    Bytes other(&scope, bytesUnderlying(thread, other_obj));
    comparison = self.compare(*other, other.length());
  } else if (runtime->isInstanceOfByteArray(*other_obj)) {
    ByteArray other(&scope, *other_obj);
    Bytes other_bytes(&scope, other.bytes());
    comparison = self.compare(*other_bytes, other.numItems());
  } else {
    // TODO(T38246066): allow any bytes-like object
    return NotImplementedType::object();
  }
  return Bool::fromBool(comparison != 0);
}

RawObject ByteArrayBuiltins::dunderNew(Thread* thread, Frame* frame,
                                       word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object type_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfType(*type_obj)) {
    return thread->raiseWithFmt(LayoutId::kTypeError, "not a type object");
  }
  Type type(&scope, *type_obj);
  if (type.builtinBase() != LayoutId::kByteArray) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "not a subtype of bytearray");
  }
  Layout layout(&scope, type.instanceLayout());
  ByteArray result(&scope, runtime->newInstance(layout));
  result.setBytes(runtime->emptyMutableBytes());
  result.setNumItems(0);
  return *result;
}

// TODO(T48660163): Escape single quotes
RawObject byteArrayReprSmartQuotes(Thread* thread, const ByteArray& self) {
  word length = self.numItems();
  word affix_length = 14;  // strlen("bytearray(b'')") == 14
  if (length > (kMaxWord - affix_length) / 4) {
    return thread->raiseWithFmt(LayoutId::kOverflowError,
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

  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  ByteArray buffer(&scope, runtime->newByteArray());
  // Each byte will be mapped to one or more ASCII characters.
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
  return byteArrayReprSmartQuotes(thread, self);
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

RawObject ByteArrayBuiltins::lstrip(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfByteArray(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kByteArray);
  }
  ByteArray self(&scope, *self_obj);
  Bytes self_bytes(&scope, self.bytes());
  Object chars_obj(&scope, args.get(1));
  Bytes result_bytes(&scope, Bytes::empty());
  if (chars_obj.isNoneType()) {
    result_bytes = bytesStripSpaceLeft(thread, self_bytes, self.numItems());
  } else if (runtime->isInstanceOfBytes(*chars_obj)) {
    Bytes chars(&scope, bytesUnderlying(thread, chars_obj));
    result_bytes = bytesStripLeft(thread, self_bytes, self.numItems(), chars,
                                  chars.length());
  } else if (runtime->isInstanceOfByteArray(*chars_obj)) {
    ByteArray chars(&scope, *chars_obj);
    Bytes chars_bytes(&scope, chars.bytes());
    result_bytes = bytesStripLeft(thread, self_bytes, self.numItems(),
                                  chars_bytes, chars.numItems());
  } else {
    // TODO(T38246066): support bytes-like objects other than bytes, bytearray
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "a bytes-like object is required, not '%T'",
                                &chars_obj);
  }
  ByteArray result(&scope, runtime->newByteArray());
  runtime->byteArrayIadd(thread, result, result_bytes, result_bytes.length());
  return *result;
}

RawObject ByteArrayBuiltins::rstrip(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfByteArray(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kByteArray);
  }
  ByteArray self(&scope, *self_obj);
  Bytes self_bytes(&scope, self.bytes());
  Object chars_obj(&scope, args.get(1));
  Bytes result_bytes(&scope, Bytes::empty());
  if (chars_obj.isNoneType()) {
    result_bytes = bytesStripSpaceRight(thread, self_bytes, self.numItems());
  } else if (runtime->isInstanceOfBytes(*chars_obj)) {
    Bytes chars(&scope, bytesUnderlying(thread, chars_obj));
    result_bytes = bytesStripRight(thread, self_bytes, self.numItems(), chars,
                                   chars.length());
  } else if (runtime->isInstanceOfByteArray(*chars_obj)) {
    ByteArray chars(&scope, *chars_obj);
    Bytes chars_bytes(&scope, chars.bytes());
    result_bytes = bytesStripRight(thread, self_bytes, self.numItems(),
                                   chars_bytes, chars.numItems());
  } else {
    // TODO(T38246066): support bytes-like objects other than bytes, bytearray
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "a bytes-like object is required, not '%T'",
                                &chars_obj);
  }
  ByteArray result(&scope, runtime->newByteArray());
  runtime->byteArrayIadd(thread, result, result_bytes, result_bytes.length());
  return *result;
}

RawObject ByteArrayBuiltins::strip(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfByteArray(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kByteArray);
  }
  ByteArray self(&scope, *self_obj);
  Bytes self_bytes(&scope, self.bytes());
  Object chars_obj(&scope, args.get(1));
  Bytes result_bytes(&scope, Bytes::empty());
  if (chars_obj.isNoneType()) {
    result_bytes = bytesStripSpace(thread, self_bytes, self.numItems());
  } else if (runtime->isInstanceOfBytes(*chars_obj)) {
    Bytes chars(&scope, bytesUnderlying(thread, chars_obj));
    result_bytes =
        bytesStrip(thread, self_bytes, self.numItems(), chars, chars.length());
  } else if (runtime->isInstanceOfByteArray(*chars_obj)) {
    ByteArray chars(&scope, *chars_obj);
    Bytes chars_bytes(&scope, chars.bytes());
    result_bytes = bytesStrip(thread, self_bytes, self.numItems(), chars_bytes,
                              chars.numItems());
  } else {
    // TODO(T38246066): support bytes-like objects other than bytes, bytearray
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "a bytes-like object is required, not '%T'",
                                &chars_obj);
  }
  ByteArray result(&scope, runtime->newByteArray());
  runtime->byteArrayIadd(thread, result, result_bytes, result_bytes.length());
  return *result;
}

RawObject ByteArrayBuiltins::translate(Thread* thread, Frame* frame,
                                       word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfByteArray(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kByteArray);
  }
  ByteArray self(&scope, *self_obj);
  Bytes self_bytes(&scope, self.bytes());
  Object table_obj(&scope, args.get(1));
  word table_length;
  if (table_obj.isNoneType()) {
    table_length = BytesBuiltins::kTranslationTableLength;
    table_obj = Bytes::empty();
  } else if (runtime->isInstanceOfBytes(*table_obj)) {
    Bytes bytes(&scope, bytesUnderlying(thread, table_obj));
    table_length = bytes.length();
    table_obj = *bytes;
  } else if (runtime->isInstanceOfByteArray(*table_obj)) {
    ByteArray array(&scope, *table_obj);
    table_length = array.numItems();
    table_obj = array.bytes();
  } else {
    // TODO(T38246066): allow any bytes-like object
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "a bytes-like object is required, not '%T'",
                                &table_obj);
  }
  if (table_length != BytesBuiltins::kTranslationTableLength) {
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "translation table must be %w characters long",
                                BytesBuiltins::kTranslationTableLength);
  }
  Bytes table(&scope, *table_obj);
  Object del(&scope, args.get(2));
  Bytes translated(&scope, Bytes::empty());
  if (runtime->isInstanceOfBytes(*del)) {
    Bytes bytes(&scope, bytesUnderlying(thread, del));
    translated =
        runtime->bytesTranslate(thread, self_bytes, self.numItems(), table,
                                table_length, bytes, bytes.length());
  } else if (runtime->isInstanceOfByteArray(*del)) {
    ByteArray array(&scope, *del);
    Bytes bytes(&scope, array.bytes());
    translated =
        runtime->bytesTranslate(thread, self_bytes, self.numItems(), table,
                                table_length, bytes, array.numItems());
  } else {
    // TODO(T38246066): allow any bytes-like object
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "a bytes-like object is required, not '%T'",
                                &del);
  }
  ByteArray result(&scope, runtime->newByteArray());
  if (translated.isSmallBytes()) {
    runtime->byteArrayIadd(thread, result, translated, translated.length());
  } else {
    result.setBytes(*translated);
    result.setNumItems(translated.length());
  }
  return *result;
}

const BuiltinMethod ByteArrayIteratorBuiltins::kBuiltinMethods[] = {
    {SymbolId::kDunderIter, dunderIter},
    {SymbolId::kDunderLengthHint, dunderLengthHint},
    {SymbolId::kDunderNext, dunderNext},
    {SymbolId::kSentinelId, nullptr},
};

RawObject ByteArrayIteratorBuiltins::dunderIter(Thread* thread, Frame* frame,
                                                word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isByteArrayIterator()) {
    return thread->raiseRequiresType(self, SymbolId::kByteArrayIterator);
  }
  return *self;
}

RawObject ByteArrayIteratorBuiltins::dunderNext(Thread* thread, Frame* frame,
                                                word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  if (!self_obj.isByteArrayIterator()) {
    return thread->raiseRequiresType(self_obj, SymbolId::kByteArrayIterator);
  }
  ByteArrayIterator self(&scope, *self_obj);
  ByteArray bytearray(&scope, self.iterable());
  if (self.index() >= bytearray.numItems()) {
    return thread->raise(LayoutId::kStopIteration, NoneType::object());
  }
  Int item(&scope, thread->runtime()->newInt(bytearray.byteAt(self.index())));
  self.setIndex(self.index() + 1);
  return *item;
}

RawObject ByteArrayIteratorBuiltins::dunderLengthHint(Thread* thread,
                                                      Frame* frame,
                                                      word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  if (!self_obj.isByteArrayIterator()) {
    return thread->raiseRequiresType(self_obj, SymbolId::kByteArrayIterator);
  }
  ByteArrayIterator self(&scope, *self_obj);
  ByteArray bytearray(&scope, self.iterable());
  return SmallInt::fromWord(bytearray.numItems() - self.index());
}

}  // namespace python
