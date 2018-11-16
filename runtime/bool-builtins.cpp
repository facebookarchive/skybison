#include "bool-builtins.h"

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "thread.h"

namespace python {

Object *builtinBooldunderBool(Thread *thread, Frame *frame, word nargs) {
  if (nargs != 1) {
    return thread->throwTypeErrorFromCString("not enough arguments");
  }
  Arguments args(frame, nargs);
  if (args.get(0)->isBool()) {
    return args.get(0);
  }
  return thread->throwTypeErrorFromCString("unsupported type for __bool__");
}

}  // namespace python
