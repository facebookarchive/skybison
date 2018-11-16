#include "function-builtins.h"

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

RawObject builtinFunctionGet(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 3) {
    return thread->raiseTypeErrorWithCStr("__get__ needs 3 arguments");
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  Object instance(&scope, args.get(1));
  if (instance->isNoneType()) {
    return *self;
  }
  return thread->runtime()->newBoundMethod(self, instance);
}

}  // namespace python
