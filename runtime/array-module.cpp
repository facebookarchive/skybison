#include "array-module.h"

#include "builtins.h"
#include "handles.h"
#include "modules.h"
#include "objects.h"
#include "runtime.h"
#include "symbols.h"
#include "thread.h"
#include "type-builtins.h"

namespace py {

RawObject FUNC(array, _array_check)(Thread* thread, Arguments args) {
  return Bool::fromBool(thread->runtime()->isInstanceOfArray(args.get(0)));
}

static word itemSize(byte typecode) {
  switch (typecode) {
    case 'b':
    case 'B':
      return kByteSize;
    case 'u':
      return kWcharSize;
    case 'h':
    case 'H':
      return kShortSize;
    case 'i':
    case 'I':
      return kIntSize;
    case 'l':
    case 'L':
      return kLongSize;
    case 'q':
    case 'Q':
      return kLongLongSize;
    case 'f':
      return kFloatSize;
    case 'd':
      return kDoubleSize;
    default:
      return -1;
  }
}

RawObject FUNC(array, _array_new)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Str typecode_str(&scope, strUnderlying(args.get(1)));
  DCHECK(typecode_str.length() == 1, "typecode must be a single-char str");
  byte typecode = typecode_str.byteAt(0);
  word item_size = itemSize(typecode);
  if (item_size == -1) {
    return thread->raiseWithFmt(
        LayoutId::kValueError,
        "bad typecode (must be b, B, u, h, H, i, I, l, L, q, Q, f or d)");
  }
  word len = SmallInt::cast(args.get(2)).value() * item_size;
  Runtime* runtime = thread->runtime();

  Type array_type(&scope, args.get(0));
  Layout layout(&scope, array_type.instanceLayout());
  Array result(&scope, runtime->newInstance(layout));
  result.setTypecode(*typecode_str);
  result.setLength(0);
  result.setBuffer(runtime->mutableBytesWith(len, 0));
  return *result;
}

static bool isIntTypecode(char typecode) {
  switch (typecode) {
    case 'f':
      FALLTHROUGH;
    case 'd':
      FALLTHROUGH;
    case 'u':
      return false;
    default:
      return true;
  }
}

static RawObject raiseOverflowError(Thread* thread, CastError error) {
  if (error == CastError::Underflow) {
    return thread->raiseWithFmt(LayoutId::kOverflowError, "less than minimum");
  }
  DCHECK(error == CastError::Overflow, "Only two forms of CastErrors");
  return thread->raiseWithFmt(LayoutId::kOverflowError, "greater than maximum");
}

// TODO(T67799743): Abstract out integer cases to int-builtins.cpp for reuse
// with memoryviews
static RawObject packObject(Thread* thread, uword address, char typecode,
                            word index, RawObject value) {
  byte* dst = reinterpret_cast<byte*>(address + index);
  if (isIntTypecode(typecode)) {
    if (!value.isInt()) return Unbound::object();
    switch (typecode) {
      case 'b': {
        OptInt<char> opt_val = RawInt::cast(value).asInt<char>();
        if (opt_val.error != CastError::None) {
          return raiseOverflowError(thread, opt_val.error);
        }
        std::memcpy(dst, &opt_val.value, sizeof(opt_val.value));
        break;
      }
      case 'h': {
        OptInt<short> opt_val = RawInt::cast(value).asInt<short>();
        if (opt_val.error != CastError::None) {
          return raiseOverflowError(thread, opt_val.error);
        }
        std::memcpy(dst, &opt_val.value, sizeof(opt_val.value));
        break;
      }
      case 'i': {
        OptInt<int> opt_val = RawInt::cast(value).asInt<int>();
        if (opt_val.error != CastError::None) {
          return raiseOverflowError(thread, opt_val.error);
        }
        std::memcpy(dst, &opt_val.value, sizeof(opt_val.value));
        break;
      }
      case 'l': {
        OptInt<long> opt_val = RawInt::cast(value).asInt<long>();
        if (opt_val.error != CastError::None) {
          return raiseOverflowError(thread, opt_val.error);
        }
        std::memcpy(dst, &opt_val.value, sizeof(opt_val.value));
        break;
      }
      case 'B': {
        OptInt<unsigned char> opt_val =
            RawInt::cast(value).asInt<unsigned char>();
        if (opt_val.error != CastError::None) {
          return raiseOverflowError(thread, opt_val.error);
        }
        std::memcpy(dst, &opt_val.value, sizeof(opt_val.value));
        break;
      }
      case 'H': {
        OptInt<unsigned short> opt_val =
            RawInt::cast(value).asInt<unsigned short>();
        if (opt_val.error != CastError::None) {
          return raiseOverflowError(thread, opt_val.error);
        }
        std::memcpy(dst, &opt_val.value, sizeof(opt_val.value));
        break;
      }
      case 'I': {
        OptInt<unsigned int> opt_val =
            RawInt::cast(value).asInt<unsigned int>();
        if (opt_val.error != CastError::None) {
          return raiseOverflowError(thread, opt_val.error);
        }
        std::memcpy(dst, &opt_val.value, sizeof(opt_val.value));
        break;
      }
      case 'L': {
        OptInt<unsigned long> opt_val =
            RawInt::cast(value).asInt<unsigned long>();
        if (opt_val.error != CastError::None) {
          return raiseOverflowError(thread, opt_val.error);
        }
        std::memcpy(dst, &opt_val.value, sizeof(opt_val.value));
        break;
      }
      case 'q': {
        OptInt<long long> opt_val = RawInt::cast(value).asInt<long long>();
        if (opt_val.error != CastError::None) {
          return raiseOverflowError(thread, opt_val.error);
        }
        std::memcpy(dst, &opt_val.value, sizeof(opt_val.value));
        break;
      }
      case 'Q': {
        OptInt<unsigned long long> opt_val =
            RawInt::cast(value).asInt<unsigned long long>();
        if (opt_val.error != CastError::None) {
          return raiseOverflowError(thread, opt_val.error);
        }
        std::memcpy(dst, &opt_val.value, sizeof(opt_val.value));
        break;
      }
    }
    return NoneType::object();
  }

  Runtime* runtime = thread->runtime();
  switch (typecode) {
    case 'f': {
      if (!runtime->isInstanceOfFloat(value)) return Unbound::object();
      float value_float = Float::cast(floatUnderlying(value)).value();
      std::memcpy(dst, &value_float, sizeof(value_float));
      return NoneType::object();
    }

    case 'd': {
      if (!runtime->isInstanceOfFloat(value)) return Unbound::object();
      double value_double = Float::cast(floatUnderlying(value)).value();
      std::memcpy(dst, &value_double, sizeof(value_double));
      return NoneType::object();
    }

    case 'u':
      UNIMPLEMENTED("array.__setitem__ with unicode is unimplemented");
    default:
      UNREACHABLE("invalid typecode");
  }
  return NoneType::object();
}

// TODO(T67799743): Abstract out integer cases to int-builtins.cpp for reuse
// with memoryviews
static RawObject unpackObject(Thread* thread, uword address, char format,
                              word index) {
  Runtime* runtime = thread->runtime();
  byte* src = reinterpret_cast<byte*>(address + index);
  switch (format) {
    case 'b':
      return RawSmallInt::fromWord(Utils::readBytes<signed char>(src));
    case 'B':
      return RawSmallInt::fromWord(Utils::readBytes<unsigned char>(src));
    case 'h':
      return RawSmallInt::fromWord(Utils::readBytes<short>(src));
    case 'H':
      return RawSmallInt::fromWord(Utils::readBytes<unsigned short>(src));
    case 'i':
      return runtime->newInt(Utils::readBytes<int>(src));
    case 'I':
      return runtime->newInt(Utils::readBytes<unsigned int>(src));
    case 'l':
      return runtime->newInt(Utils::readBytes<long>(src));
    case 'L':
      return runtime->newIntFromUnsigned(Utils::readBytes<unsigned long>(src));
    case 'q':
      return runtime->newInt(Utils::readBytes<long long>(src));
    case 'Q':
      return runtime->newIntFromUnsigned(
          Utils::readBytes<unsigned long long>(src));
    case 'f':
      return runtime->newFloat(Utils::readBytes<float>(src));
    case 'd':
      return runtime->newFloat(Utils::readBytes<double>(src));
    case 'u':
      UNIMPLEMENTED("array.__getitem__ with unicode is unimplemented");
    default:
      UNREACHABLE("invalid format");
  }
}

RawObject FUNC(array, _array_getitem)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfArray(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(array));
  }
  Array array(&scope, *self_obj);

  Object index_obj(&scope, args.get(1));
  if (!runtime->isInstanceOfInt(*index_obj)) {
    return Unbound::object();
  }
  word index = intUnderlying(*index_obj).asWordSaturated();
  if (!SmallInt::isValid(index)) {
    return thread->raiseWithFmt(LayoutId::kIndexError,
                                "cannot fit '%T' into an index-sized integer",
                                &index_obj);
  }
  word length = array.length();
  if (index < 0) {
    index = length - index;
  }
  if (index < 0 || index >= length) {
    return thread->raiseWithFmt(LayoutId::kIndexError,
                                "array index out of range");
  }
  char typecode = Str::cast(array.typecode()).byteAt(0);
  word item_size = itemSize(typecode);
  word byte_index;
  if (__builtin_mul_overflow(index, item_size, &byte_index)) {
    return thread->raiseWithFmt(LayoutId::kIndexError,
                                "array index out of range");
  }
  return unpackObject(thread, MutableBytes::cast(array.buffer()).address(),
                      typecode, byte_index);
}

RawObject FUNC(array, _array_setitem)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfArray(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(array));
  }
  Array array(&scope, *self_obj);

  Object index_obj(&scope, args.get(1));
  if (!runtime->isInstanceOfInt(*index_obj)) {
    return Unbound::object();
  }
  word index = intUnderlying(*index_obj).asWordSaturated();
  if (!SmallInt::isValid(index)) {
    return thread->raiseWithFmt(LayoutId::kIndexError,
                                "cannot fit '%T' into an index-sized integer",
                                &index_obj);
  }
  word length = array.length();
  if (index < 0) {
    index = length - index;
  }
  if (index < 0 || index >= length) {
    return thread->raiseWithFmt(LayoutId::kIndexError,
                                "array assignment index out of range");
  }
  char typecode = Str::cast(array.typecode()).byteAt(0);
  word item_size = itemSize(typecode);
  word byte_index;
  if (__builtin_mul_overflow(index, item_size, &byte_index)) {
    return thread->raiseWithFmt(LayoutId::kIndexError,
                                "array assignment index out of range");
  }
  return packObject(thread, MutableBytes::cast(array.buffer()).address(),
                    typecode, byte_index, args.get(2));
}

static void arrayEnsureCapacity(Thread* thread, const Array& array,
                                word min_length) {
  DCHECK_BOUND(min_length, SmallInt::kMaxValue);
  HandleScope scope(thread);
  MutableBytes buffer(&scope, array.buffer());
  word curr_length = buffer.length();
  if (min_length <= curr_length) return;
  word new_length = Runtime::newCapacity(curr_length, min_length);
  MutableBytes new_buffer(
      &scope, thread->runtime()->newMutableBytesUninitialized(new_length));
  new_buffer.replaceFromWith(0, *buffer, curr_length);
  new_buffer.replaceFromWithByte(curr_length, 0, new_length - curr_length);
  array.setBuffer(*new_buffer);
}

RawObject FUNC(array, _array_reserve)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object array_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfArray(*array_obj)) {
    return thread->raiseRequiresType(array_obj, ID(array));
  }
  Array array(&scope, *array_obj);
  word item_size = itemSize(Str::cast(array.typecode()).byteAt(0));
  word size = intUnderlying(args.get(1)).asWord() * item_size;
  arrayEnsureCapacity(thread, array, size);
  return NoneType::object();
}

RawObject FUNC(array, _array_append)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfArray(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(array));
  }
  Array array(&scope, *self_obj);
  char typecode = Str::cast(array.typecode()).byteAt(0);
  word item_size = itemSize(typecode);
  word length = array.length();
  // This shouldn't overflow, since length is limited to a SmallInt
  word new_length = length + 1;
  word new_capacity;
  if (__builtin_mul_overflow(new_length, item_size, &new_capacity)) {
    return thread->raiseWithFmt(LayoutId::kIndexError,
                                "array assignment index out of range");
  }

  arrayEnsureCapacity(thread, array, new_capacity);
  MutableBytes buffer(&scope, array.buffer());
  Object result(&scope, packObject(thread, buffer.address(), typecode,
                                   new_capacity - item_size, args.get(1)));
  if (!result.isErrorException() && !result.isUnbound()) {
    array.setLength(new_length);
  }
  return *result;
}

RawObject METH(array, __len__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfArray(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(array));
  }
  Array self(&scope, args.get(0));
  return SmallInt::fromWord(self.length());
}

static const BuiltinAttribute kArrayAttributes[] = {
    {ID(_array__buffer), RawArray::kBufferOffset, AttributeFlags::kHidden},
    {ID(_array__length), RawArray::kLengthOffset, AttributeFlags::kHidden},
    {ID(typecode), RawArray::kTypecodeOffset, AttributeFlags::kReadOnly},
};

void initializeArrayType(Thread* thread) {
  addBuiltinType(thread, ID(array), LayoutId::kArray,
                 /*superclass_id=*/LayoutId::kObject, kArrayAttributes,
                 Array::kSize, /*basetype=*/true);
}

}  // namespace py
