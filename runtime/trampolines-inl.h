#pragma once

#include "objects.h"
#include "trampolines.h"

#include <cstdio>

namespace python {

// Template for entry points for native functions (e.g. builtins), invoked via
// CALL_FUNCTION
// TODO(t24656189) - replace with JITed code once we have the facilities for
// that.
template <Object* (*Fn)(Thread*, Frame*, word)>
Object* nativeTrampoline(Thread* thread, Frame* previousFrame, word argc) {
  HandleScope scope;
  Handle<Object> result(&scope, Fn(thread, previousFrame, argc));
  Handle<Object> pendingException(&scope, thread->pendingException());
  assert(result->isError() != pendingException->isNone());
  if (result->isError()) {
    // TODO: pendingException should eventually be an instance of BaseException
    // (or a subclass), and it should be thrown up into python.
    fprintf(stderr, "aborting due to pending exception");
    if (pendingException->isLargeString()) {
      // TODO handle small string's too
      LargeString* s = LargeString::cast(*pendingException);
      fprintf(
          stderr,
          ": %.*s",
          static_cast<int>(s->length()),
          reinterpret_cast<const char*>(s->address()));
    }
    fprintf(stderr, "\n");

    std::abort();
  }
  return *result;
}

} // namespace python
