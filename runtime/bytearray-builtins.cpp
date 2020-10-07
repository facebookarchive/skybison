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
                 /*superclass_id=*/LayoutId::kObject, kBytearrayAttributes,
                 Bytearray::kSize, /*basetype=*/true);

  addBuiltinType(thread, ID(bytearray_iterator), LayoutId::kBytearrayIterator,
                 /*superclass_id=*/LayoutId::kObject,
                 kBytearrayIteratorAttributes, BytearrayIterator::kSize,
                 /*basetype=*/false);
}

RawObject METH(bytearray, __add__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
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

RawObject METH(bytearray, __eq__)(Thread* thread, Arguments args) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
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

RawObject METH(bytearray, __ge__)(Thread* thread, Arguments args) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
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

RawObject METH(bytearray, __gt__)(Thread* thread, Arguments args) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
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

RawObject METH(bytearray, __iadd__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
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

RawObject METH(bytearray, __imul__)(Thread* thread, Arguments args) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
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

RawObject METH(bytearray, __iter__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfBytearray(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(bytearray));
  }
  Bytearray self(&scope, *self_obj);
  return runtime->newBytearrayIterator(thread, self);
}

RawObject METH(bytearray, __le__)(Thread* thread, Arguments args) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
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

RawObject METH(bytearray, __len__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfBytearray(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(bytearray));
  }
  Bytearray self(&scope, *self_obj);
  return SmallInt::fromWord(self.numItems());
}

RawObject METH(bytearray, __lt__)(Thread* thread, Arguments args) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
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

RawObject METH(bytearray, __mul__)(Thread* thread, Arguments args) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
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

RawObject METH(bytearray, __ne__)(Thread* thread, Arguments args) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
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

RawObject METH(bytearray, __new__)(Thread* thread, Arguments args) {
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
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Type cls(&scope, runtime->typeOf(*array));
  Str name(&scope, cls.name());
  word name_length = name.length();
  word length = array.numItems();
  if (length > (kMaxWord - 6 - name_length) / 4) {
    return thread->raiseWithFmt(LayoutId::kOverflowError,
                                "bytearray object is too large to make repr");
  }

  // Precalculate length and determine which quote to use; single is preferred
  word result_length = name_length + length + 5;  // <cls>(b'<contents>')
  bool has_single_quote = false;
  bool has_double_quote = false;
  for (word i = 0; i < length; i++) {
    byte current = array.byteAt(i);
    switch (current) {
      case '\'':
        result_length++;
        has_single_quote = true;
        break;
      case '"':
        has_double_quote = true;
        break;
      case '\t':
      case '\n':
      case '\r':
      case '\\':
        result_length++;
        break;
      default:
        if (!ASCII::isPrintable(current)) {
          result_length += 3;
        }
    }
  }
  byte delimiter = (has_single_quote && !has_double_quote) ? '"' : '\'';

  MutableBytes result(&scope,
                      runtime->newMutableBytesUninitialized(result_length));
  word j = 0;
  result.replaceFromWithStr(0, *name, name_length);
  j += name_length;
  result.byteAtPut(j++, '(');
  result.byteAtPut(j++, 'b');
  result.byteAtPut(j++, delimiter);

  for (word i = 0; i < length; i++) {
    byte current = array.byteAt(i);
    switch (current) {
      case '\'':
        result.byteAtPut(j++, '\\');
        result.byteAtPut(j++, current);
        break;
      case '\t':
        result.byteAtPut(j++, '\\');
        result.byteAtPut(j++, 't');
        break;
      case '\n':
        result.byteAtPut(j++, '\\');
        result.byteAtPut(j++, 'n');
        break;
      case '\r':
        result.byteAtPut(j++, '\\');
        result.byteAtPut(j++, 'r');
        break;
      case '\\':
        result.byteAtPut(j++, '\\');
        result.byteAtPut(j++, '\\');
        break;
      default:
        if (ASCII::isPrintable(current)) {
          result.byteAtPut(j++, current);
        } else {
          result.byteAtPut(j++, '\\');
          result.byteAtPut(j++, 'x');
          result.putHex(j, current);
          j += 2;
        }
    }
  }

  result.byteAtPut(j++, delimiter);
  result.byteAtPut(j++, ')');
  DCHECK(j == result_length, "expected %ld bytes, wrote %ld", result_length, j);
  return result.becomeStr();
}

RawObject METH(bytearray, __repr__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfBytearray(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(bytearray));
  }
  Bytearray self(&scope, *self_obj);
  return bytearrayRepr(thread, self);
}

RawObject METH(bytearray, hex)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object obj(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfBytearray(*obj)) {
    return thread->raiseRequiresType(obj, ID(bytearray));
  }
  Bytearray self(&scope, *obj);
  Bytes bytes(&scope, self.items());
  return bytesHex(thread, bytes, self.numItems());
}

RawObject METH(bytearray, lower)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
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

RawObject METH(bytearray, lstrip)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
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

RawObject METH(bytearray, rstrip)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
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

RawObject METH(bytearray, strip)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
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

RawObject METH(bytearray, translate)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
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

RawObject METH(bytearray, upper)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
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

static RawObject bytearraySplitLines(Thread* thread, const Bytearray& bytearray,
                                     bool keepends) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  List result(&scope, runtime->newList());
  word length = bytearray.numItems();
  Bytearray line(&scope, *bytearray);

  for (word i = 0, j = 0; i < length; j = i) {
    // Skip newline bytes
    for (; i < length; i++) {
      byte b = bytearray.byteAt(i);
      if (b == '\n' || b == '\r') {
        break;
      }
    }

    word eol_pos = i;
    if (i < length) {
      word cur = i;
      word next = i + 1;
      i++;
      // Check for \r\n specifically
      if (bytearray.byteAt(cur) == '\r' && next < length &&
          bytearray.byteAt(next) == '\n') {
        i++;
      }
      if (keepends) {
        eol_pos = i;
      }
    }

    line = runtime->newBytearray();
    word line_length = eol_pos - j;
    runtime->bytearrayEnsureCapacity(thread, line, line_length);
    line.setNumItems(line_length);
    line.replaceFromWithStartAt(0, *bytearray, line_length, j);

    runtime->listAdd(thread, result, line);
  }

  return *result;
}

RawObject METH(bytearray, splitlines)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object self_obj(&scope, args.get(0));
  Object keepends_obj(&scope, args.get(1));
  if (!runtime->isInstanceOfBytearray(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(bytearray));
  }
  if (!runtime->isInstanceOfInt(*keepends_obj)) {
    return thread->raiseRequiresType(keepends_obj, ID(int));
  }
  Bytearray self(&scope, *self_obj);
  bool keepends = !intUnderlying(*keepends_obj).isZero();
  return bytearraySplitLines(thread, self, keepends);
}

RawObject METH(bytearray_iterator, __iter__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isBytearrayIterator()) {
    return thread->raiseRequiresType(self, ID(bytearray_iterator));
  }
  return *self;
}

RawObject METH(bytearray_iterator, __next__)(Thread* thread, Arguments args) {
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
                                                    Arguments args) {
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
