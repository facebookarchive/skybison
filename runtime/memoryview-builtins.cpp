#include "memoryview-builtins.h"

#include "int-builtins.h"

namespace python {

const BuiltinMethod MemoryViewBuiltins::kBuiltinMethods[] = {
    {SymbolId::kDunderGetItem, dunderGetItem},
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

static RawObject raiseRequiresMemoryView(Thread* thread) {
  HandleScope scope(thread);
  unique_c_ptr<char> function_name_cstr(thread->functionName().toCStr());
  Object message(&scope, thread->runtime()->newStrFromFormat(
                             "'%s' requires a 'memoryview' object",
                             function_name_cstr.get()));
  return thread->raiseTypeError(*message);
}

RawObject MemoryViewBuiltins::dunderGetItem(Thread* thread, Frame* frame,
                                            word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!self_obj.isMemoryView()) return raiseRequiresMemoryView(thread);
  MemoryView self(&scope, *self_obj);

  Object index_obj(&scope, args.get(1));
  Object index_int_obj(&scope, intFromIndex(thread, index_obj));
  if (index_int_obj.isError()) return *index_int_obj;
  if (!index_int_obj.isSmallInt()) {
    return thread->raiseIndexErrorWithCStr(
        "cannot fit 'int' into an index-sized integer");
  }
  word index = RawSmallInt::cast(*index_int_obj).value();
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
