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
template <RawObject (*Fn)(Thread*, Frame*, word)>
RawObject nativeTrampoline(Thread* thread, Frame* /*caller_frame*/, word argc) {
  HandleScope scope(thread);
  Frame* frame = thread->pushNativeFrame(bit_cast<void*>(Fn), argc);
  Object result(&scope, Fn(thread, frame, argc));
  DCHECK(result->isError() == thread->hasPendingException(),
         "error/exception mismatch");
  thread->popFrame();
  return *result;
}

template <RawObject (*Fn)(Thread*, Frame*, word)>
RawObject nativeTrampolineKw(Thread* thread, Frame* /*caller_frame*/,
                             word argc) {
  HandleScope scope(thread);
  Frame* frame = thread->pushNativeFrame(bit_cast<void*>(Fn), argc + 1);
  Object result(&scope, Fn(thread, frame, argc + 1));
  DCHECK(result->isError() == thread->hasPendingException(),
         "error/exception mismatch");
  thread->popFrame();
  return *result;
}

}  // namespace python
