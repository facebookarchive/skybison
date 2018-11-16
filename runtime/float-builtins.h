#pragma once

#include "frame.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

class FloatBuiltins {
 public:
  static void initialize(Runtime* runtime);

  static Object* dunderAdd(Thread* thread, Frame* frame, word nargs);
  static Object* dunderEq(Thread* thread, Frame* frame, word nargs);
  static Object* dunderGe(Thread* thread, Frame* frame, word nargs);
  static Object* dunderGt(Thread* thread, Frame* frame, word nargs);
  static Object* dunderLe(Thread* thread, Frame* frame, word nargs);
  static Object* dunderLt(Thread* thread, Frame* frame, word nargs);
  static Object* dunderNe(Thread* thread, Frame* frame, word nargs);
  static Object* dunderNew(Thread* thread, Frame* frame, word nargs);
  static Object* dunderPow(Thread* thread, Frame* frame, word nargs);
  static Object* dunderSub(Thread* thread, Frame* frame, word nargs);

 private:
  static Object* floatFromObject(Thread* thread, Frame* frame,
                                 const Handle<Object>& obj);
  static Object* floatFromString(Thread* thread, Str* str);
  static const BuiltinMethod kMethods[];

  DISALLOW_IMPLICIT_CONSTRUCTORS(FloatBuiltins);
};

}  // namespace python
