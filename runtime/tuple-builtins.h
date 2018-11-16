#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

class TupleBuiltins {
 public:
  static void initialize(Runtime* runtime);

  static Object* dunderEq(Thread* thread, Frame* frame, word nargs);
  static Object* dunderGetItem(Thread* thread, Frame* frame, word nargs);
  static Object* dunderNew(Thread* thread, Frame* frame, word nargs);

  static Object* slice(Thread* thread, ObjectArray* tuple, Slice* slice);

 private:
  static const BuiltinMethod kMethods[];

  DISALLOW_IMPLICIT_CONSTRUCTORS(TupleBuiltins);
};

}  // namespace python
