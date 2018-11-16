#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

class IntegerBuiltins {
 public:
  static void initialize(Runtime* runtime);

  static Object* intFromString(Thread* thread, Object* str);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(IntegerBuiltins);
};

class SmallIntegerBuiltins {
 public:
  static void initialize(Runtime* runtime);

  static Object* bitLength(Thread* thread, Frame* frame, word nargs);
  static Object* dunderAdd(Thread* thread, Frame* frame, word nargs);
  static Object* dunderBool(Thread* thread, Frame* frame, word nargs);
  static Object* dunderEq(Thread* thread, Frame* frame, word nargs);
  static Object* dunderFloorDiv(Thread* thread, Frame* frame, word nargs);
  static Object* dunderGe(Thread* thread, Frame* frame, word nargs);
  static Object* dunderGt(Thread* thread, Frame* frame, word nargs);
  static Object* dunderInvert(Thread* thread, Frame* frame, word nargs);
  static Object* dunderLe(Thread* thread, Frame* frame, word nargs);
  static Object* dunderLt(Thread* thread, Frame* frame, word nargs);
  static Object* dunderMod(Thread* thread, Frame* frame, word nargs);
  static Object* dunderMul(Thread* thread, Frame* frame, word nargs);
  static Object* dunderNe(Thread* thread, Frame* frame, word nargs);
  static Object* dunderNeg(Thread* thread, Frame* frame, word nargs);
  static Object* dunderPos(Thread* thread, Frame* frame, word nargs);
  static Object* dunderSub(Thread* thread, Frame* frame, word nargs);
  static Object* dunderXor(Thread* thread, Frame* frame, word nargs);

 private:
  static const BuiltinMethod kMethods[];

  DISALLOW_IMPLICIT_CONSTRUCTORS(SmallIntegerBuiltins);
};

}  // namespace python
