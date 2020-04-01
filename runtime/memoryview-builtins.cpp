#include "memoryview-builtins.h"

#include "builtins.h"
#include "bytes-builtins.h"
#include "float-builtins.h"
#include "int-builtins.h"
#include "mmap-module.h"

namespace py {

const BuiltinAttribute MemoryViewBuiltins::kAttributes[] = {
    {ID(format), RawMemoryView::kFormatOffset, AttributeFlags::kReadOnly},
    {ID(readonly), RawMemoryView::kReadOnlyOffset, AttributeFlags::kReadOnly},
    {SymbolId::kSentinelId, -1},
};

static char formatChar(const Str& format) {
  if (format.charLength() == 2) {
    if (format.charAt(0) != '@') return -1;
    return format.charAt(1);
  }
  if (format.charLength() != 1) return -1;
  return format.charAt(0);
}

static word itemSize(char format) {
  switch (format) {
    case 'c':
    case 'b':
    case 'B':
      return sizeof(char);
    case 'h':
    case 'H':
      return sizeof(short);
    case 'i':
    case 'I':
      return sizeof(int);
    case 'l':
    case 'L':
      return sizeof(long);
    case 'q':
    case 'Q':
      return sizeof(long long);
    case 'n':
    case 'N':
      return sizeof(size_t);
    case 'f':
      return sizeof(float);
    case 'd':
      return sizeof(double);
    case '?':
      return sizeof(bool);
    case 'P':
      return sizeof(void*);
    default:
      return -1;
  }
}

RawObject memoryviewItemsize(Thread* thread, const MemoryView& view) {
  HandleScope scope(thread);
  Str format(&scope, view.format());
  char format_c = formatChar(format);
  DCHECK(format_c > 0, "invalid memoryview");
  word item_size = itemSize(format_c);
  DCHECK(item_size > 0, "invalid memoryview");
  return SmallInt::fromWord(item_size);
}

static RawObject raiseInvalidValueError(Thread* thread, char format) {
  return thread->raiseWithFmt(LayoutId::kValueError,
                              "memoryview: invalid value for format '%c'",
                              format);
}

static RawObject raiseInvalidTypeError(Thread* thread, char format) {
  return thread->raiseWithFmt(
      LayoutId::kTypeError, "memoryview: invalid type for format '%c'", format);
}

static bool isIntFormat(char format) {
  switch (format) {
    case 'b':
      FALLTHROUGH;
    case 'h':
      FALLTHROUGH;
    case 'i':
      FALLTHROUGH;
    case 'l':
      FALLTHROUGH;
    case 'B':
      FALLTHROUGH;
    case 'H':
      FALLTHROUGH;
    case 'I':
      FALLTHROUGH;
    case 'L':
      FALLTHROUGH;
    case 'q':
      FALLTHROUGH;
    case 'Q':
      FALLTHROUGH;
    case 'n':
      FALLTHROUGH;
    case 'N':
      FALLTHROUGH;
    case 'P':
      return true;
    default:
      return false;
  }
}

static RawObject packObject(Thread* thread, uword address, char format,
                            word index, RawObject value) {
  byte* dst = reinterpret_cast<byte*>(address + index);
  if (isIntFormat(format)) {
    if (!value.isInt()) return Unbound::object();
    switch (format) {
      case 'b': {
        OptInt<char> opt_val = RawInt::cast(value).asInt<char>();
        if (opt_val.error != CastError::None) {
          return raiseInvalidValueError(thread, format);
        }
        std::memcpy(dst, &opt_val.value, sizeof(opt_val.value));
        break;
      }
      case 'h': {
        OptInt<short> opt_val = RawInt::cast(value).asInt<short>();
        if (opt_val.error != CastError::None) {
          return raiseInvalidValueError(thread, format);
        }
        std::memcpy(dst, &opt_val.value, sizeof(opt_val.value));
        break;
      }
      case 'i': {
        OptInt<int> opt_val = RawInt::cast(value).asInt<int>();
        if (opt_val.error != CastError::None) {
          return raiseInvalidValueError(thread, format);
        }
        std::memcpy(dst, &opt_val.value, sizeof(opt_val.value));
        break;
      }
      case 'l': {
        OptInt<long> opt_val = RawInt::cast(value).asInt<long>();
        if (opt_val.error != CastError::None) {
          return raiseInvalidValueError(thread, format);
        }
        std::memcpy(dst, &opt_val.value, sizeof(opt_val.value));
        break;
      }
      case 'B': {
        OptInt<unsigned char> opt_val =
            RawInt::cast(value).asInt<unsigned char>();
        if (opt_val.error != CastError::None) {
          return raiseInvalidValueError(thread, format);
        }
        std::memcpy(dst, &opt_val.value, sizeof(opt_val.value));
        break;
      }
      case 'H': {
        OptInt<unsigned short> opt_val =
            RawInt::cast(value).asInt<unsigned short>();
        if (opt_val.error != CastError::None) {
          return raiseInvalidValueError(thread, format);
        }
        std::memcpy(dst, &opt_val.value, sizeof(opt_val.value));
        break;
      }
      case 'I': {
        OptInt<unsigned int> opt_val =
            RawInt::cast(value).asInt<unsigned int>();
        if (opt_val.error != CastError::None) {
          return raiseInvalidValueError(thread, format);
        }
        std::memcpy(dst, &opt_val.value, sizeof(opt_val.value));
        break;
      }
      case 'L': {
        OptInt<unsigned long> opt_val =
            RawInt::cast(value).asInt<unsigned long>();
        if (opt_val.error != CastError::None) {
          return raiseInvalidValueError(thread, format);
        }
        std::memcpy(dst, &opt_val.value, sizeof(opt_val.value));
        break;
      }
      case 'q': {
        OptInt<long long> opt_val = RawInt::cast(value).asInt<long long>();
        if (opt_val.error != CastError::None) {
          return raiseInvalidValueError(thread, format);
        }
        std::memcpy(dst, &opt_val.value, sizeof(opt_val.value));
        break;
      }
      case 'Q': {
        OptInt<unsigned long long> opt_val =
            RawInt::cast(value).asInt<unsigned long long>();
        if (opt_val.error != CastError::None) {
          return raiseInvalidValueError(thread, format);
        }
        std::memcpy(dst, &opt_val.value, sizeof(opt_val.value));
        break;
      }
      case 'n': {
        OptInt<ssize_t> opt_val = RawInt::cast(value).asInt<ssize_t>();
        if (opt_val.error != CastError::None) {
          return raiseInvalidValueError(thread, format);
        }
        std::memcpy(dst, &opt_val.value, sizeof(opt_val.value));
        break;
      }
      case 'N': {
        OptInt<size_t> opt_val = RawInt::cast(value).asInt<size_t>();
        if (opt_val.error != CastError::None) {
          return raiseInvalidValueError(thread, format);
        }
        std::memcpy(dst, &opt_val.value, sizeof(opt_val.value));
        break;
      }
      case 'P': {
        OptInt<uintptr_t> opt_val = RawInt::cast(value).asInt<uintptr_t>();
        if (opt_val.error != CastError::None) {
          return raiseInvalidValueError(thread, format);
        }
        std::memcpy(dst, &opt_val.value, sizeof(opt_val.value));
        break;
      }
    }
    return NoneType::object();
  }

  switch (format) {
    case 'f': {
      if (!value.isFloat()) return Unbound::object();
      float value_float = Float::cast(floatUnderlying(value)).value();
      std::memcpy(dst, &value_float, sizeof(value_float));
      return NoneType::object();
    }

    case 'd': {
      if (!value.isFloat()) return Unbound::object();
      double value_double = Float::cast(floatUnderlying(value)).value();
      std::memcpy(dst, &value_double, sizeof(value_double));
      return NoneType::object();
    }

    case 'c': {
      if (!value.isBytes()) return raiseInvalidTypeError(thread, format);
      RawBytes value_bytes = bytesUnderlying(value);
      if (value_bytes.length() != 1) {
        return raiseInvalidValueError(thread, format);
      }
      *dst = value_bytes.byteAt(0);
      return NoneType::object();
    }

    case '?': {
      if (!value.isBool()) return Unbound::object();
      bool value_bool = Bool::cast(value).value();
      std::memcpy(dst, &value_bool, sizeof(value_bool));
      return NoneType::object();
    }
    default:
      UNREACHABLE("invalid format");
  }
  return NoneType::object();
}

static RawObject unpackObject(Thread* thread, uword address, word length,
                              char format, word index) {
  Runtime* runtime = thread->runtime();
  DCHECK_INDEX(index, length - static_cast<word>(itemSize(format) - 1));
  byte* src = reinterpret_cast<byte*>(address + index);
  switch (format) {
    case 'c':
      return runtime->newBytes(1, Utils::readBytes<byte>(src));
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
    case 'n':
      return runtime->newInt(Utils::readBytes<ssize_t>(src));
    case 'N':
      return runtime->newIntFromUnsigned(Utils::readBytes<size_t>(src));
    case 'P':
      return runtime->newIntFromCPtr(Utils::readBytes<void*>(src));
    case 'f':
      return runtime->newFloat(Utils::readBytes<float>(src));
    case 'd':
      return runtime->newFloat(Utils::readBytes<double>(src));
    case '?': {
      return Bool::fromBool(Utils::readBytes<byte>(src) != 0);
    }
    default:
      UNREACHABLE("invalid format");
  }
}

RawObject memoryviewGetitem(Thread* thread, const MemoryView& view,
                            word index) {
  HandleScope scope(thread);
  Object buffer(&scope, view.buffer());
  Runtime* runtime = thread->runtime();

  // TODO(T36619828) support str subclasses
  Str format(&scope, view.format());
  char format_c = formatChar(format);
  // TODO(T58046846): Replace DCHECK(char > 0) checks
  DCHECK(format_c > 0, "invalid memoryview");

  word length = view.length();
  DCHECK_INDEX(index, length - static_cast<word>(itemSize(format_c) - 1));

  if (runtime->isInstanceOfBytes(*buffer)) {
    // TODO(T38246066) support bytes subclasses
    if (buffer.isLargeBytes()) {
      LargeBytes bytes(&scope, *buffer);
      return unpackObject(thread, bytes.address(), length, format_c, index);
    }
    CHECK(buffer.isSmallBytes(),
          "memoryview.__getitem__ with non bytes/memory");
    Bytes bytes(&scope, *buffer);
    byte bytes_buffer[SmallBytes::kMaxLength];
    bytes.copyTo(bytes_buffer, length);
    return unpackObject(thread, reinterpret_cast<uword>(bytes_buffer), length,
                        format_c, index);
  }
  CHECK(buffer.isInt(), "memoryview.__getitem__ with non bytes/memory");
  return unpackObject(thread, Int::cast(*buffer).asInt<uword>().value, length,
                      format_c, index);
}

RawObject memoryviewSetitem(Thread* thread, const MemoryView& view,
                            const Int& index, const Object& value) {
  HandleScope scope(thread);
  Object buffer(&scope, view.buffer());
  Str format(&scope, view.format());
  char fmt = formatChar(format);
  // TODO(T58046846): Replace DCHECK(char > 0) checks
  DCHECK(fmt > 0, "invalid memoryview");
  word byte_index = index.asWord();
  DCHECK_INDEX(byte_index,
               view.length() - static_cast<word>(itemSize(fmt) - 1));
  if (buffer.isMutableBytes()) {
    return packObject(thread, LargeBytes::cast(*buffer).address(), fmt,
                      byte_index, *value);
  }
  DCHECK(buffer.isInt(), "memoryview.__setitem__ with non bytes/memory");
  return packObject(thread, Int::cast(*buffer).asInt<uword>().value, fmt,
                    byte_index, *value);
}

static RawObject raiseDifferentStructureError(Thread* thread) {
  return thread->raiseWithFmt(
      LayoutId::kValueError,
      "memoryview assignment: lvalue and rvalue have different structures");
}

RawObject memoryviewSetslice(Thread* thread, const MemoryView& view, word start,
                             word stop, word step, word slice_len,
                             const Object& value_obj) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Str format(&scope, view.format());
  char fmt = formatChar(format);
  // TODO(T58046846): Replace DCHECK(char > 0) checks
  DCHECK(fmt > 0, "invalid memoryview");
  Object buffer(&scope, view.buffer());
  Bytes value_bytes(&scope, Bytes::empty());
  if (runtime->isInstanceOfBytes(*value_obj)) {
    value_bytes = *value_obj;
    if (fmt != 'B' || value_bytes.length() != slice_len) {
      return raiseDifferentStructureError(thread);
    }
  } else if (runtime->isInstanceOfByteArray(*value_obj)) {
    ByteArray value_bytearray(&scope, *value_obj);
    if (fmt != 'B' || value_bytearray.numItems() != slice_len) {
      return raiseDifferentStructureError(thread);
    }
    value_bytes = value_bytearray.items();
  }
  if (value_bytes != Bytes::empty()) {
    byte* address = nullptr;
    if (buffer.isMutableBytes()) {
      address = reinterpret_cast<byte*>(LargeBytes::cast(*buffer).address());
    } else {
      DCHECK(buffer.isInt(), "memoryview.__setitem__ with non bytes/memory");
      address = reinterpret_cast<byte*>(Int::cast(*buffer).asCPtr());
    }
    if (step == 1) {
      value_bytes.copyTo(address + start, slice_len);
      return NoneType::object();
    }
    for (word i = 0; start < stop; i++, start += step) {
      address[start] = value_bytes.byteAt(i);
    }
    return NoneType::object();
  }

  if (value_obj.isMemoryView()) {
    MemoryView value(&scope, *value_obj);
    Str value_format(&scope, value.format());
    char value_fmt = formatChar(value_format);
    word item_size = itemSize(value_fmt);
    DCHECK(item_size > 0, "invalid memoryview");
    if (fmt != value_fmt || (value.length() / item_size) != slice_len) {
      return raiseDifferentStructureError(thread);
    }
    byte small_bytes_buf[SmallBytes::kMaxLength];
    uword value_address;
    Object value_buffer(&scope, value.buffer());
    if (value_buffer.isLargeBytes()) {
      value_address = LargeBytes::cast(*value_buffer).address();
    } else if (value_buffer.isInt()) {
      value_address = Int::cast(*value_buffer).asInt<uword>().value;
    } else {
      DCHECK(value_buffer.isSmallBytes(),
             "memoryview.__setitem__ with non bytes/memory");
      Bytes bytes(&scope, *value_buffer);
      bytes.copyTo(small_bytes_buf, value.length());
      value_address = reinterpret_cast<uword>(small_bytes_buf);
    }
    uword address;
    if (buffer.isMutableBytes()) {
      address = LargeBytes::cast(*buffer).address();
    } else {
      DCHECK(buffer.isInt(), "memoryview.__setitem__ with non bytes/memory");
      address = Int::cast(*buffer).asInt<uword>().value;
    }
    if (step == 1 && item_size == 1) {
      std::memcpy(reinterpret_cast<void*>(address + start),
                  reinterpret_cast<void*>(value_address), slice_len);
    }
    start *= item_size;
    step *= item_size;
    for (; start < stop; value_address += item_size, start += step) {
      std::memcpy(reinterpret_cast<void*>(address + start),
                  reinterpret_cast<void*>(value_address), item_size);
    }
    return NoneType::object();
  }
  UNIMPLEMENTED("unexpected buffer");
}

static word pow2_remainder(word dividend, word divisor) {
  DCHECK(divisor > 0 && Utils::isPowerOfTwo(divisor), "must be power of two");
  word mask = divisor - 1;
  return dividend & mask;
}

RawObject METH(memoryview, cast)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!self_obj.isMemoryView()) {
    return thread->raiseRequiresType(self_obj, ID(memoryview));
  }
  MemoryView self(&scope, *self_obj);

  Runtime* runtime = thread->runtime();
  Object format_obj(&scope, args.get(1));
  if (!runtime->isInstanceOfStr(*format_obj)) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "format argument must be a string");
  }
  Str format(&scope, *format_obj);
  char format_c = formatChar(format);
  word item_size;
  if (format_c < 0 || (item_size = itemSize(format_c)) < 0) {
    return thread->raiseWithFmt(
        LayoutId::kValueError,
        "memoryview: destination must be a native single character format "
        "prefixed with an optional '@'");
  }

  word length = self.length();
  if (pow2_remainder(length, item_size) != 0) {
    return thread->raiseWithFmt(
        LayoutId::kValueError,
        "memoryview: length is not a multiple of itemsize");
  }
  Object buffer(&scope, self.buffer());
  MemoryView result(
      &scope, runtime->newMemoryView(
                  thread, buffer, length,
                  self.readOnly() ? ReadOnly::ReadOnly : ReadOnly::ReadWrite));
  result.setFormat(*format);
  return *result;
}

RawObject METH(memoryview, __len__)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!self_obj.isMemoryView()) {
    return thread->raiseRequiresType(self_obj, ID(memoryview));
  }
  MemoryView self(&scope, *self_obj);
  // TODO(T36619828) support str subclasses
  Str format(&scope, self.format());
  char format_c = formatChar(format);
  DCHECK(format_c > 0, "invalid format");
  word item_size = itemSize(format_c);
  DCHECK(item_size > 0, "invalid memoryview");
  return SmallInt::fromWord(self.length() / item_size);
}

RawObject METH(memoryview, __new__)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Runtime* runtime = thread->runtime();
  if (args.get(0) != runtime->typeAt(LayoutId::kMemoryView)) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "memoryview.__new__(X): X is not 'memoryview'");
  }

  Object object(&scope, args.get(1));
  if (runtime->isInstanceOfBytes(*object)) {
    Bytes bytes(&scope, *object);
    return runtime->newMemoryView(thread, bytes, bytes.length(),
                                  ReadOnly::ReadOnly);
  }
  if (runtime->isInstanceOfByteArray(*object)) {
    ByteArray bytearray(&scope, *object);
    Bytes bytes(&scope, bytearray.items());
    return runtime->newMemoryView(thread, bytes, bytearray.numItems(),
                                  ReadOnly::ReadWrite);
  }
  if (object.isMemoryView()) {
    MemoryView view(&scope, *object);
    Object buffer(&scope, view.buffer());
    MemoryView result(
        &scope, runtime->newMemoryView(thread, buffer, view.length(),
                                       view.readOnly() ? ReadOnly::ReadOnly
                                                       : ReadOnly::ReadWrite));
    result.setFormat(view.format());
    return *result;
  }
  if (object.isMmap()) {
    Mmap mmap_obj(&scope, *object);
    Pointer pointer(&scope, mmap_obj.data());
    // TODO(T64584485): Store the Pointer itself inside the memoryview
    MemoryView result(&scope, runtime->newMemoryViewFromCPtr(
                                  thread, pointer.cptr(), pointer.length(),
                                  mmap_obj.isWritable() ? ReadOnly::ReadWrite
                                                        : ReadOnly::ReadOnly));
    result.setFormat(SmallStr::fromCodePoint('B'));
    return *result;
  }
  return thread->raiseWithFmt(LayoutId::kTypeError,
                              "memoryview: a bytes-like object is required");
}

}  // namespace py
