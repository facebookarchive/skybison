#include "runtime.h"

namespace python {

struct PyStructSequence_Desc;

extern "C" int PyStructSequence_InitType2(PyTypeObject* /* e */,
                                          PyStructSequence_Desc* /* c */) {
  UNIMPLEMENTED("PyStructSequence_InitType2");
}

extern "C" PyObject* PyStructSequence_GetItem(PyObject* /* p */,
                                              Py_ssize_t /* i */) {
  UNIMPLEMENTED("PyStructSequence_GetItem");
}

extern "C" PyObject* PyStructSequence_New(PyTypeObject* /* e */) {
  UNIMPLEMENTED("PyStructSequence_New");
}

extern "C" PyTypeObject* PyStructSequence_NewType(
    PyStructSequence_Desc* /* c */) {
  UNIMPLEMENTED("PyStructSequence_NewType");
}

extern "C" void PyStructSequence_SetItem(PyObject* /* p */, Py_ssize_t /* i */,
                                         PyObject* /* v */) {
  UNIMPLEMENTED("PyStructSequence_SetItem");
}

}  // namespace python
