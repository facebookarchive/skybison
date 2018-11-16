#pragma once

#include "objects.h"
#include "trampolines.h"

#include "utils.h"

namespace python {

// Template for entry points for native functions (e.g. builtins), invoked via
// CALL_FUNCTION
// TODO(t24656189) - replace with JITed code once we have the facilities for
// that.
template <Object* (*Fn)(Thread*, Frame*, word)>
Object* nativeTrampoline(Thread* thread, Frame* previousFrame, word argc) {
  HandleScope scope;

  thread->pushNativeFrame(Utils::castFnPtrToVoid(Fn));

  Handle<Object> result(&scope, Fn(thread, previousFrame, argc));
  Handle<Object> pendingException(&scope, thread->pendingException());
  assert(result->isError() != pendingException->isNone());
  // TODO: pendingException should eventually be an instance of BaseException
  // (or a subclass), and it should be thrown up into python.
  thread->abortOnPendingException();
  thread->popFrame();
  return *result;
}

} // namespace python
