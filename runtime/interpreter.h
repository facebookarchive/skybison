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
      Thread* thread,
      CompareOp op,
      const Handle<Object>& left,
      const Handle<Object>& right);
  static Object* richCompare(
      Thread* thread,
      CompareOp op,
      const Handle<Object>& left,
      const Handle<Object>& right);

  static Object* call(Thread* thread, Frame* frame, Object** sp, word nargs);
  static Object* callKw(Thread* thread, Frame* frame, Object** sp, word nargs);

  // batch concat/join <num> string objects on the stack (no conversion)
  static Object* stringJoin(Thread* thread, Object** sp, word num);

  static Object* isTrue(Thread* thread, Frame* caller, Object** sp);

  static Object* callDescriptorGet(
      Thread* thread,
      Frame* caller,
      const Handle<Object>& descriptor,
      const Handle<Object>& receiver,
      const Handle<Class>& receiver_type);

  static Object* lookupMethod(
      Thread* thread,
      Frame* caller,
      const Handle<Object>& receiver,
      const Handle<Object>& selector,
      bool* is_unbound);

  static Object* unaryOperation(
      Thread* thread,
      Frame* caller,
      Object** sp,
      const Handle<Object>& receiver,
      const Handle<Object>& selector);

 private:
  static Object*
  callBoundMethod(Thread* thread, Frame* frame, Object** sp, word nargs);

  static Object*
  callCallable(Thread* thread, Frame* frame, Object** sp, word nargs);

  DISALLOW_IMPLICIT_CONSTRUCTORS(Interpreter);
};

} // namespace python
