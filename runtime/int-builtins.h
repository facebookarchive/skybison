#pragma once

#include "globals.h"
#include "runtime.h"

namespace py {

// Convert bool `object` to Int.
RawObject convertBoolToInt(RawObject object);

// Convert int `value` to double. Returns a NoneType and sets `result` if the
// conversion was successful, raises an OverflowError otherwise.
RawObject convertIntToDouble(Thread* thread, const Int& value, double* result);

// Returns true if the Float `left` is equals Int `right`. Returns false if
// `right` cannot be exactly represented as a Float.
bool doubleEqualsInt(Thread* thread, double left, const Int& right);

// Performs a `<`, `<=`, `>=` or `>` comparison between the Float `left`
// and the Int `right`. If `right` cannot be exactly represented as a Float
// the numbers are considered inequal and the rounding direction determines
// the lesser/greater status.
bool compareDoubleWithInt(Thread* thread, double left, const Int& right,
                          CompareOp op);

// Converts obj into an integer using __index__. Equivalent to `PyNumber_Index`.
// Returns obj if obj is an instance of Int. Raises a TypeError if a non-Int obj
// does not have __index__ or if __index__ returns non-int.
RawObject intFromIndex(Thread* thread, const Object& obj);

word largeIntHash(RawLargeInt value);

word intHash(RawObject value);

class IntBuiltins : public Builtins<IntBuiltins, ID(int), LayoutId::kInt> {
 public:
  static void postInitialize(Runtime* runtime, const Type& new_type);

  static const BuiltinAttribute kAttributes[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(IntBuiltins);
};

class SmallIntBuiltins
    : public ImmediateBuiltins<SmallIntBuiltins, ID(smallint),
                               LayoutId::kSmallInt, LayoutId::kInt> {
 public:
  static void postInitialize(Runtime* runtime, const Type& new_type);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(SmallIntBuiltins);
};

class LargeIntBuiltins : public Builtins<LargeIntBuiltins, ID(largeint),
                                         LayoutId::kLargeInt, LayoutId::kInt> {
 public:
  static void postInitialize(Runtime* runtime, const Type& new_type);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(LargeIntBuiltins);
};

class BoolBuiltins : public ImmediateBuiltins<BoolBuiltins, ID(bool),
                                              LayoutId::kBool, LayoutId::kInt> {
 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(BoolBuiltins);
};

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
