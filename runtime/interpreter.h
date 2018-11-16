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
  static void initOpTable();
  static Object* execute(Thread* thread, Frame* frame);

  static Object* compare(
      CompareOp op,
      const Handle<Object>& left,
      const Handle<Object>& right);
  static Object* richCompare(
      CompareOp op,
      const Handle<Object>& left,
      const Handle<Object>& right);

  static Object* call(Thread* thread, Frame* frame, Object** sp, word nargs);
  static Object*
  callBoundMethod(Thread* thread, Frame* frame, Object** sp, word nargs);

  static Object*
  callType(Thread* thread, Frame* frame, Object** sp, word nargs);

  static Object* callKw(Thread* thread, Frame* frame, Object** sp, word nargs);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(Interpreter);
};

} // namespace python
