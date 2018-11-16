#include "runtime.h"

namespace python {

PY_EXPORT int PyByteArray_Check_Func(PyObject* /* obj */) {
  UNIMPLEMENTED("PyByteArray_Check_Func");
}

PY_EXPORT PyObject* PyByteArray_FromStringAndSize(const char* /* s */,
                                                  Py_ssize_t /* e */) {
  UNIMPLEMENTED("PyByteArray_FromStringAndSize");
}

PY_EXPORT int PyByteArray_Resize(PyObject* /* f */, Py_ssize_t /* e */) {
  UNIMPLEMENTED("PyByteArray_Resize");
}

PY_EXPORT char* PyByteArray_AsString(PyObject* /* f */) {
  UNIMPLEMENTED("PyByteArray_AsString");
}

PY_EXPORT PyObject* PyByteArray_Concat(PyObject* /* a */, PyObject* /* b */) {
  UNIMPLEMENTED("PyByteArray_Concat");
}

PY_EXPORT PyObject* PyByteArray_FromObject(PyObject* /* t */) {
  UNIMPLEMENTED("PyByteArray_FromObject");
}

PY_EXPORT Py_ssize_t PyByteArray_Size(PyObject* /* f */) {
  UNIMPLEMENTED("PyByteArray_Size");
}

}  // namespace python
