#pragma once

#include "runtime.h"

namespace py {

// Appends a byte to the end of the array.
inline void bytearrayAdd(Thread* thread, Runtime* runtime,
                         const Bytearray& array, byte value) {
  runtime->bytearrayExtend(thread, array, View<byte>(&value, 1));
}

// Returns a new RawBytes containing the bytes in the array.
RawObject bytearrayAsBytes(Thread* thread, const Bytearray& array);

// Returns a new Str containing the repr of `array`. On error, raise & return
// OverflowError.
RawObject bytearrayRepr(Thread* thread, const Bytearray& array);

// Writes the value to the array as two hex digits.
void writeByteAsHexDigits(Thread* thread, const Bytearray& array, byte value);

class BytearrayBuiltins
    : public Builtins<BytearrayBuiltins, ID(bytearray), LayoutId::kBytearray> {
 public:
  static const BuiltinAttribute kAttributes[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(BytearrayBuiltins);
};

class BytearrayIteratorBuiltins
    : public Builtins<BytearrayIteratorBuiltins, ID(bytearray_iterator),
                      LayoutId::kBytearrayIterator> {
 public:
  static const BuiltinAttribute kAttributes[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(BytearrayIteratorBuiltins);
};

}  // namespace py
