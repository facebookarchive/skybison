#pragma once

#include "objects.h"
#include "thread.h"
#include "trampolines.h"

#include "utils.h"

namespace python {

// Template for entry points for native functions (e.g. builtins), invoked via
// CALL_FUNCTION
// TODO(t24656189) - replace with JITed code once we have the facilities for
// that.
template <Object* (*Fn)(Thread*, Frame*, word)>
Object* nativeTrampoline(Thread* thread, Frame* /*caller_frame*/, word argc) {
  HandleScope scope(thread);
  Frame* frame = thread->pushNativeFrame(Utils::castFnPtrToVoid(Fn), argc);
  Handle<Object> result(&scope, Fn(thread, frame, argc));
  DCHECK(result->isError() == thread->hasPendingException(),
         "error/exception mismatch");
  thread->abortOnPendingException();
  thread->popFrame();
  return *result;
}

template <Object* (*Fn)(Thread*, Frame*, word)>
Object* nativeTrampolineKw(Thread* thread, Frame* /*caller_frame*/, word argc) {
  HandleScope scope(thread);
  Frame* frame = thread->pushNativeFrame(Utils::castFnPtrToVoid(Fn), argc + 1);
  Handle<Object> result(&scope, Fn(thread, frame, argc + 1));
  DCHECK(result->isError() == thread->hasPendingException(),
         "error/exception mismatch");
  thread->abortOnPendingException();
  thread->popFrame();
  return *result;
}

}  // namespace python
