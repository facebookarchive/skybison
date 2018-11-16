#include "runtime.h"

namespace python {

PY_EXPORT int PyErr_CheckSignals(void) { UNIMPLEMENTED("PyErr_CheckSignals"); }

PY_EXPORT void PyErr_SetInterrupt(void) { UNIMPLEMENTED("PyErr_SetInterrupt"); }

PY_EXPORT void PyOS_InitInterrupts(void) {
  UNIMPLEMENTED("PyOS_InitInterrupts");
}

PY_EXPORT int PyOS_InterruptOccurred(void) {
  UNIMPLEMENTED("PyOS_InterruptOccurred");
}

}  // namespace python
