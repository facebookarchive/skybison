#include "runtime.h"

namespace python {

PY_EXPORT int PyErr_CheckSignals() { UNIMPLEMENTED("PyErr_CheckSignals"); }

PY_EXPORT void PyErr_SetInterrupt() { UNIMPLEMENTED("PyErr_SetInterrupt"); }

PY_EXPORT void PyOS_InitInterrupts() { UNIMPLEMENTED("PyOS_InitInterrupts"); }

PY_EXPORT int PyOS_InterruptOccurred() {
  UNIMPLEMENTED("PyOS_InterruptOccurred");
}

PY_EXPORT void PyOS_AfterFork() { UNIMPLEMENTED("PyOS_AfterFork"); }

}  // namespace python
