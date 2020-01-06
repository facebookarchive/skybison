#pragma once

#include "frame.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace py {

word doubleHash(double value);

word floatHash(RawObject value);

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
  static RawObject dunderHash(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderInt(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderLe(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderLt(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderMul(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderNeg(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderPow(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderRound(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderRtrueDiv(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderSub(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderTrueDiv(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderTrunc(Thread* thread, Frame* frame, word nargs);

  static const BuiltinMethod kBuiltinMethods[];
  static const BuiltinAttribute kAttributes[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(FloatBuiltins);
};

void decodeDouble(double value, bool* is_neg, int* exp, uint64_t* mantissa);

inline word floatHash(RawObject value) {
  return doubleHash(Float::cast(value).value());
}

}  // namespace py
