#pragma once

#include "globals.h"
#include "runtime.h"

namespace python {

class IntBuiltins {
 public:
  static void initialize(Runtime* runtime);

  static Object* bitLength(Thread* thread, Frame* frame, word nargs);
  static Object* dunderBool(Thread* thread, Frame* frame, word nargs);
  static Object* dunderEq(Thread* thread, Frame* frame, word nargs);
  static Object* dunderGe(Thread* thread, Frame* frame, word nargs);
  static Object* dunderGt(Thread* thread, Frame* frame, word nargs);
  static Object* dunderNe(Thread* thread, Frame* frame, word nargs);
  static Object* dunderNew(Thread* thread, Frame* frame, word nargs);
  static Object* dunderNeg(Thread* thread, Frame* frame, word nargs);
  static Object* dunderInt(Thread* thread, Frame* frame, word nargs);
  static Object* dunderLe(Thread* thread, Frame* frame, word nargs);
  static Object* dunderLt(Thread* thread, Frame* frame, word nargs);
  static Object* dunderPos(Thread* thread, Frame* frame, word nargs);
  static Object* intFromString(Thread* thread, Object* str, int base);
  static Object* intFromBool(Object* bool_obj);

 private:
  static Object* negateLargeInteger(Runtime* runtime,
                                    const Handle<Object>& large_integer);
  static const BuiltinMethod kMethods[];

  DISALLOW_IMPLICIT_CONSTRUCTORS(IntBuiltins);
};

class SmallIntBuiltins {
 public:
  static void initialize(Runtime* runtime);

  static Object* dunderAdd(Thread* thread, Frame* frame, word nargs);
  static Object* dunderAnd(Thread* thread, Frame* frame, word nargs);
  static Object* dunderFloorDiv(Thread* thread, Frame* frame, word nargs);
  static Object* dunderInvert(Thread* thread, Frame* frame, word nargs);
  static Object* dunderMod(Thread* thread, Frame* frame, word nargs);
  static Object* dunderMul(Thread* thread, Frame* frame, word nargs);
  static Object* dunderSub(Thread* thread, Frame* frame, word nargs);
  static Object* dunderTrueDiv(Thread* thread, Frame* frame, word nargs);
  static Object* dunderXor(Thread* thread, Frame* frame, word nargs);
  static Object* dunderRepr(Thread* thread, Frame* frame, word nargs);

 private:
  static const BuiltinMethod kMethods[];

  DISALLOW_IMPLICIT_CONSTRUCTORS(SmallIntBuiltins);
};

class BoolBuiltins {
 public:
  static void initialize(Runtime* runtime);

  static Object* dunderNew(Thread* thread, Frame* frame, word nargs);

 private:
  static const BuiltinMethod kMethods[];

  DISALLOW_IMPLICIT_CONSTRUCTORS(BoolBuiltins);
};

}  // namespace python
