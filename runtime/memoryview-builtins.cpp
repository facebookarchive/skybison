#include "memoryview-builtins.h"

#include "int-builtins.h"

namespace python {

const BuiltinMethod MemoryViewBuiltins::kBuiltinMethods[] = {
    {SymbolId::kCast, cast},
    {SymbolId::kDunderGetitem, dunderGetItem},
    {SymbolId::kDunderLen, dunderLen},
    {SymbolId::kDunderNew, dunderNew},
    {SymbolId::kSentinelId, nullptr},
};

static char formatChar(const Str& format) {
  if (format.length() == 2) {
    if (format.charAt(0) != '@') return -1;
    return format.charAt(1);
  }
  if (format.length() != 1) return -1;
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

static word pow2_remainder(word dividend, word divisor) {
  DCHECK(divisor > 0 && Utils::isPowerOfTwo(divisor), "must be power of two");
  word mask = divisor - 1;
  return dividend & mask;
}

RawObject MemoryViewBuiltins::cast(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!self_obj.isMemoryView()) {
    return thread->raiseRequiresType(self_obj, SymbolId::kMemoryView);
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

RawObject MemoryViewBuiltins::dunderGetItem(Thread* thread, Frame* frame,
                                            word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!self_obj.isMemoryView()) {
    return thread->raiseRequiresType(self_obj, SymbolId::kMemoryView);
  }
  MemoryView self(&scope, *self_obj);

  Object index_obj(&scope, args.get(1));
  Object index_int_obj(&scope, intFromIndex(thread, index_obj));
  if (index_int_obj.isError()) return *index_int_obj;
  Int index_int(&scope, intUnderlying(thread, index_int_obj));
  if (index_int.isLargeInt()) {
    return thread->raiseWithFmt(LayoutId::kIndexError,
                                "cannot fit '%T' into an index-sized integer",
                                &index_obj);
  }
  word index = index_int.asWord();
  // TODO(T36619828) support str subclasses
  Str format(&scope, self.format());
  char format_c = formatChar(format);
  DCHECK(format_c > 0, "invalid memoryview");
  word item_size = itemSize(format_c);
  DCHECK(item_size > 0, "invalid memoryview");
  word index_abs = std::abs(index);
  word length = self.length();
  word byte_index;
  if (__builtin_mul_overflow(index_abs, item_size, &byte_index) ||
      byte_index + (item_size - 1) >= length) {
    return thread->raiseWithFmt(LayoutId::kIndexError, "index out of bounds");
  }
  if (index < 0) {
    byte_index = length - byte_index;
  }
  Object buffer(&scope, self.buffer());
  Runtime* runtime = thread->runtime();
  if (runtime->isInstanceOfBytes(*buffer)) {
    // TODO(T38246066) support bytes subclasses
    if (buffer.isLargeBytes()) {
      LargeBytes bytes(&scope, *buffer);
      return unpackObject(thread, bytes.address(), length, format_c,
                          byte_index);
    }
    CHECK(buffer.isSmallBytes(),
          "memoryview.__getitem__ with non bytes/memory");
    Bytes bytes(&scope, *buffer);
    byte bytes_buffer[SmallBytes::kMaxLength];
    bytes.copyTo(bytes_buffer, length);
    return unpackObject(thread, reinterpret_cast<uword>(bytes_buffer), length,
                        format_c, byte_index);
  }
  CHECK(buffer.isInt(), "memoryview.__getitem__ with non bytes/memory");
  return unpackObject(thread, Int::cast(*buffer).asInt<uword>().value, length,
                      format_c, byte_index);
}

RawObject MemoryViewBuiltins::dunderLen(Thread* thread, Frame* frame,
                                        word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!self_obj.isMemoryView()) {
    return thread->raiseRequiresType(self_obj, SymbolId::kMemoryView);
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

RawObject MemoryViewBuiltins::dunderNew(Thread* thread, Frame* frame,
                                        word nargs) {
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
    Bytes bytes(&scope, bytearray.bytes());
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
  return thread->raiseWithFmt(LayoutId::kTypeError,
                              "memoryview: a bytes-like object is required");
}

}  // namespace python
