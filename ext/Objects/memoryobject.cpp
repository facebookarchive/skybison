#include "runtime.h"

namespace python {

extern "C" PyObject* PyMemoryView_FromMemory(char* /* m */, Py_ssize_t /* e */,
                                             int /* s */) {
  UNIMPLEMENTED("PyMemoryView_FromMemory");
}

extern "C" PyObject* PyMemoryView_FromObject(PyObject* /* v */) {
  UNIMPLEMENTED("PyMemoryView_FromObject");
}

extern "C" PyObject* PyMemoryView_GetContiguous(PyObject* /* j */, int /* e */,
                                                char /* r */) {
  UNIMPLEMENTED("PyMemoryView_GetContiguous");
}

}  // namespace python
