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
  DISALLOW_IMPLICIT_CONSTRUCTORS(Interpreter);
};

} // namespace python
