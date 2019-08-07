#pragma once

#include "frame.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

/* Attempts to get a float value out of obj.
 * If obj is already a RawFloat, return it immediately.
 * Otherwise, check if obj has a __float__ method.
 * If the result of __float__ is RawFloat, then return it.
 * Returns Error::object() and sets an exception if obj is not a float and does
 * not have __float__, or if __float__ returns some non-float value.
 */
RawObject asFloatObject(Thread* thread, const Object& obj);

// Grabs the base float value from an instance of float.
RawObject floatUnderlying(Thread* thread, const Object& obj);

class FloatBuiltins
    : public Builtins<FloatBuiltins, SymbolId::kFloat, LayoutId::kFloat> {
 public:
  static RawObject dunderAbs(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderAdd(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderBool(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderEq(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderFloat(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderGe(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderGt(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderInt(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderLe(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderLt(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderMul(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderNeg(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderNew(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderPow(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderRound(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderRtrueDiv(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderSub(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderTrueDiv(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderTrunc(Thread* thread, Frame* frame, word nargs);

  static const BuiltinMethod kBuiltinMethods[];
  static const BuiltinAttribute kAttributes[];

 private:
  static RawObject floatFromObject(Thread* thread, Frame* frame, word nargs);
  static RawObject floatFromString(Thread* thread, RawStr str);

  DISALLOW_IMPLICIT_CONSTRUCTORS(FloatBuiltins);
};

void decodeDouble(double value, bool* is_neg, int* exp, uint64_t* mantissa);

}  // namespace python
