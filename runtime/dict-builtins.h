#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

class DictBuiltins {
 public:
  static void initialize(Runtime* runtime);

  static Object* dunderContains(Thread* thread, Frame* frame, word nargs);
  static Object* dunderDelItem(Thread* thread, Frame* frame, word nargs);
  static Object* dunderEq(Thread* thread, Frame* frame, word nargs);
  static Object* dunderGetItem(Thread* thread, Frame* frame, word nargs);
  static Object* dunderLen(Thread* thread, Frame* frame, word nargs);

 private:
  static const BuiltinMethod kMethods[];

  DISALLOW_IMPLICIT_CONSTRUCTORS(DictBuiltins);
};

}  // namespace python
