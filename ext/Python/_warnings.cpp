// _warnings.c implementation

#include "cpython-types.h"
#include "utils.h"

namespace python {

PY_EXPORT int PyErr_WarnEx(PyObject*, const char*, Py_ssize_t) {
  UNIMPLEMENTED("PyErr_WarnEx");
}

PY_EXPORT int PyErr_ResourceWarning(PyObject* /* e */, Py_ssize_t /* l */,
                                    const char* /* t */, ...) {
  UNIMPLEMENTED("PyErr_ResourceWarning");
}

PY_EXPORT int PyErr_WarnExplicit(PyObject* /* y */, const char* /* t */,
                                 const char* /* r */, int /* o */,
                                 const char* /* r */, PyObject* /* y */) {
  UNIMPLEMENTED("PyErr_WarnExplicit");
}

PY_EXPORT int PyErr_WarnFormat(PyObject* /* y */, Py_ssize_t /* l */,
                               const char* /* t */, ...) {
  UNIMPLEMENTED("PyErr_WarnFormat");
}

}  // namespace python
