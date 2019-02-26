#include "bytearray-builtins.h"

#include "bytes-builtins.h"
#include "runtime.h"

namespace python {

RawObject byteArrayAsBytes(Thread* thread, Runtime* runtime,
                           const ByteArray& array) {
  HandleScope scope(thread);
  Bytes bytes(&scope, array.bytes());
  return runtime->bytesSubseq(thread, bytes, 0, array.numItems());
}

void writeByteAsHexDigits(Thread* thread, const ByteArray& array, byte value) {
  const byte* hex_digits = reinterpret_cast<const byte*>("0123456789abcdef");
  thread->runtime()->byteArrayExtend(
      thread, array, {hex_digits[value >> 4], hex_digits[value & 0xf]});
}

const BuiltinAttribute ByteArrayBuiltins::kAttributes[] = {
    {SymbolId::kItems, ByteArray::kBytesOffset},
    {SymbolId::kAllocated, ByteArray::kNumItemsOffset},
    {SymbolId::kSentinelId, -1},
};

// clang-format off
const BuiltinMethod ByteArrayBuiltins::kBuiltinMethods[] = {
    {SymbolId::kDunderAdd, dunderAdd},
    {SymbolId::kDunderIadd, dunderIadd},
    {SymbolId::kDunderInit, dunderInit},
    {SymbolId::kDunderNew, dunderNew},
    {SymbolId::kDunderRepr, dunderRepr},
    {SymbolId::kSentinelId, nullptr},
};
// clang-format on

RawObject ByteArrayBuiltins::dunderAdd(Thread* thread, Frame* frame,
                                       word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfByteArray(*self_obj)) {
    return thread->raiseTypeErrorWithCStr(
        "'__add__' requires a 'bytearray' object");
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

RawObject ByteArrayBuiltins::dunderIadd(Thread* thread, Frame* frame,
                                        word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfByteArray(*self_obj)) {
    return thread->raiseTypeErrorWithCStr(
        "'__iadd__' requires a 'bytearray' object");
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

RawObject ByteArrayBuiltins::dunderInit(Thread* thread, Frame* frame,
                                        word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfByteArray(*self_obj)) {
    return thread->raiseTypeErrorWithCStr(
        "'__init__' requires a 'bytearray' object");
  }
  ByteArray self(&scope, *self_obj);
  // always clear the existing contents of the array
  if (self.numItems() > 0) {
    self.downsize(0);
  }
  Object source(&scope, args.get(1));
  Object encoding(&scope, args.get(2));
  Object errors(&scope, args.get(3));
  if (source.isUnboundValue()) {
    if (encoding.isUnboundValue() && errors.isUnboundValue()) {
      return NoneType::object();
    }
    return thread->raiseTypeErrorWithCStr(
        "encoding or errors without sequence argument");
  }
  if (runtime->isInstanceOfStr(*source)) {
    if (encoding.isUnboundValue()) {
      return thread->raiseTypeErrorWithCStr(
          "string argument without an encoding");
    }
    UNIMPLEMENTED("string encoding");  // return NoneType::value();
  }
  if (!encoding.isUnboundValue() || !errors.isUnboundValue()) {
    return thread->raiseTypeErrorWithCStr(
        "encoding or errors without a string argument");
  }
  if (runtime->isInstanceOfInt(*source)) {
    // TODO(T38780562): strict subclass of int
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
    Object maybe_bytes(&scope, bytesFromIterable(thread, source));
    if (maybe_bytes.isError()) return *maybe_bytes;
    Bytes bytes(&scope, *maybe_bytes);
    runtime->byteArrayIadd(thread, self, bytes, bytes.length());
  }
  return NoneType::object();
}

RawObject ByteArrayBuiltins::dunderNew(Thread* thread, Frame* frame,
                                       word nargs) {
  Arguments args(frame, nargs);
  if (!args.get(0)->isType()) {
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
    return thread->raiseTypeErrorWithCStr(
        "'__repr__' requires a 'bytearray' object");
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
  runtime->byteArrayExtend(
      thread, buffer,
      {'b', 'y', 't', 'e', 'a', 'r', 'r', 'a', 'y', '(', 'b', quote});
  for (word i = 0; i < length; i++) {
    byte current = self.byteAt(i);
    if (current == quote || current == '\\') {
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
  runtime->byteArrayExtend(thread, buffer, {quote, ')'});
  return runtime->newStrFromByteArray(buffer);
}

}  // namespace python
