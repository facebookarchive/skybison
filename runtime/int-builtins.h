#pragma once

#include "globals.h"
#include "runtime.h"

namespace python {

// Convert object to a RawInt.
// - If the object is an Int return it unchanged.
// - If object has an __int__ method call it and return the result.
// Throws if the conversion isn't possible.
RawObject asIntObject(Thread* thread, const Object& object);

// Convert int `value` to double.
// Returns a NoneType and sets `result` if the conversion was successful,
// raises an error otherwise.
RawObject convertIntToDouble(Thread* thread, const Int& value, double* result);

// Returns true if the Float `left` is equals Int `right`. Returns false if
// `right` cannot be exactly represented as a Float.
bool doubleEqualsInt(Thread* thread, double left, const Int& right);

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
  static RawObject dunderGe(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderGt(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderMod(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderMul(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderNe(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderNew(Thread* thread, Frame* frame, word nargs);
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
  static RawObject fromBytes(Thread* thread, Frame* frame, word nargs);
  static RawObject fromBytesKw(Thread* thread, Frame* frame, word nargs);
  static RawObject intFromString(Thread* thread, RawObject str, int base);
  static RawObject intFromBool(RawObject bool_obj);
  static RawObject toBytes(Thread* thread, Frame* frame, word nargs);

  // Converts a bool value to a small int, returns other values unchanged.
  static RawObject asInt(const Int& value);

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
  static void postInitialize(Runtime*, const Type& new_type) {
    new_type.setBuiltinBase(kSuperType);
  }

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

}  // namespace python
