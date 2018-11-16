#include "runtime.h"

namespace python {

extern "C" int PyErr_CheckSignals(void) { UNIMPLEMENTED("PyErr_CheckSignals"); }

extern "C" void PyErr_SetInterrupt(void) {
  UNIMPLEMENTED("PyErr_SetInterrupt");
}

extern "C" void PyOS_InitInterrupts(void) {
  UNIMPLEMENTED("PyOS_InitInterrupts");
}

extern "C" int PyOS_InterruptOccurred(void) {
  UNIMPLEMENTED("PyOS_InterruptOccurred");
}

}  // namespace python
