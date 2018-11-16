#pragma once

#include "bytecode.h"
#include "globals.h"
#include "handles.h"

namespace python {

class Object;
class Frame;
class Thread;

class Interpreter {
 public:
  static Object* execute(Thread* thread, Frame* frame);

  static Object* compare(
      CompareOp op,
      const Handle<Object>& left,
      const Handle<Object>& right);
  static Object* richCompare(
      CompareOp op,
      const Handle<Object>& left,
      const Handle<Object>& right);

 private:
  static Object* call(Thread* thread, Frame* frame, Object** sp, word nargs);
  static Object*
  callBoundMethod(Thread* thread, Frame* frame, Object** sp, word nargs);

  static Object* callKw(Thread* thread, Frame* frame, Object** sp, word nargs);

  DISALLOW_IMPLICIT_CONSTRUCTORS(Interpreter);
};

} // namespace python
