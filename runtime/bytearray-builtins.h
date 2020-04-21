#pragma once

#include "runtime.h"

namespace py {

// Appends a byte to the end of the array.
inline void byteArrayAdd(Thread* thread, Runtime* runtime,
                         const ByteArray& array, byte value) {
  runtime->byteArrayExtend(thread, array, View<byte>(&value, 1));
}

// Returns a new RawBytes containing the bytes in the array.
RawObject byteArrayAsBytes(Thread* thread, const ByteArray& array);

// Returns a new Str containing the repr of `array`. On error, raise & return
// OverflowError.
RawObject byteArrayRepr(Thread* thread, const ByteArray& array);

// Writes the value to the array as two hex digits.
void writeByteAsHexDigits(Thread* thread, const ByteArray& array, byte value);

class ByteArrayBuiltins
    : public Builtins<ByteArrayBuiltins, ID(bytearray), LayoutId::kByteArray> {
 public:
  static const BuiltinAttribute kAttributes[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(ByteArrayBuiltins);
};

class ByteArrayIteratorBuiltins
    : public Builtins<ByteArrayIteratorBuiltins, ID(bytearray_iterator),
                      LayoutId::kByteArrayIterator> {
 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(ByteArrayIteratorBuiltins);
};

}  // namespace py
