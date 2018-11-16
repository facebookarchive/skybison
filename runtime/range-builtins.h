#pragma once

#include "globals.h"
#include "runtime.h"

namespace python {

// TODO(dulinr): write this in Python
class RangeBuiltins {
 public:
  static void initialize(Runtime* runtime);

  static Object* dunderIter(Thread* thread, Frame* frame, word nargs);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(RangeBuiltins);
};

class RangeIteratorBuiltins {
 public:
  static void initialize(Runtime* runtime);

  static Object* dunderIter(Thread* thread, Frame* frame, word nargs);
  static Object* dunderNext(Thread* thread, Frame* frame, word nargs);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(RangeIteratorBuiltins);
};

}  // namespace python
