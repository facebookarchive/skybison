#include "runtime.h"

namespace python {

PY_EXPORT int PyErr_CheckSignals() {
  // TODO(T43363720): Check thread for pending signal
  return 0;
}

PY_EXPORT void PyErr_SetInterrupt() { UNIMPLEMENTED("PyErr_SetInterrupt"); }

PY_EXPORT void PyOS_InitInterrupts() { UNIMPLEMENTED("PyOS_InitInterrupts"); }

PY_EXPORT int PyOS_InterruptOccurred() {
  UNIMPLEMENTED("PyOS_InterruptOccurred");
}

PY_EXPORT void PyOS_AfterFork() {
  // TODO(T39596544): do nothing until we have a GIL.
}

}  // namespace python
