#include "runtime.h"

namespace python {

extern "C" void PyOS_AfterFork(void) { UNIMPLEMENTED("PyOS_AfterFork"); }

extern "C" PyObject* PyOS_FSPath(PyObject* /* h */) {
  UNIMPLEMENTED("PyOS_FSPath");
}

}  // namespace python
