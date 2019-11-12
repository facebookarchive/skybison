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

class IntBuiltins
    : public Builtins<IntBuiltins, SymbolId::kInt, LayoutId::kInt> {
 public:
  static void postInitialize(Runtime* runtime, const Type& new_type);

  static RawObject bitLength(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderAbs(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderAnd(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderAdd(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderBool(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderDivmod(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderEq(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderFloat(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderFloordiv(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderFormat(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderGe(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderGt(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderHash(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderMod(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderMul(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderNe(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderNeg(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderInt(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderInvert(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderLe(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderLshift(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderLt(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderOr(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderPos(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderRepr(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderRshift(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderSub(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderTrueDiv(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderXor(Thread* thread, Frame* frame, word nargs);
  static RawObject toBytes(Thread* thread, Frame* frame, word nargs);

  static const BuiltinAttribute kAttributes[];
  static const BuiltinMethod kBuiltinMethods[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(IntBuiltins);
};

class SmallIntBuiltins : public Builtins<SmallIntBuiltins, SymbolId::kSmallInt,
                                         LayoutId::kSmallInt, LayoutId::kInt> {
 public:
  static void postInitialize(Runtime* runtime, const Type& new_type);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(SmallIntBuiltins);
};

class LargeIntBuiltins : public Builtins<LargeIntBuiltins, SymbolId::kLargeInt,
                                         LayoutId::kLargeInt, LayoutId::kInt> {
 public:
  static void postInitialize(Runtime* runtime, const Type& new_type);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(LargeIntBuiltins);
};

class BoolBuiltins : public Builtins<BoolBuiltins, SymbolId::kBool,
                                     LayoutId::kBool, LayoutId::kInt> {
 public:
  static void postInitialize(Runtime*, const Type& new_type) {
    new_type.setBuiltinBase(kSuperType);
  }

  static RawObject dunderNew(Thread* thread, Frame* frame, word nargs);

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
