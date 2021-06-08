#pragma once

#include "asserts.h"
#include "globals.h"
#include "objects.h"

namespace py {

extern const byte kLowerCaseHexDigitArray[16];
extern const byte kUpperCaseHexDigitArray[16];

// Converts an uword to ascii decimal digits. The digits can only be efficiently
// produced from least to most significant without knowing the exact number of
// digits upfront. Because of this the function takes a `buffer_end` argument
// and writes the digit before it. Returns a pointer to the last byte written.
inline byte* uwordToDecimal(uword num, byte* buffer_end) {
  byte* start = buffer_end;
  do {
    *--start = '0' + num % 10;
    num /= 10;
  } while (num > 0);
  return start;
}

inline byte lowerCaseHexDigit(uword value) {
  return kLowerCaseHexDigitArray[value];
}

inline void uwordToHexadecimal(byte* buffer, word num_digits, uword value) {
  DCHECK(num_digits > 0, "num_digits must be positive");
  for (;;) {
    byte b = kLowerCaseHexDigitArray[value & ((1 << kBitsPerHexDigit) - 1)];
    num_digits--;
    buffer[num_digits] = b;
    if (num_digits == 0) break;
    value >>= kBitsPerHexDigit;
  }
}

inline void uwordToHexadecimalWithMutableBytes(RawMutableBytes dest, word index,
                                               word num_digits, uword value) {
  DCHECK(num_digits > 0, "num_digits must be positive");
  for (;;) {
    byte b = kLowerCaseHexDigitArray[value & ((1 << kBitsPerHexDigit) - 1)];
    num_digits--;
    dest.byteAtPut(index + num_digits, b);
    if (num_digits == 0) break;
    value >>= kBitsPerHexDigit;
  }
}

}  // namespace py
