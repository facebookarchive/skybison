#include "memoryview-builtins.h"

#include "builtins.h"
#include "bytes-builtins.h"
#include "float-builtins.h"
#include "int-builtins.h"
#include "mmap-module.h"
#include "type-builtins.h"

namespace py {

static const BuiltinAttribute kMemoryViewAttributes[] = {
    {ID(format), RawMemoryView::kFormatOffset, AttributeFlags::kReadOnly},
    {ID(obj), RawMemoryView::kObjectOffset, AttributeFlags::kReadOnly},
    {ID(readonly), RawMemoryView::kReadOnlyOffset, AttributeFlags::kReadOnly},
    {ID(shape), RawMemoryView::kShapeOffset, AttributeFlags::kReadOnly},
    {ID(strides), RawMemoryView::kStridesOffset, AttributeFlags::kReadOnly},
    {ID(_memoryview__start), RawMemoryView::kStartOffset,
     AttributeFlags::kHidden},
};

void initializeMemoryViewType(Thread* thread) {
  addBuiltinType(thread, ID(memoryview), LayoutId::kMemoryView,
                 /*superclass_id=*/LayoutId::kObject, kMemoryViewAttributes);
}

static char formatChar(const Str& format) {
  if (format.length() == 2) {
    if (format.byteAt(0) != '@') return -1;
    return format.byteAt(1);
  }
  if (format.length() != 1) return -1;
  return format.byteAt(0);
}

static word itemSize(char format) {
  switch (format) {
    case 'c':
    case 'b':
    case 'B':
      return kByteSize;
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
    case 'n':
    case 'N':
      return kWordSize;
    case 'f':
      return kFloatSize;
    case 'd':
      return kDoubleSize;
    case '?':
      return kBoolSize;
    case 'P':
      return kPointerSize;
    default:
      return -1;
  }
}

word memoryviewItemsize(Thread* thread, const MemoryView& view) {
  HandleScope scope(thread);
  Str format(&scope, view.format());
  char format_c = formatChar(format);
  DCHECK(format_c > 0, "invalid memoryview");
  word item_size = itemSize(format_c);
  DCHECK(item_size > 0, "invalid memoryview");
  return item_size;
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

// Helper function that returns the location within the memoryview buffer to
// find requested index
static word bufferIndex(const MemoryView& view, word index) {
  word step = intUnderlying(Tuple::cast(view.strides()).at(0)).asWord();
  if (step != 1) {
    UNIMPLEMENTED("Stride != 1 is not yet supported");
  }
  DCHECK_INDEX(index, view.length());
  return view.start() + index;
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
  word item_size = itemSize(format_c);
  DCHECK(item_size > 0, "invalid memoryview");
  word buffer_index = bufferIndex(view, index);

  if (runtime->isInstanceOfBytes(*buffer)) {
    // TODO(T38246066) support bytes subclasses
    if (buffer.isLargeBytes()) {
      LargeBytes bytes(&scope, *buffer);
      return unpackObject(thread, bytes.address(), bytes.length(), format_c,
                          buffer_index);
    }
    CHECK(buffer.isSmallBytes(),
          "memoryview.__getitem__ with non bytes/memory");
    Bytes bytes(&scope, *buffer);
    byte bytes_buffer[SmallBytes::kMaxLength];
    bytes.copyTo(bytes_buffer, bytes.length());
    return unpackObject(thread, reinterpret_cast<uword>(bytes_buffer),
                        bytes.length(), format_c, buffer_index);
  }
  CHECK(buffer.isPointer(), "memoryview.__getitem__ with non bytes/memory");
  void* cptr = Pointer::cast(*buffer).cptr();
  word ptr_length = Pointer::cast(*buffer).length();
  return unpackObject(thread, reinterpret_cast<uword>(cptr), ptr_length,
                      format_c, buffer_index);
}

RawObject memoryviewGetslice(Thread* thread, const MemoryView& view, word start,
                             word stop, word step) {
  if (step != 1) {
    UNIMPLEMENTED("Stride != 1 is not yet supported");
  }

  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();

  Str format(&scope, view.format());
  char format_c = formatChar(format);
  // TODO(T58046846): Replace DCHECK(char > 0) checks
  DCHECK(format_c > 0, "invalid memoryview");
  word item_size = itemSize(format_c);
  DCHECK(item_size > 0, "invalid memoryview");
  word slice_len = Slice::length(start, stop, step);
  word slice_byte_size = slice_len * item_size;

  Object buffer(&scope, view.buffer());
  Object obj(&scope, view.object());
  MemoryView result(
      &scope, runtime->newMemoryView(
                  thread, obj, buffer, slice_byte_size,
                  view.readOnly() ? ReadOnly::ReadOnly : ReadOnly::ReadWrite));
  result.setFormat(view.format());
  result.setStart(view.start() + start * item_size);
  return *result;
}

RawObject memoryviewSetitem(Thread* thread, const MemoryView& view, word index,
                            const Object& value) {
  HandleScope scope(thread);
  Object buffer(&scope, view.buffer());
  Str format(&scope, view.format());
  char fmt = formatChar(format);
  // TODO(T58046846): Replace DCHECK(char > 0) checks
  DCHECK(fmt > 0, "invalid memoryview");
  word item_size = itemSize(fmt);
  DCHECK(item_size > 0, "invalid memoryview");

  word buffer_index = bufferIndex(view, index);
  if (buffer.isMutableBytes()) {
    return packObject(thread, LargeBytes::cast(*buffer).address(), fmt,
                      buffer_index, *value);
  }
  DCHECK(buffer.isPointer(), "memoryview.__setitem__ with non bytes/memory");
  void* cptr = Pointer::cast(*buffer).cptr();
  return packObject(thread, reinterpret_cast<uword>(cptr), fmt, buffer_index,
                    *value);
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
  word stride = intUnderlying(Tuple::cast(view.strides()).at(0)).asWord();
  if (view.start() != 0 || stride != 1) {
    UNIMPLEMENTED("Set item with slicing called on a sliced memoryview");
  }

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
  } else if (runtime->isInstanceOfBytearray(*value_obj)) {
    Bytearray value_bytearray(&scope, *value_obj);
    if (fmt != 'B' || value_bytearray.numItems() != slice_len) {
      return raiseDifferentStructureError(thread);
    }
    value_bytes = value_bytearray.items();
  } else if (value_obj.isMemoryView()) {
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
      DCHECK(buffer.isPointer(),
             "memoryview.__setitem__ with non bytes/memory");
      address = reinterpret_cast<uword>(Pointer::cast(*buffer).cptr());
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
  } else if (runtime->isByteslike(*value_obj)) {
    UNIMPLEMENTED("unsupported bytes-like type");
  } else {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "a bytes-like object is required, not '%T'",
                                &value_obj);
  }
  byte* address;
  if (buffer.isMutableBytes()) {
    address = reinterpret_cast<byte*>(LargeBytes::cast(*buffer).address());
  } else {
    DCHECK(buffer.isPointer(), "memoryview.__setitem__ with non bytes/memory");
    address = static_cast<byte*>(Pointer::cast(*buffer).cptr());
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
  Object obj(&scope, self.object());
  MemoryView result(
      &scope, runtime->newMemoryView(
                  thread, obj, buffer, length,
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
    Bytes bytes(&scope, bytesUnderlying(*object));
    MemoryView result(
        &scope, runtime->newMemoryView(thread, object, bytes, bytes.length(),
                                       ReadOnly::ReadOnly));
    return *result;
  }
  if (runtime->isInstanceOfBytearray(*object)) {
    Bytearray bytearray(&scope, *object);
    Bytes bytes(&scope, bytearray.items());
    MemoryView result(&scope, runtime->newMemoryView(thread, object, bytes,
                                                     bytearray.numItems(),
                                                     ReadOnly::ReadWrite));
    return *result;
  }
  if (object.isMemoryView()) {
    MemoryView view(&scope, *object);
    Object buffer(&scope, view.buffer());
    Object view_obj(&scope, view.object());
    MemoryView result(
        &scope, runtime->newMemoryView(thread, view_obj, buffer, view.length(),
                                       view.readOnly() ? ReadOnly::ReadOnly
                                                       : ReadOnly::ReadWrite));
    result.setFormat(view.format());
    return *result;
  }
  if (object.isMmap()) {
    Mmap mmap_obj(&scope, *object);
    Pointer pointer(&scope, mmap_obj.data());
    MemoryView result(
        &scope,
        runtime->newMemoryViewFromCPtr(
            thread, object, pointer.cptr(), pointer.length(),
            mmap_obj.isWritable() ? ReadOnly::ReadWrite : ReadOnly::ReadOnly));
    result.setFormat(SmallStr::fromCodePoint('B'));
    return *result;
  }
  return thread->raiseWithFmt(LayoutId::kTypeError,
                              "memoryview: a bytes-like object is required");
}

}  // namespace py
