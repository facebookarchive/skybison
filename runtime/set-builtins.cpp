#include "set-builtins.h"

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

Object* builtinSetLen(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 1) {
    return thread->throwTypeErrorFromCString("__len__() takes no arguments");
  }
  HandleScope scope(thread);
  Arguments args(caller, nargs);
  Handle<Object> self(&scope, args.get(0));
  if (self->isSet()) {
    return SmallInteger::fromWord(Set::cast(*self)->numItems());
  }
  // TODO(cshapiro): handle user-defined subtypes of set.
  return thread->throwTypeErrorFromCString("'__len__' requires a 'set' object");
}

} // namespace python
