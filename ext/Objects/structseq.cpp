#include "runtime.h"

namespace python {

PY_EXPORT int PyStructSequence_InitType2(PyTypeObject* /* e */,
                                         PyStructSequence_Desc* /* c */) {
  UNIMPLEMENTED("PyStructSequence_InitType2");
}

PY_EXPORT PyObject* PyStructSequence_GetItem(PyObject* /* p */,
                                             Py_ssize_t /* i */) {
  UNIMPLEMENTED("PyStructSequence_GetItem");
}

PY_EXPORT PyObject* PyStructSequence_New(PyTypeObject* /* e */) {
  UNIMPLEMENTED("PyStructSequence_New");
}

PY_EXPORT PyTypeObject* PyStructSequence_NewType(
    PyStructSequence_Desc* /* c */) {
  UNIMPLEMENTED("PyStructSequence_NewType");
}

PY_EXPORT void PyStructSequence_SetItem(PyObject* /* p */, Py_ssize_t /* i */,
                                        PyObject* /* v */) {
  UNIMPLEMENTED("PyStructSequence_SetItem");
}

}  // namespace python
