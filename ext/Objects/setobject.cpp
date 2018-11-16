#include "runtime.h"

namespace python {

PY_EXPORT PyObject* PyFrozenSet_New(PyObject* /* e */) {
  UNIMPLEMENTED("PyFrozenSet_New");
}

PY_EXPORT int PySet_Add(PyObject* /* t */, PyObject* /* y */) {
  UNIMPLEMENTED("PySet_Add");
}

PY_EXPORT int PySet_Clear(PyObject* /* t */) { UNIMPLEMENTED("PySet_Clear"); }

PY_EXPORT int PySet_ClearFreeList() { return 0; }

PY_EXPORT int PySet_Contains(PyObject* /* t */, PyObject* /* y */) {
  UNIMPLEMENTED("PySet_Contains");
}

PY_EXPORT int PySet_Discard(PyObject* /* t */, PyObject* /* y */) {
  UNIMPLEMENTED("PySet_Discard");
}

PY_EXPORT PyObject* PySet_New(PyObject* /* e */) { UNIMPLEMENTED("PySet_New"); }

PY_EXPORT PyObject* PySet_Pop(PyObject* /* t */) { UNIMPLEMENTED("PySet_Pop"); }

PY_EXPORT Py_ssize_t PySet_Size(PyObject* /* t */) {
  UNIMPLEMENTED("PySet_Size");
}

}  // namespace python
