#include "sys-module.h"

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "thread.h"

namespace python {

Object* builtinSysExit(Thread* thread, Frame* frame, word nargs) {
  if (nargs > 1) {
    return thread->throwTypeErrorFromCString(
        "exit() accepts at most 1 argument");
  }

  // TODO: PyExc_SystemExit

  word code = 0;  // success
  if (nargs == 1) {
    Arguments args(frame, nargs);
    Object* arg = args.get(0);
    if (!arg->isSmallInteger()) {
      return thread->throwTypeErrorFromCString(
          "exit() expects numeric argument");
    }
    code = SmallInteger::cast(arg)->value();
  }

  std::exit(code);
}

}  // namespace python
