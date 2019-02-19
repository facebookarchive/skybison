#include "bytearray-builtins.h"

#include "runtime.h"

namespace python {

RawObject byteArrayAsBytes(Thread* thread, Runtime* runtime,
                           const ByteArray& array) {
  HandleScope scope(thread);
  Bytes bytes(&scope, array.bytes());
  return runtime->bytesSubseq(thread, bytes, 0, array.numItems());
}

void writeByteAsHexDigits(Thread* thread, const ByteArray& array, byte value) {
  const byte* hex_digits = reinterpret_cast<const byte*>("0123456789abcdef");
  thread->runtime()->byteArrayExtend(
      thread, array, {hex_digits[value >> 4], hex_digits[value & 0xf]});
}

void ByteArrayBuiltins::initialize(Runtime* runtime) {
  HandleScope scope;
  runtime->addEmptyBuiltinType(SymbolId::kByteArray, LayoutId::kByteArray,
                               LayoutId::kObject);
}

}  // namespace python
