#include "bytearray-builtins.h"

#include "runtime.h"

namespace python {

RawObject byteArrayAsBytes(Thread* thread, Runtime* runtime,
                           const ByteArray& array) {
  HandleScope scope(thread);
  Bytes bytes(&scope, array.bytes());
  return runtime->bytesSubseq(thread, bytes, 0, array.numItems());
}

void ByteArrayBuiltins::initialize(Runtime* runtime) {
  HandleScope scope;
  runtime->addEmptyBuiltinType(SymbolId::kByteArray, LayoutId::kByteArray,
                               LayoutId::kObject);
}

}  // namespace python
