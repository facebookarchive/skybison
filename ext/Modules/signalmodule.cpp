#include "runtime.h"

namespace py {

PY_EXPORT void _PySignal_AfterFork() {
  // TODO(T39596544): do nothing until we have a GIL.
}

PY_EXPORT int PyErr_CheckSignals() {
  Thread* thread = Thread::current();
  if (thread->runtime()->handlePendingSignals(thread).isErrorException()) {
    return -1;
  }
  return 0;
}

PY_EXPORT void PyErr_SetInterrupt() { UNIMPLEMENTED("PyErr_SetInterrupt"); }

PY_EXPORT void PyOS_InitInterrupts() { UNIMPLEMENTED("PyOS_InitInterrupts"); }

PY_EXPORT int PyOS_InterruptOccurred() {
  UNIMPLEMENTED("PyOS_InterruptOccurred");
}

PY_EXPORT void PyOS_AfterFork_Child() {
  // TODO(T39596544): do nothing until we have a GIL.
}

PY_EXPORT void PyOS_AfterFork_Parent() {
  // TODO(T39596544): do nothing until we have a GIL.
}

PY_EXPORT void PyOS_BeforeFork() {
  // TODO(T39596544): do nothing until we have a GIL.
}

}  // namespace py
