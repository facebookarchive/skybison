#include "bytearray-builtins.h"

#include "bytes-builtins.h"
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
    {SymbolId::kDunderInit, dunderInit},
    {SymbolId::kDunderNew, dunderNew},
};

void ByteArrayBuiltins::initialize(Runtime* runtime) {
  HandleScope scope;
  runtime->addBuiltinType(SymbolId::kByteArray, LayoutId::kByteArray,
                          LayoutId::kObject, kAttributes,
                          View<NativeMethod>(nullptr, 0), kBuiltinMethods);
}

RawObject ByteArrayBuiltins::dunderInit(Thread* thread, Frame* frame,
                                        word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfByteArray(*self_obj)) {
    return thread->raiseTypeErrorWithCStr(
        "'__init__' requires a 'bytearray' object");
  }
  ByteArray self(&scope, *self_obj);
  // always clear the existing contents of the array
  if (self.numItems() > 0) {
    self.downsize(0);
  }
  Object source(&scope, args.get(1));
  Object encoding(&scope, args.get(2));
  Object errors(&scope, args.get(3));
  if (source.isUnboundValue()) {
    if (encoding.isUnboundValue() && errors.isUnboundValue()) {
      return NoneType::object();
    }
    return thread->raiseTypeErrorWithCStr(
        "encoding or errors without sequence argument");
  }
  if (runtime->isInstanceOfStr(*source)) {
    if (encoding.isUnboundValue()) {
      return thread->raiseTypeErrorWithCStr(
          "string argument without an encoding");
    }
    UNIMPLEMENTED("string encoding");  // return NoneType::value();
  }
  if (!encoding.isUnboundValue() || !errors.isUnboundValue()) {
    return thread->raiseTypeErrorWithCStr(
        "encoding or errors without a string argument");
  }
  if (runtime->isInstanceOfInt(*source)) {
    // TODO(T38780562): strict subclass of int
    if (!source.isSmallInt()) {
      return thread->raiseOverflowErrorWithCStr(
          "cannot fit count into an index-sized integer");
    }
    word count = RawSmallInt::cast(*source).value();
    if (count < 0) {
      return thread->raiseValueErrorWithCStr("negative count");
    }
    runtime->byteArrayEnsureCapacity(thread, self, count - 1);
    self.setNumItems(count);
  } else if (runtime->isInstanceOfBytes(*source)) {  // TODO(T38246066)
    Bytes bytes(&scope, *source);
    runtime->byteArrayIadd(thread, self, bytes, bytes.length());
  } else if (runtime->isInstanceOfByteArray(*source)) {
    ByteArray array(&scope, *source);
    Bytes bytes(&scope, array.bytes());
    runtime->byteArrayIadd(thread, self, bytes, array.numItems());
  } else {
    Object maybe_bytes(&scope, bytesFromIterable(thread, source));
    if (maybe_bytes.isError()) return *maybe_bytes;
    Bytes bytes(&scope, *maybe_bytes);
    runtime->byteArrayIadd(thread, self, bytes, bytes.length());
  }
  return NoneType::object();
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
