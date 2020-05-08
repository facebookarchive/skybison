#pragma once

#include "globals.h"
#include "handles.h"
#include "objects.h"

namespace py {

class Thread;

word doubleHash(double value);

word floatHash(RawObject value);

void initializeFloatType(Thread* thread);

void decodeDouble(double value, bool* is_neg, int* exp, int64_t* mantissa);

inline word floatHash(RawObject value) {
  return doubleHash(Float::cast(value).value());
}

}  // namespace py
