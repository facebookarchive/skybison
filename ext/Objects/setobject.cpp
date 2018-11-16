#include "runtime.h"

namespace python {

extern "C" PyObject* PyFrozenSet_New(PyObject* /* e */) {
  UNIMPLEMENTED("PyFrozenSet_New");
}

extern "C" int PySet_Add(PyObject* /* t */, PyObject* /* y */) {
  UNIMPLEMENTED("PySet_Add");
}

extern "C" int PySet_Clear(PyObject* /* t */) { UNIMPLEMENTED("PySet_Clear"); }

extern "C" int PySet_Contains(PyObject* /* t */, PyObject* /* y */) {
  UNIMPLEMENTED("PySet_Contains");
}

extern "C" int PySet_Discard(PyObject* /* t */, PyObject* /* y */) {
  UNIMPLEMENTED("PySet_Discard");
}

extern "C" PyObject* PySet_New(PyObject* /* e */) {
  UNIMPLEMENTED("PySet_New");
}

extern "C" PyObject* PySet_Pop(PyObject* /* t */) {
  UNIMPLEMENTED("PySet_Pop");
}

extern "C" Py_ssize_t PySet_Size(PyObject* /* t */) {
  UNIMPLEMENTED("PySet_Size");
}

}  // namespace python
