#pragma once

#include "globals.h"
#include "runtime.h"

namespace py {

// Convert bool `object` to Int.
RawObject convertBoolToInt(RawObject object);

// Convert int `value` to double. Returns a NoneType and sets `result` if the
// conversion was successful, raises an OverflowError otherwise.
RawObject convertIntToDouble(Thread* thread, const Int& value, double* result);

// Calculates the GCD of two Ints using the Euclidean algorithm
RawObject intGCD(Thread* thread, const Int& a, const Int& b);

// Returns true if the Float `left` is equals Int `right`. Returns false if
// `right` cannot be exactly represented as a Float.
bool doubleEqualsInt(Thread* thread, double left, const Int& right);

// Performs a `<`, `<=`, `>=` or `>` comparison between the Float `left`
// and the Int `right`. If `right` cannot be exactly represented as a Float
// the numbers are considered inequal and the rounding direction determines
// the lesser/greater status.
bool compareDoubleWithInt(Thread* thread, double left, const Int& right,
                          CompareOp op);

bool intDivmodNear(Thread* thread, const Int& dividend, const Int& divisor,
                   Object* quotient, Object* remainder);

// Converts obj into an integer using __index__. Equivalent to `PyNumber_Index`.
// Returns obj if obj is an instance of Int. Raises a TypeError if a non-Int obj
// does not have __index__ or if __index__ returns non-int.
RawObject intFromIndex(Thread* thread, const Object& obj);

word largeIntHash(RawLargeInt value);

word intHash(RawObject value);

void initializeIntTypes(Thread* thread);

inline word intHash(RawObject value) {
  if (value.isSmallInt()) {
    return SmallInt::cast(value).hash();
  }
  if (value.isBool()) {
    return Bool::cast(value).hash();
  }
  return largeIntHash(LargeInt::cast(value));
}

}  // namespace py
