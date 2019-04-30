#include "bytes-builtins.h"

#include "bytearray-builtins.h"
#include "frame.h"
#include "int-builtins.h"
#include "runtime.h"
#include "slice-builtins.h"
#include "trampolines-inl.h"

namespace python {

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
  Object count_index(&scope, args.get(1));
  Object count_obj(&scope, intFromIndex(thread, count_index));
  if (count_obj.isError()) return *count_obj;
  Bytes self(&scope, *self_obj);
  Int count_int(&scope, intUnderlying(thread, count_obj));
  word count = count_int.asWordSaturated();
  if (!SmallInt::isValid(count)) {
    return thread->raiseOverflowError(runtime->newStrFromFmt(
        "cannot fit '%T' into an index-sized integer", &count_index));
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
    return thread->runtime()->bytesJoin(thread, self, self.length(), src,
                                        list.numItems());
  }
  if (iterable.isTuple()) {
    Tuple src(&scope, *iterable);
    return thread->runtime()->bytesJoin(thread, self, self.length(), src,
                                        src.length());
  }
  // Slow path: collect items into list in Python and call again
  return NoneType::object();
}

}  // namespace python
