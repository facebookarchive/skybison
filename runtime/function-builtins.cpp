#include "function-builtins.h"

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

Object* builtinFunctionGet(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 3) {
    return thread->throwTypeErrorFromCString("__get__ needs 3 arguments");
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Handle<Object> self(&scope, args.get(0));
  Handle<Object> instance(&scope, args.get(1));
  if (instance->isNone()) {
    return *self;
  }
  return thread->runtime()->newBoundMethod(self, instance);
}

} // namespace python
