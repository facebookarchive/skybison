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

RawObject METH(int, __abs__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(int, __add__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(int, __and__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(int, __bool__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(int, __ceil__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(int, __divmod__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(int, __eq__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(int, __float__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(int, __floor__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(int, __floordiv__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(int, __format__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(int, __ge__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(int, __gt__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(int, __hash__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(int, __index__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(int, __int__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(int, __invert__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(int, __le__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(int, __lshift__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(int, __lt__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(int, __mod__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(int, __mul__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(int, __ne__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(int, __neg__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(int, __or__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(int, __pos__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(int, __repr__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(int, __round__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(int, __rshift__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(int, __str__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(int, __sub__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(int, __truediv__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(int, __trunc__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(int, __xor__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(int, bit_length)(Thread* thread, Frame* frame, word nargs);
RawObject METH(int, conjugate)(Thread* thread, Frame* frame, word nargs);
RawObject METH(int, to_bytes)(Thread* thread, Frame* frame, word nargs);

class IntBuiltins : public Builtins<IntBuiltins, ID(int), LayoutId::kInt> {
 public:
  static void postInitialize(Runtime* runtime, const Type& new_type);

  static const BuiltinAttribute kAttributes[];
  static const BuiltinMethod kBuiltinMethods[];

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

RawObject METH(bool, __new__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(bool, __or__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(bool, __ror__)(Thread* thread, Frame* frame, word nargs);

class BoolBuiltins : public ImmediateBuiltins<BoolBuiltins, ID(bool),
                                              LayoutId::kBool, LayoutId::kInt> {
 public:
  static const BuiltinMethod kBuiltinMethods[];

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
