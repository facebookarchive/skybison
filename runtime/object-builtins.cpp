#include "function-builtins.h"

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

Object* builtinObjectInit(Thread* thread, Frame*, word nargs) {
  if (nargs != 1) {
    thread->throwTypeErrorFromCString("object.__init__() takes no arguments");
    return Error::object();
  }
  return None::object();
}

Object* builtinObjectNew(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  if (nargs < 1) {
    thread->throwTypeErrorFromCString("object.__new__() takes no arguments");
    return Error::object();
  }
  HandleScope scope(thread->handles());
  Handle<Class> klass(&scope, args.get(0));
  Handle<Layout> layout(&scope, klass->instanceLayout());
  return thread->runtime()->newInstance(layout);
}

} // namespace python
