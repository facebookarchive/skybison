#include "runtime.h"

namespace python {

struct PyCodeObject;

PY_EXPORT PyCodeObject* PyCode_New(int /* t */, int /* t */, int /* s */,
                                   int /* e */, int /* s */, PyObject* /* e */,
                                   PyObject* /* s */, PyObject* /* s */,
                                   PyObject* /* s */, PyObject* /* s */,
                                   PyObject* /* s */, PyObject* /* e */,
                                   PyObject* /* e */, int /* o */,
                                   PyObject* /* b */) {
  UNIMPLEMENTED("PyCode_New");
}

PY_EXPORT Py_ssize_t PyCode_GetNumFree_Func(PyObject*) {
  UNIMPLEMENTED("PyCode_GetNumFree_Func");
}

PY_EXPORT PyObject* PyCode_GetName_Func(PyObject*) {
  UNIMPLEMENTED("PyCode_GetName_Func");
}

PY_EXPORT PyObject* PyCode_GetFreevars_Func(PyObject*) {
  UNIMPLEMENTED("PyCode_GetVars_Func");
}

PY_EXPORT PyObject* _PyCode_ConstantKey(PyObject*) {
  UNIMPLEMENTED("_PyCode_ConstantKey");
}

}  // namespace python
