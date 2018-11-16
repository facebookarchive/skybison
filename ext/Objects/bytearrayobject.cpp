#include "cpython-data.h"
#include "runtime.h"

namespace python {

extern "C" char* PyByteArray_AS_STRING_Func(PyObject* /* self */) {
  UNIMPLEMENTED("PyByteArray_AS_STRING_Func");
}

extern "C" int PyByteArray_Check_Func(PyObject* /* obj */) {
  UNIMPLEMENTED("PyByteArray_Check_Func");
}

extern "C" PyObject* PyByteArray_FromStringAndSize(const char* /* s */,
                                                   Py_ssize_t /* e */) {
  UNIMPLEMENTED("PyByteArray_FromStringAndSize");
}

extern "C" int PyByteArray_Resize(PyObject* /* f */, Py_ssize_t /* e */) {
  UNIMPLEMENTED("PyByteArray_Resize");
}

extern "C" char* PyByteArray_AsString(PyObject* /* f */) {
  UNIMPLEMENTED("PyByteArray_AsString");
}

extern "C" PyObject* PyByteArray_Concat(PyObject* /* a */, PyObject* /* b */) {
  UNIMPLEMENTED("PyByteArray_Concat");
}

extern "C" PyObject* PyByteArray_FromObject(PyObject* /* t */) {
  UNIMPLEMENTED("PyByteArray_FromObject");
}

extern "C" Py_ssize_t PyByteArray_Size(PyObject* /* f */) {
  UNIMPLEMENTED("PyByteArray_Size");
}

}  // namespace python
