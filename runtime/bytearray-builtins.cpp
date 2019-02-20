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

const BuiltinAttribute ByteArrayBuiltins::kAttributes[] = {
    {SymbolId::kItems, ByteArray::kBytesOffset},
    {SymbolId::kAllocated, ByteArray::kNumItemsOffset},
};

const BuiltinMethod ByteArrayBuiltins::kBuiltinMethods[] = {
    {SymbolId::kDunderNew, dunderNew},
};

void ByteArrayBuiltins::initialize(Runtime* runtime) {
  HandleScope scope;
  runtime->addBuiltinType(SymbolId::kByteArray, LayoutId::kByteArray,
                          LayoutId::kObject, kAttributes,
                          View<NativeMethod>(nullptr, 0), kBuiltinMethods);
}

RawObject ByteArrayBuiltins::dunderNew(Thread* thread, Frame* frame,
                                       word nargs) {
  Arguments args(frame, nargs);
  if (!args.get(0)->isType()) {
    return thread->raiseTypeErrorWithCStr("not a type object");
  }
  HandleScope scope(thread);
  Type type(&scope, args.get(0));
  if (type.builtinBase() != LayoutId::kByteArray) {
    return thread->raiseTypeErrorWithCStr("not a subtype of bytearray");
  }
  Layout layout(&scope, type.instanceLayout());
  Runtime* runtime = thread->runtime();
  ByteArray result(&scope, runtime->newInstance(layout));
  result.setNumItems(0);
  result.setBytes(runtime->newBytes(0, 0));
  return *result;
}

}  // namespace python
