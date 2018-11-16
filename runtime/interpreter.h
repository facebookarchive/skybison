#pragma once

#include "globals.h"

namespace python {

class Object;
class Frame;
class Thread;

class Interpreter {
 public:
  static Object* execute(Thread* thread, Frame* frame);

 private:
  static Object* call(Thread* thread, Frame* frame, Object** sp, word nargs);
  static Object*
  callBoundMethod(Thread* thread, Frame* frame, Object** sp, word nargs);

  static Object* callKw(Thread* thread, Frame* frame, Object** sp, word nargs);

  DISALLOW_IMPLICIT_CONSTRUCTORS(Interpreter);
};

} // namespace python
