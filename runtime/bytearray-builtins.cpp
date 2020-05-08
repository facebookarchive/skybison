#include "bytearray-builtins.h"

#include "builtins.h"
#include "bytes-builtins.h"
#include "int-builtins.h"
#include "runtime.h"
#include "slice-builtins.h"
#include "type-builtins.h"
#include "unicode.h"

namespace py {

RawObject bytearrayAsBytes(Thread* thread, const Bytearray& array) {
  HandleScope scope(thread);
  Bytes bytes(&scope, array.items());
  return bytesSubseq(thread, bytes, 0, array.numItems());
}

void writeByteAsHexDigits(Thread* thread, const Bytearray& array, byte value) {
  const byte* hex_digits = reinterpret_cast<const byte*>("0123456789abcdef");
  const byte bytes[] = {hex_digits[value >> 4], hex_digits[value & 0xf]};
  thread->runtime()->bytearrayExtend(thread, array, bytes);
}

static const BuiltinAttribute kBytearrayAttributes[] = {
    {ID(_bytearray__bytes), RawBytearray::kItemsOffset,
     AttributeFlags::kHidden},
    {ID(_bytearray__num_items), RawBytearray::kNumItemsOffset,
     AttributeFlags::kHidden},
};

static const BuiltinAttribute kBytearrayIteratorAttributes[] = {
    {ID(_bytearray_iterator__iterable), RawBytearrayIterator::kIterableOffset,
     AttributeFlags::kHidden},
    {ID(_bytearray_iterator__index), RawBytearrayIterator::kIndexOffset,
     AttributeFlags::kHidden},
};

void initializeBytearrayTypes(Thread* thread) {
  addBuiltinType(thread, ID(bytearray), LayoutId::kBytearray,
                 /*superclass_id=*/LayoutId::kObject, kBytearrayAttributes);

  addBuiltinType(thread, ID(bytearray_iterator), LayoutId::kBytearrayIterator,
                 /*superclass_id=*/LayoutId::kObject,
                 kBytearrayIteratorAttributes);
}

RawObject METH(bytearray, __add__)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfBytearray(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(bytearray));
  }
  Object other_obj(&scope, args.get(1));
  word other_len;
  if (runtime->isInstanceOfBytearray(*other_obj)) {
    Bytearray array(&scope, *other_obj);
    other_len = array.numItems();
    other_obj = array.items();
  } else if (runtime->isInstanceOfBytes(*other_obj)) {
    Bytes bytes(&scope, bytesUnderlying(*other_obj));
    other_len = bytes.length();
    other_obj = *bytes;
  } else {
    return thread->raiseWithFmt(
        LayoutId::kTypeError,
        "can only concatenate bytearray or bytes to bytearray");
  }

  Bytearray self(&scope, *self_obj);
  Bytes self_bytes(&scope, self.items());
  word self_len = self.numItems();
  Bytes other_bytes(&scope, *other_obj);

  Bytearray result(&scope, runtime->newBytearray());
  runtime->bytearrayEnsureCapacity(thread, result, self_len + other_len);
  runtime->bytearrayIadd(thread, result, self_bytes, self_len);
  runtime->bytearrayIadd(thread, result, other_bytes, other_len);
  return *result;
}

RawObject METH(bytearray, __eq__)(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfBytearray(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(bytearray));
  }
  Bytearray self(&scope, *self_obj);
  Object other_obj(&scope, args.get(1));
  word comparison;
  if (runtime->isInstanceOfBytes(*other_obj)) {
    Bytes other(&scope, bytesUnderlying(*other_obj));
    comparison = self.compare(*other, other.length());
  } else if (runtime->isInstanceOfBytearray(*other_obj)) {
    Bytearray other(&scope, *other_obj);
    Bytes other_bytes(&scope, other.items());
    comparison = self.compare(*other_bytes, other.numItems());
  } else {
    // TODO(T38246066): allow any bytes-like object
    return NotImplementedType::object();
  }
  return Bool::fromBool(comparison == 0);
}

RawObject METH(bytearray, __ge__)(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfBytearray(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(bytearray));
  }
  Bytearray self(&scope, *self_obj);
  Object other_obj(&scope, args.get(1));
  word comparison;
  if (runtime->isInstanceOfBytes(*other_obj)) {
    Bytes other(&scope, bytesUnderlying(*other_obj));
    comparison = self.compare(*other, other.length());
  } else if (runtime->isInstanceOfBytearray(*other_obj)) {
    Bytearray other(&scope, *other_obj);
    Bytes other_bytes(&scope, other.items());
    comparison = self.compare(*other_bytes, other.numItems());
  } else {
    // TODO(T38246066): allow any bytes-like object
    return NotImplementedType::object();
  }
  return Bool::fromBool(comparison >= 0);
}

RawObject METH(bytearray, __gt__)(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfBytearray(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(bytearray));
  }
  Bytearray self(&scope, *self_obj);
  Object other_obj(&scope, args.get(1));
  word comparison;
  if (runtime->isInstanceOfBytes(*other_obj)) {
    Bytes other(&scope, bytesUnderlying(*other_obj));
    comparison = self.compare(*other, other.length());
  } else if (runtime->isInstanceOfBytearray(*other_obj)) {
    Bytearray other(&scope, *other_obj);
    Bytes other_bytes(&scope, other.items());
    comparison = self.compare(*other_bytes, other.numItems());
  } else {
    // TODO(T38246066): allow any bytes-like object
    return NotImplementedType::object();
  }
  return Bool::fromBool(comparison > 0);
}

RawObject METH(bytearray, __iadd__)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfBytearray(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(bytearray));
  }
  Bytearray self(&scope, *self_obj);
  Object other_obj(&scope, args.get(1));
  word other_len;
  if (runtime->isInstanceOfBytearray(*other_obj)) {
    Bytearray array(&scope, *other_obj);
    other_len = array.numItems();
    other_obj = array.items();
  } else if (runtime->isInstanceOfBytes(*other_obj)) {
    Bytes bytes(&scope, bytesUnderlying(*other_obj));
    other_len = bytes.length();
    other_obj = *bytes;
  } else {
    return thread->raiseWithFmt(
        LayoutId::kTypeError,
        "can only concatenate bytearray or bytes to bytearray");
  }
  Bytes other(&scope, *other_obj);
  runtime->bytearrayIadd(thread, self, other, other_len);
  return *self;
}

RawObject METH(bytearray, __imul__)(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfBytearray(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(bytearray));
  }
  Bytearray self(&scope, *self_obj);
  Object count_index(&scope, args.get(1));
  Object count_obj(&scope, intFromIndex(thread, count_index));
  if (count_obj.isError()) return *count_obj;
  word count = intUnderlying(*count_obj).asWordSaturated();
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
  Bytes source(&scope, self.items());
  if (new_length <= self.capacity()) {
    // fits into existing backing LargeBytes - repeat in place
    for (word i = 1; i < count; i++) {
      runtime->bytearrayIadd(thread, self, source, length);
    }
    return *self;
  }
  // grows beyond existing bytes - allocate new
  self.setItems(runtime->bytesRepeat(thread, source, length, count));
  DCHECK(self.capacity() == new_length, "unexpected result length");
  self.setNumItems(new_length);
  return *self;
}

RawObject METH(bytearray, __iter__)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfBytearray(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(bytearray));
  }
  Bytearray self(&scope, *self_obj);
  return runtime->newBytearrayIterator(thread, self);
}

RawObject METH(bytearray, __le__)(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfBytearray(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(bytearray));
  }
  Bytearray self(&scope, *self_obj);
  Object other_obj(&scope, args.get(1));
  word comparison;
  if (runtime->isInstanceOfBytes(*other_obj)) {
    Bytes other(&scope, bytesUnderlying(*other_obj));
    comparison = self.compare(*other, other.length());
  } else if (runtime->isInstanceOfBytearray(*other_obj)) {
    Bytearray other(&scope, *other_obj);
    Bytes other_bytes(&scope, other.items());
    comparison = self.compare(*other_bytes, other.numItems());
  } else {
    // TODO(T38246066): allow any bytes-like object
    return NotImplementedType::object();
  }
  return Bool::fromBool(comparison <= 0);
}

RawObject METH(bytearray, __len__)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfBytearray(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(bytearray));
  }
  Bytearray self(&scope, *self_obj);
  return SmallInt::fromWord(self.numItems());
}

RawObject METH(bytearray, __lt__)(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfBytearray(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(bytearray));
  }
  Bytearray self(&scope, *self_obj);
  Object other_obj(&scope, args.get(1));
  word comparison;
  if (runtime->isInstanceOfBytes(*other_obj)) {
    Bytes other(&scope, bytesUnderlying(*other_obj));
    comparison = self.compare(*other, other.length());
  } else if (runtime->isInstanceOfBytearray(*other_obj)) {
    Bytearray other(&scope, *other_obj);
    Bytes other_bytes(&scope, other.items());
    comparison = self.compare(*other_bytes, other.numItems());
  } else {
    // TODO(T38246066): allow any bytes-like object
    return NotImplementedType::object();
  }
  return Bool::fromBool(comparison < 0);
}

RawObject METH(bytearray, __mul__)(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfBytearray(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(bytearray));
  }
  Bytearray self(&scope, *self_obj);
  Object count_index(&scope, args.get(1));
  Object count_obj(&scope, intFromIndex(thread, count_index));
  if (count_obj.isError()) return *count_obj;
  word count = intUnderlying(*count_obj).asWordSaturated();
  if (!SmallInt::isValid(count)) {
    return thread->raiseWithFmt(LayoutId::kOverflowError,
                                "cannot fit '%T' into an index-sized integer",
                                &count_index);
  }
  word length = self.numItems();
  if (count <= 0 || length == 0) {
    return runtime->newBytearray();
  }
  word new_length;
  if (__builtin_mul_overflow(length, count, &new_length) ||
      !SmallInt::isValid(new_length)) {
    return thread->raiseMemoryError();
  }
  Bytes source(&scope, self.items());
  Bytearray result(&scope, runtime->newBytearray());
  Bytes repeated(&scope, runtime->bytesRepeat(thread, source, length, count));
  DCHECK(repeated.length() == new_length, "unexpected result length");
  if (repeated.isSmallBytes()) {
    runtime->bytearrayIadd(thread, result, repeated, new_length);
  } else {
    result.setItems(*repeated);
    result.setNumItems(new_length);
  }
  return *result;
}

RawObject METH(bytearray, __ne__)(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfBytearray(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(bytearray));
  }
  Bytearray self(&scope, *self_obj);
  Object other_obj(&scope, args.get(1));
  word comparison;
  if (runtime->isInstanceOfBytes(*other_obj)) {
    Bytes other(&scope, bytesUnderlying(*other_obj));
    comparison = self.compare(*other, other.length());
  } else if (runtime->isInstanceOfBytearray(*other_obj)) {
    Bytearray other(&scope, *other_obj);
    Bytes other_bytes(&scope, other.items());
    comparison = self.compare(*other_bytes, other.numItems());
  } else {
    // TODO(T38246066): allow any bytes-like object
    return NotImplementedType::object();
  }
  return Bool::fromBool(comparison != 0);
}

RawObject METH(bytearray, __new__)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object type_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfType(*type_obj)) {
    return thread->raiseWithFmt(LayoutId::kTypeError, "not a type object");
  }
  Type type(&scope, *type_obj);
  if (type.builtinBase() != LayoutId::kBytearray) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "not a subtype of bytearray");
  }
  Layout layout(&scope, type.instanceLayout());
  Bytearray result(&scope, runtime->newInstance(layout));
  result.setItems(runtime->emptyMutableBytes());
  result.setNumItems(0);
  return *result;
}

RawObject bytearrayRepr(Thread* thread, const Bytearray& array) {
  word length = array.numItems();
  word affix_length = 14;  // strlen("bytearray(b'')") == 14
  if (length > (kMaxWord - affix_length) / 4) {
    return thread->raiseWithFmt(LayoutId::kOverflowError,
                                "bytearray object is too large to make repr");
  }

  // Figure out which quote to use; single is preferred
  bool has_single_quote = false;
  bool has_double_quote = false;
  for (word i = 0; i < length; i++) {
    byte current = array.byteAt(i);
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
  Bytearray buffer(&scope, runtime->newBytearray());
  // Each byte will be mapped to one or more ASCII characters.
  runtime->bytearrayEnsureCapacity(thread, buffer, length + affix_length);
  const byte bytearray_str[] = {'b', 'y', 't', 'e', 'a', 'r',
                                'r', 'a', 'y', '(', 'b', quote};
  runtime->bytearrayExtend(thread, buffer, bytearray_str);
  for (word i = 0; i < length; i++) {
    byte current = array.byteAt(i);
    if (current == '\'' || current == '\\') {
      const byte bytes[] = {'\\', current};
      runtime->bytearrayExtend(thread, buffer, bytes);
    } else if (current == '\t') {
      const byte bytes[] = {'\\', 't'};
      runtime->bytearrayExtend(thread, buffer, bytes);
    } else if (current == '\n') {
      const byte bytes[] = {'\\', 'n'};
      runtime->bytearrayExtend(thread, buffer, bytes);
    } else if (current == '\r') {
      const byte bytes[] = {'\\', 'r'};
      runtime->bytearrayExtend(thread, buffer, bytes);
    } else if (current < ' ' || current >= 0x7f) {
      const byte bytes[] = {'\\', 'x'};
      runtime->bytearrayExtend(thread, buffer, bytes);
      writeByteAsHexDigits(thread, buffer, current);
    } else {
      bytearrayAdd(thread, runtime, buffer, current);
    }
  }

  const byte quote_with_paren[] = {quote, ')'};
  runtime->bytearrayExtend(thread, buffer, quote_with_paren);
  return runtime->newStrFromBytearray(buffer);
}

RawObject METH(bytearray, __repr__)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfBytearray(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(bytearray));
  }
  Bytearray self(&scope, *self_obj);
  return bytearrayRepr(thread, self);
}

RawObject METH(bytearray, hex)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object obj(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfBytearray(*obj)) {
    return thread->raiseRequiresType(obj, ID(bytearray));
  }
  Bytearray self(&scope, *obj);
  Bytes bytes(&scope, self.items());
  return bytesHex(thread, bytes, self.numItems());
}

RawObject METH(bytearray, lower)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfBytearray(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(bytearray));
  }
  Bytearray self(&scope, *self_obj);
  Bytes items(&scope, self.items());
  word num_items = self.numItems();
  MutableBytes lowered(&scope,
                       runtime->newMutableBytesUninitialized(items.length()));
  for (word i = 0; i < num_items; i++) {
    lowered.byteAtPut(i, ASCII::toLower(items.byteAt(i)));
  }
  Bytearray result(&scope, runtime->newBytearray());
  result.setItems(*lowered);
  result.setNumItems(num_items);
  return *result;
}

RawObject METH(bytearray, lstrip)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfBytearray(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(bytearray));
  }
  Bytearray self(&scope, *self_obj);
  Bytes self_bytes(&scope, self.items());
  Object chars_obj(&scope, args.get(1));
  Bytes result_bytes(&scope, Bytes::empty());
  if (chars_obj.isNoneType()) {
    result_bytes = bytesStripSpaceLeft(thread, self_bytes, self.numItems());
  } else if (runtime->isInstanceOfBytes(*chars_obj)) {
    Bytes chars(&scope, bytesUnderlying(*chars_obj));
    result_bytes = bytesStripLeft(thread, self_bytes, self.numItems(), chars,
                                  chars.length());
  } else if (runtime->isInstanceOfBytearray(*chars_obj)) {
    Bytearray chars(&scope, *chars_obj);
    Bytes chars_bytes(&scope, chars.items());
    result_bytes = bytesStripLeft(thread, self_bytes, self.numItems(),
                                  chars_bytes, chars.numItems());
  } else {
    // TODO(T38246066): support bytes-like objects other than bytes, bytearray
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "a bytes-like object is required, not '%T'",
                                &chars_obj);
  }
  Bytearray result(&scope, runtime->newBytearray());
  runtime->bytearrayIadd(thread, result, result_bytes, result_bytes.length());
  return *result;
}

RawObject METH(bytearray, rstrip)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfBytearray(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(bytearray));
  }
  Bytearray self(&scope, *self_obj);
  Bytes self_bytes(&scope, self.items());
  Object chars_obj(&scope, args.get(1));
  Bytes result_bytes(&scope, Bytes::empty());
  if (chars_obj.isNoneType()) {
    result_bytes = bytesStripSpaceRight(thread, self_bytes, self.numItems());
  } else if (runtime->isInstanceOfBytes(*chars_obj)) {
    Bytes chars(&scope, bytesUnderlying(*chars_obj));
    result_bytes = bytesStripRight(thread, self_bytes, self.numItems(), chars,
                                   chars.length());
  } else if (runtime->isInstanceOfBytearray(*chars_obj)) {
    Bytearray chars(&scope, *chars_obj);
    Bytes chars_bytes(&scope, chars.items());
    result_bytes = bytesStripRight(thread, self_bytes, self.numItems(),
                                   chars_bytes, chars.numItems());
  } else {
    // TODO(T38246066): support bytes-like objects other than bytes, bytearray
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "a bytes-like object is required, not '%T'",
                                &chars_obj);
  }
  Bytearray result(&scope, runtime->newBytearray());
  runtime->bytearrayIadd(thread, result, result_bytes, result_bytes.length());
  return *result;
}

RawObject METH(bytearray, strip)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfBytearray(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(bytearray));
  }
  Bytearray self(&scope, *self_obj);
  Bytes self_bytes(&scope, self.items());
  Object chars_obj(&scope, args.get(1));
  Bytes result_bytes(&scope, Bytes::empty());
  if (chars_obj.isNoneType()) {
    result_bytes = bytesStripSpace(thread, self_bytes, self.numItems());
  } else if (runtime->isInstanceOfBytes(*chars_obj)) {
    Bytes chars(&scope, bytesUnderlying(*chars_obj));
    result_bytes =
        bytesStrip(thread, self_bytes, self.numItems(), chars, chars.length());
  } else if (runtime->isInstanceOfBytearray(*chars_obj)) {
    Bytearray chars(&scope, *chars_obj);
    Bytes chars_bytes(&scope, chars.items());
    result_bytes = bytesStrip(thread, self_bytes, self.numItems(), chars_bytes,
                              chars.numItems());
  } else {
    // TODO(T38246066): support bytes-like objects other than bytes, bytearray
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "a bytes-like object is required, not '%T'",
                                &chars_obj);
  }
  Bytearray result(&scope, runtime->newBytearray());
  runtime->bytearrayIadd(thread, result, result_bytes, result_bytes.length());
  return *result;
}

RawObject METH(bytearray, translate)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfBytearray(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(bytearray));
  }
  Bytearray self(&scope, *self_obj);
  Bytes self_bytes(&scope, self.items());
  Object table_obj(&scope, args.get(1));
  word table_length;
  if (table_obj.isNoneType()) {
    table_length = kByteTranslationTableLength;
    table_obj = Bytes::empty();
  } else if (runtime->isInstanceOfBytes(*table_obj)) {
    Bytes bytes(&scope, bytesUnderlying(*table_obj));
    table_length = bytes.length();
    table_obj = *bytes;
  } else if (runtime->isInstanceOfBytearray(*table_obj)) {
    Bytearray array(&scope, *table_obj);
    table_length = array.numItems();
    table_obj = array.items();
  } else {
    // TODO(T38246066): allow any bytes-like object
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "a bytes-like object is required, not '%T'",
                                &table_obj);
  }
  if (table_length != kByteTranslationTableLength) {
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "translation table must be %w characters long",
                                kByteTranslationTableLength);
  }
  Bytes table(&scope, *table_obj);
  Object del(&scope, args.get(2));
  Bytes translated(&scope, Bytes::empty());
  if (runtime->isInstanceOfBytes(*del)) {
    Bytes bytes(&scope, bytesUnderlying(*del));
    translated =
        runtime->bytesTranslate(thread, self_bytes, self.numItems(), table,
                                table_length, bytes, bytes.length());
  } else if (runtime->isInstanceOfBytearray(*del)) {
    Bytearray array(&scope, *del);
    Bytes bytes(&scope, array.items());
    translated =
        runtime->bytesTranslate(thread, self_bytes, self.numItems(), table,
                                table_length, bytes, array.numItems());
  } else {
    // TODO(T38246066): allow any bytes-like object
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "a bytes-like object is required, not '%T'",
                                &del);
  }
  Bytearray result(&scope, runtime->newBytearray());
  if (translated.isSmallBytes()) {
    runtime->bytearrayIadd(thread, result, translated, translated.length());
  } else {
    result.setItems(*translated);
    result.setNumItems(translated.length());
  }
  return *result;
}

RawObject METH(bytearray, upper)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfBytearray(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(bytearray));
  }
  Bytearray self(&scope, *self_obj);
  Bytes items(&scope, self.items());
  word num_items = self.numItems();
  MutableBytes uppered(&scope,
                       runtime->newMutableBytesUninitialized(items.length()));
  for (word i = 0; i < num_items; i++) {
    uppered.byteAtPut(i, ASCII::toUpper(items.byteAt(i)));
  }
  Bytearray result(&scope, runtime->newBytearray());
  result.setItems(*uppered);
  result.setNumItems(num_items);
  return *result;
}

RawObject METH(bytearray_iterator, __iter__)(Thread* thread, Frame* frame,
                                             word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isBytearrayIterator()) {
    return thread->raiseRequiresType(self, ID(bytearray_iterator));
  }
  return *self;
}

RawObject METH(bytearray_iterator, __next__)(Thread* thread, Frame* frame,
                                             word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  if (!self_obj.isBytearrayIterator()) {
    return thread->raiseRequiresType(self_obj, ID(bytearray_iterator));
  }
  BytearrayIterator self(&scope, *self_obj);
  Bytearray bytearray(&scope, self.iterable());
  if (self.index() >= bytearray.numItems()) {
    return thread->raise(LayoutId::kStopIteration, NoneType::object());
  }
  Int item(&scope, thread->runtime()->newInt(bytearray.byteAt(self.index())));
  self.setIndex(self.index() + 1);
  return *item;
}

RawObject METH(bytearray_iterator, __length_hint__)(Thread* thread,
                                                    Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  if (!self_obj.isBytearrayIterator()) {
    return thread->raiseRequiresType(self_obj, ID(bytearray_iterator));
  }
  BytearrayIterator self(&scope, *self_obj);
  Bytearray bytearray(&scope, self.iterable());
  return SmallInt::fromWord(bytearray.numItems() - self.index());
}

}  // namespace py
