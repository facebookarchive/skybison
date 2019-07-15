#include "bytes-builtins.h"

#include "bytearray-builtins.h"
#include "frame.h"
#include "int-builtins.h"
#include "runtime.h"
#include "slice-builtins.h"
#include "utils.h"

namespace python {

RawObject bytesFind(const Bytes& haystack, word haystack_len,
                    const Bytes& needle, word needle_len, word start,
                    word end) {
  DCHECK_BOUND(haystack_len, haystack.length());
  DCHECK_BOUND(needle_len, needle.length());
  Slice::adjustSearchIndices(&start, &end, haystack_len);
  for (word i = start; i <= end - needle_len; i++) {
    bool has_match = true;
    for (word j = 0; has_match && j < needle_len; j++) {
      has_match = haystack.byteAt(i + j) == needle.byteAt(j);
    }
    if (has_match) {
      return SmallInt::fromWord(i);
    }
  }
  return SmallInt::fromWord(-1);
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
  const byte bytes_delim[] = {'b', delimiter};
  runtime->byteArrayExtend(thread, buffer, bytes_delim);
  for (word i = 0; i < len; i++) {
    byte current = self.byteAt(i);
    if (current == delimiter || current == '\\') {
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

RawObject bytesUnderlying(Thread* thread, const Object& obj) {
  if (obj.isBytes()) return *obj;
  DCHECK(thread->runtime()->isInstanceOfBytes(*obj),
         "cannot get a base bytes value from a non-bytes");
  HandleScope scope(thread);
  UserBytesBase user_bytes(&scope, *obj);
  return user_bytes.value();
}

void SmallBytesBuiltins::postInitialize(Runtime* runtime,
                                        const Type& new_type) {
  new_type.setBuiltinBase(kSuperType);
  runtime->setSmallBytesType(new_type);
  Layout::cast(new_type.instanceLayout())
      .setDescribedType(runtime->typeAt(kSuperType));
}

void LargeBytesBuiltins::postInitialize(Runtime* runtime,
                                        const Type& new_type) {
  new_type.setBuiltinBase(kSuperType);
  runtime->setLargeBytesType(new_type);
  Layout::cast(new_type.instanceLayout())
      .setDescribedType(runtime->typeAt(kSuperType));
}

// Used only for UserBytesBase as a heap-allocated object.
const BuiltinAttribute BytesBuiltins::kAttributes[] = {
    {SymbolId::kInvalid, UserBytesBase::kValueOffset},
    {SymbolId::kSentinelId, 0},
};

// clang-format off
const BuiltinMethod BytesBuiltins::kBuiltinMethods[] = {
    {SymbolId::kDunderAdd, dunderAdd},
    {SymbolId::kDunderEq, dunderEq},
    {SymbolId::kDunderGe, dunderGe},
    {SymbolId::kDunderGt, dunderGt},
    {SymbolId::kDunderHash, dunderHash},
    {SymbolId::kDunderLe, dunderLe},
    {SymbolId::kDunderLen, dunderLen},
    {SymbolId::kDunderLt, dunderLt},
    {SymbolId::kDunderMul, dunderMul},
    {SymbolId::kDunderNe, dunderNe},
    {SymbolId::kDunderRepr, dunderRepr},
    {SymbolId::kHex, hex},
    {SymbolId::kTranslate, translate},
    {SymbolId::kSentinelId, nullptr},
};
// clang-format on

void BytesBuiltins::postInitialize(Runtime*, const Type& new_type) {
  new_type.setBuiltinBase(LayoutId::kBytes);
}

RawObject BytesBuiltins::dunderAdd(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfBytes(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kBytes);
  }
  Bytes self(&scope, bytesUnderlying(thread, self_obj));
  Object other_obj(&scope, args.get(1));
  if (runtime->isInstanceOfBytes(*other_obj)) {
    Bytes other(&scope, bytesUnderlying(thread, other_obj));
    return runtime->bytesConcat(thread, self, other);
  }
  if (runtime->isInstanceOfByteArray(*other_obj)) {
    ByteArray other(&scope, *other_obj);
    Bytes other_bytes(&scope, byteArrayAsBytes(thread, runtime, other));
    return runtime->bytesConcat(thread, self, other_bytes);
  }
  // TODO(T38246066): buffers besides bytes/bytearray
  return thread->raiseWithFmt(LayoutId::kTypeError, "can't concat %T to bytes",
                              &other_obj);
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
  Bytes self(&scope, bytesUnderlying(thread, self_obj));
  Bytes other(&scope, bytesUnderlying(thread, other_obj));
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
  Bytes self(&scope, bytesUnderlying(thread, self_obj));
  Bytes other(&scope, bytesUnderlying(thread, other_obj));
  return Bool::fromBool(self.compare(*other) >= 0);
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
  Bytes self(&scope, bytesUnderlying(thread, self_obj));
  Bytes other(&scope, bytesUnderlying(thread, other_obj));
  return Bool::fromBool(self.compare(*other) > 0);
}

RawObject BytesBuiltins::dunderHash(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfBytes(*self)) {
    return thread->raiseRequiresType(self, SymbolId::kBytes);
  }
  return runtime->hash(*self);
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
  Bytes self(&scope, bytesUnderlying(thread, self_obj));
  Bytes other(&scope, bytesUnderlying(thread, other_obj));
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

  Bytes self(&scope, bytesUnderlying(thread, self_obj));
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
  Bytes self(&scope, bytesUnderlying(thread, self_obj));
  Bytes other(&scope, bytesUnderlying(thread, other_obj));
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
  Object count_index(&scope, args.get(1));
  Object count_obj(&scope, intFromIndex(thread, count_index));
  if (count_obj.isError()) return *count_obj;
  Bytes self(&scope, bytesUnderlying(thread, self_obj));
  Int count_int(&scope, intUnderlying(thread, count_obj));
  word count = count_int.asWordSaturated();
  if (!SmallInt::isValid(count)) {
    return thread->raiseWithFmt(LayoutId::kOverflowError,
                                "cannot fit '%T' into an index-sized integer",
                                &count_index);
  }
  word length = self.length();
  if (count <= 0 || length == 0) {
    return Bytes::empty();
  }
  if (count == 1) {
    return *self;
  }
  word new_length;
  if (__builtin_mul_overflow(length, count, &new_length) ||
      !SmallInt::isValid(new_length)) {
    return thread->raiseWithFmt(LayoutId::kOverflowError,
                                "repeated bytes are too long");
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
  Bytes self(&scope, bytesUnderlying(thread, self_obj));
  Bytes other(&scope, bytesUnderlying(thread, other_obj));
  return Bool::fromBool(self.compare(*other) != 0);
}

RawObject BytesBuiltins::dunderRepr(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfBytes(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kBytes);
  }
  Bytes self(&scope, bytesUnderlying(thread, self_obj));
  return bytesReprSmartQuotes(thread, self);
}

RawObject BytesBuiltins::hex(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object obj(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfBytes(*obj)) {
    return thread->raiseRequiresType(obj, SymbolId::kBytes);
  }
  Bytes self(&scope, bytesUnderlying(thread, obj));
  return bytesHex(thread, self, self.length());
}

RawObject BytesBuiltins::translate(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfBytes(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kBytes);
  }
  Bytes self(&scope, bytesUnderlying(thread, self_obj));
  Object table_obj(&scope, args.get(1));
  word table_length;
  if (table_obj.isNoneType()) {
    table_length = kTranslationTableLength;
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
  if (table_length != kTranslationTableLength) {
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "translation table must be %w characters long",
                                kTranslationTableLength);
  }
  Bytes table(&scope, *table_obj);
  Object del(&scope, args.get(2));
  if (runtime->isInstanceOfBytes(*del)) {
    Bytes bytes(&scope, bytesUnderlying(thread, del));
    return runtime->bytesTranslate(thread, self, self.length(), table, bytes,
                                   bytes.length());
  }
  if (runtime->isInstanceOfByteArray(*del)) {
    ByteArray array(&scope, *del);
    Bytes bytes(&scope, array.bytes());
    return runtime->bytesTranslate(thread, self, self.length(), table, bytes,
                                   array.numItems());
  }
  // TODO(T38246066): allow any bytes-like object
  return thread->raiseWithFmt(
      LayoutId::kTypeError, "a bytes-like object is required, not '%T'", &del);
}

}  // namespace python
