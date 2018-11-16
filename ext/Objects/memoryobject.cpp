#include "runtime.h"

namespace python {

PY_EXPORT PyObject* PyMemoryView_FromMemory(char* /* m */, Py_ssize_t /* e */,
                                            int /* s */) {
  UNIMPLEMENTED("PyMemoryView_FromMemory");
}

PY_EXPORT PyObject* PyMemoryView_FromObject(PyObject* /* v */) {
  UNIMPLEMENTED("PyMemoryView_FromObject");
}

PY_EXPORT PyObject* PyMemoryView_GetContiguous(PyObject* /* j */, int /* e */,
                                               char /* r */) {
  UNIMPLEMENTED("PyMemoryView_GetContiguous");
}

}  // namespace python
