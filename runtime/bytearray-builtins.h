#pragma once

#include "runtime.h"

namespace python {

// Appends a byte to the end of the array.
inline void byteArrayAdd(Thread* thread, Runtime* runtime,
                         const ByteArray& array, byte value) {
  runtime->byteArrayExtend(thread, array, View<byte>(&value, 1));
}

// Returns a new RawBytes containing the bytes in the array.
RawObject byteArrayAsBytes(Thread* thread, Runtime* runtime,
                           const ByteArray& array);

// Writes the value to the array as two hex digits.
void writeByteAsHexDigits(Thread* thread, const ByteArray& array, byte value);

class ByteArrayBuiltins
    : public Builtins<ByteArrayBuiltins, SymbolId::kByteArray,
                      LayoutId::kByteArray> {
 public:
  static RawObject dunderAdd(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderGetItem(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderIadd(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderInit(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderLen(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderNew(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderRepr(Thread* thread, Frame* frame, word nargs);

  static RawObject hex(Thread* thread, Frame* frame, word nargs);
  static RawObject join(Thread* thread, Frame* frame, word nargs);

  static const BuiltinAttribute kAttributes[];
  static const BuiltinMethod kBuiltinMethods[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(ByteArrayBuiltins);
};

}  // namespace python
