#include "tuple-builtins.h"

#include "frame.h"
#include "globals.h"
#include "interpreter.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

Object* builtinTupleEq(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(frame, nargs);
  if (args.get(0)->isObjectArray() && args.get(1)->isObjectArray()) {
    HandleScope scope(thread->handles());
    Handle<ObjectArray> self(&scope, args.get(0));
    Handle<ObjectArray> other(&scope, args.get(1));
    if (self->length() != other->length()) {
      return Boolean::falseObj();
    }
    Handle<Object> left(&scope, None::object());
    Handle<Object> right(&scope, None::object());
    word length = self->length();
    for (word i = 0; i < length; i++) {
      left = self->at(i);
      right = other->at(i);
      Object* result =
          Interpreter::compareOperation(thread, frame, EQ, left, right);
      if (result == Boolean::falseObj()) {
        return result;
      }
    }
    return Boolean::trueObj();
  }
  // TODO(cshapiro): handle user-defined subtypes of tuple.
  return thread->runtime()->notImplemented();
}

} // namespace python
