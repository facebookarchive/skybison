#include "runtime.h"

namespace python {

PY_EXPORT int PySlice_GetIndices(PyObject* /* r */, Py_ssize_t /* h */,
                                 Py_ssize_t* /* t */, Py_ssize_t* /* p */,
                                 Py_ssize_t* /* p */) {
  UNIMPLEMENTED("PySlice_GetIndices");
}

PY_EXPORT PyObject* PySlice_New(PyObject* /* t */, PyObject* /* p */,
                                PyObject* /* p */) {
  UNIMPLEMENTED("PySlice_New");
}

PY_EXPORT Py_ssize_t PySlice_AdjustIndices(Py_ssize_t /* h */,
                                           Py_ssize_t* /* t */,
                                           Py_ssize_t* /* p */,
                                           Py_ssize_t /* p */) {
  UNIMPLEMENTED("PySlice_AdjustIndices");
}

PY_EXPORT int PySlice_GetIndicesEx(PyObject* /* r */, Py_ssize_t /* h */,
                                   Py_ssize_t* /* t */, Py_ssize_t* /* p */,
                                   Py_ssize_t* /* p */, Py_ssize_t* /* h */) {
  UNIMPLEMENTED("PySlice_GetIndicesEx");
}

PY_EXPORT int PySlice_Unpack(PyObject* /* r */, Py_ssize_t* /* t */,
                             Py_ssize_t* /* p */, Py_ssize_t* /* p */) {
  UNIMPLEMENTED("PySlice_Unpack");
}

}  // namespace python
