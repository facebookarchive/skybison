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

class ByteArrayBuiltins {
 public:
  static void initialize(Runtime* runtime);

  static RawObject dunderInit(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderNew(Thread* thread, Frame* frame, word nargs);

 private:
  static const BuiltinAttribute kAttributes[];
  static const BuiltinMethod kBuiltinMethods[];

  DISALLOW_IMPLICIT_CONSTRUCTORS(ByteArrayBuiltins);
};

}  // namespace python
