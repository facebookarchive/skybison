#include "ref-builtins.h"

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

RawObject builtinRefInit(Thread* thread, Frame*, word nargs) {
  if (nargs < 2 || nargs > 3) {
    return thread->raiseTypeErrorWithCStr("ref() expected 2 or 3 arguments");
  }
  return NoneType::object();
}

RawObject builtinRefNew(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  if (nargs < 2 || nargs > 3) {
    return thread->raiseTypeErrorWithCStr("ref() expected 2 or 3 arguments");
  }
  Arguments args(frame, nargs);
  Handle<WeakRef> ref(&scope, thread->runtime()->newWeakRef());
  ref->setReferent(args.get(1));
  if (nargs == 3) {
    ref->setCallback(args.get(2));
  }

  return *ref;
}

}  // namespace python
