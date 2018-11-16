//
// Created by Carl S. Shapiro on 9/15/17.
//

#ifndef PYTHON_INTERPRETER_H
#define PYTHON_INTERPRETER_H

#include "Globals.h"

namespace python {

class Object;
class Frame;
class Thread;

class Interpreter {
 public:
  static Object* execute(Thread *thread, Frame *frame);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(Interpreter);
};

}

#endif //PYTHON_INTERPRETER_H
