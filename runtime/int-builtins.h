#pragma once

#include "globals.h"
#include "runtime.h"

namespace python {

class IntBuiltins {
 public:
  static void initialize(Runtime* runtime);

  static RawObject bitLength(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderBool(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderEq(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderGe(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderGt(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderNe(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderNew(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderNeg(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderInt(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderLe(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderLt(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderOr(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderPos(Thread* thread, Frame* frame, word nargs);
  static RawObject intFromString(Thread* thread, RawObject str, int base);
  static RawObject intFromBool(RawObject bool_obj);

 private:
  static RawObject negateLargeInteger(Runtime* runtime,
                                      const Object& large_integer);
  static const BuiltinMethod kMethods[];

  DISALLOW_IMPLICIT_CONSTRUCTORS(IntBuiltins);
};

class SmallIntBuiltins {
 public:
  static void initialize(Runtime* runtime);

  static RawObject dunderAdd(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderAnd(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderFloorDiv(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderInvert(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderMod(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderMul(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderSub(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderTrueDiv(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderXor(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderRepr(Thread* thread, Frame* frame, word nargs);

 private:
  static const BuiltinMethod kMethods[];

  DISALLOW_IMPLICIT_CONSTRUCTORS(SmallIntBuiltins);
};

class BoolBuiltins {
 public:
  static void initialize(Runtime* runtime);

  static RawObject dunderNew(Thread* thread, Frame* frame, word nargs);

 private:
  static const BuiltinMethod kMethods[];

  DISALLOW_IMPLICIT_CONSTRUCTORS(BoolBuiltins);
};

}  // namespace python
