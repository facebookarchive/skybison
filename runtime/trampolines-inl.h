#pragma once

#include "objects.h"
#include "thread.h"
#include "trampolines.h"

#include "utils.h"

namespace python {

// Template for entry points for native functions (e.g. builtins), invoked via
// CALL_FUNCTION
// TODO(T24656189) - replace with JITed code once we have the facilities for
// that.
template <Function::Entry Fn>
RawObject nativeTrampoline(Thread* thread, Frame* /*caller_frame*/, word argc) {
  HandleScope scope(thread);
  Frame* frame = thread->pushNativeFrame(bit_cast<void*>(Fn), argc);
  Object result(&scope, Fn(thread, frame, argc));
  DCHECK(result->isError() == thread->hasPendingException(),
         "error/exception mismatch");
  thread->popFrame();
  return *result;
}

template <Function::Entry Fn>
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

// Template for entry points for native functions (e.g. builtins), invoked via
// CALL_FUNCTION, whose positional arguments are checked before entering the
// body.
// TODO(T24656189) - replace with JITed code once we have the facilities for
// that.
// TODO(T39316450): Kill this in favor of storing the fn pointer in the
// Function->Code->code
template <Function::Entry Fn>
RawObject builtinTrampolineWrapper(Thread* thread, Frame* caller, word argc) {
  return builtinTrampoline(thread, caller, argc, Fn);
}

template <Function::Entry Fn>
RawObject builtinTrampolineWrapperKw(Thread* thread, Frame* caller, word argc) {
  return builtinTrampolineKw(thread, caller, argc, Fn);
}

}  // namespace python
