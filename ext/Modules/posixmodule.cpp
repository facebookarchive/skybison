#include "runtime.h"

namespace python {

PY_EXPORT void PyOS_AfterFork() { UNIMPLEMENTED("PyOS_AfterFork"); }

PY_EXPORT PyObject* PyOS_FSPath(PyObject* /* h */) {
  UNIMPLEMENTED("PyOS_FSPath");
}

}  // namespace python
