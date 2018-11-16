#include "sys-module.h"

#include "frame.h"
#include "globals.h"
#include "handles.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

Object* builtinSysDisplayhook(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr(
        "displayhook() takes exactly one argument");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Handle<Object> obj(&scope, args.get(0));
  if (obj->isNoneType()) {
    return NoneType::object();
  }
  UNIMPLEMENTED("sys.displayhook()");
}

Object* builtinSysExit(Thread* thread, Frame* frame, word nargs) {
  if (nargs > 1) {
    return thread->raiseTypeErrorWithCStr("exit() accepts at most 1 argument");
  }

  // TODO: PyExc_SystemExit

  word code = 0;  // success
  if (nargs == 1) {
    Arguments args(frame, nargs);
    Object* arg = args.get(0);
    if (!arg->isSmallInt()) {
      return thread->raiseTypeErrorWithCStr("exit() expects numeric argument");
    }
    code = SmallInt::cast(arg)->value();
  }

  std::exit(code);
}

}  // namespace python
