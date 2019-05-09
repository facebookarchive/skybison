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

static RawObject unpackObject(Thread* thread, const Bytes& bytes, char format,
                              word index) {
  Runtime* runtime = thread->runtime();
  switch (format) {
    case 'c':
      return runtime->newBytes(1, bytes.byteAt(index));
    case 'b':
      return RawSmallInt::fromWord(bit_cast<signed char>(bytes.byteAt(index)));
    case 'B':
      return RawSmallInt::fromWord(
          bit_cast<unsigned char>(bytes.byteAt(index)));
    case 'h':
      return RawSmallInt::fromWord(bit_cast<short>(bytes.uint16At(index)));
    case 'H':
      return RawSmallInt::fromWord(
          bit_cast<unsigned short>(bytes.uint16At(index)));
    case 'i':
      return runtime->newInt(bit_cast<int>(bytes.uint32At(index)));
    case 'I':
      return runtime->newInt(bit_cast<unsigned int>(bytes.uint32At(index)));
    case 'l':
      return runtime->newInt(bit_cast<long>(bytes.uint64At(index)));
    case 'L':
      return runtime->newIntFromUnsigned(
          bit_cast<unsigned long>(bytes.uint64At(index)));
    case 'q':
      return runtime->newInt(bit_cast<long long>(bytes.uint64At(index)));
    case 'Q':
      return runtime->newIntFromUnsigned(
          bit_cast<unsigned long long>(bytes.uint64At(index)));
    case 'n':
      return runtime->newInt(bit_cast<ssize_t>(bytes.uint64At(index)));
    case 'N':
      return runtime->newIntFromUnsigned(
          bit_cast<size_t>(bytes.uint64At(index)));
    case 'P':
      return runtime->newIntFromCPtr(bit_cast<void*>(bytes.uint64At(index)));
    case 'f':
      return runtime->newFloat(bit_cast<float>(bytes.uint32At(index)));
    case 'd':
      return runtime->newFloat(bit_cast<double>(bytes.uint64At(index)));
    case '?': {
      return Bool::fromBool(bytes.byteAt(index) != 0);
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
    return thread->raiseTypeErrorWithCStr("format argument must be a string");
  }
  Str format(&scope, *format_obj);
  char format_c = formatChar(format);
  word item_size;
  if (format_c < 0 || (item_size = itemSize(format_c)) < 0) {
    return thread->raiseValueErrorWithCStr(
        "memoryview: destination must be a native single character format "
        "prefixed with an optional '@'");
  }

  Bytes buffer(&scope, self.buffer());
  word length = buffer.length();
  if (pow2_remainder(length, item_size) != 0) {
    return thread->raiseValueErrorWithCStr(
        "memoryview: length is not a multiple of itemsize");
  }
  MemoryView result(
      &scope, runtime->newMemoryView(
                  thread, buffer,
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
  // TODO(T38246066) support bytes subclasses
  Bytes bytes(&scope, self.buffer());
  word index_abs = std::abs(index);
  word length = bytes.length();
  word byte_index;
  if (__builtin_mul_overflow(index_abs, item_size, &byte_index) ||
      byte_index + (item_size - 1) >= length) {
    return thread->raiseIndexErrorWithCStr("index out of bounds");
  }
  if (index < 0) {
    byte_index = length - byte_index;
  }
  return unpackObject(thread, bytes, format_c, byte_index);
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
  // TODO(T38246066) support bytes subclasses
  Bytes bytes(&scope, self.buffer());
  word bytes_length = bytes.length();
  // TODO(T36619828) support str subclasses
  Str format(&scope, self.format());
  char format_c = formatChar(format);
  DCHECK(format_c > 0, "invalid format");
  word item_size = itemSize(format_c);
  DCHECK(item_size > 0, "invalid memoryview");
  return SmallInt::fromWord(bytes_length / item_size);
}

RawObject MemoryViewBuiltins::dunderNew(Thread* thread, Frame* frame,
                                        word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Runtime* runtime = thread->runtime();
  if (args.get(0) != runtime->typeAt(LayoutId::kMemoryView)) {
    return thread->raiseTypeErrorWithCStr(
        "memoryview.__new__(X): X is not 'memoryview'");
  }

  Object object(&scope, args.get(1));
  if (runtime->isInstanceOfBytes(*object)) {
    Bytes bytes(&scope, *object);
    return runtime->newMemoryView(thread, bytes, ReadOnly::ReadOnly);
  }
  if (runtime->isInstanceOfByteArray(*object)) {
    ByteArray bytearray(&scope, *object);
    Bytes bytes(&scope, bytearray.bytes());
    return runtime->newMemoryView(thread, bytes, ReadOnly::ReadWrite);
  }
  if (object.isMemoryView()) {
    MemoryView view(&scope, *object);
    Bytes bytes(&scope, view.buffer());
    MemoryView result(
        &scope, runtime->newMemoryView(thread, bytes,
                                       view.readOnly() ? ReadOnly::ReadOnly
                                                       : ReadOnly::ReadWrite));
    result.setFormat(view.format());
    return *result;
  }
  return thread->raiseTypeErrorWithCStr(
      "memoryview: a bytes-like object is required");
}

}  // namespace python
