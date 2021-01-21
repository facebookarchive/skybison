#pragma once

#include "globals.h"
#include "handles.h"
#include "objects.h"

namespace py {

class Thread;

word doubleHash(double value);

word floatHash(RawObject value);

// Read ASCII digits from str and make a float from those digits.
RawObject floatFromDigits(Thread* thread, const char* str, word length);

void initializeFloatType(Thread* thread);

RawObject intFromDouble(Thread* thread, double value);

void decodeDouble(double value, bool* is_neg, int* exp, int64_t* mantissa);

inline word floatHash(RawObject value) {
  return doubleHash(Float::cast(value).value());
}

}  // namespace py
