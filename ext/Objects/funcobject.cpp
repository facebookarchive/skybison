#include "runtime.h"

namespace python {

PY_EXPORT PyObject* PyClassMethod_New(PyObject* /* e */) {
  UNIMPLEMENTED("PyClassMethod_New");
}

PY_EXPORT PyObject* PyStaticMethod_New(PyObject* /* e */) {
  UNIMPLEMENTED("PyStaticMethod_New");
}

PY_EXPORT PyObject* _PyCFunction_FastCallDict(PyObject* /* c */,
                                              PyObject* const* /* s */,
                                              Py_ssize_t /* s */,
                                              PyObject* /* s */) {
  UNIMPLEMENTED("_PyCFunction_FastCallDict");
}

PY_EXPORT PyObject* _PyCFunction_FastCallKeywords(PyObject* /* c */,
                                                  PyObject* const* /* s */,
                                                  Py_ssize_t /* s */,
                                                  PyObject* /* s */) {
  UNIMPLEMENTED("_PyCFunction_FastCallKeywords");
}

PY_EXPORT PyObject* _PyFunction_FastCallDict(PyObject* /* c */,
                                             PyObject* const* /* s */,
                                             Py_ssize_t /* s */,
                                             PyObject* /* s */) {
  UNIMPLEMENTED("_PyFunction_FastCallDict");
}

PY_EXPORT PyObject* _PyFunction_FastCallKeywords(PyObject* /* c */,
                                                 PyObject* const* /* k */,
                                                 Py_ssize_t /* s */,
                                                 PyObject* /* s */) {
  UNIMPLEMENTED("_PyFunction_FastCallKeywords");
}

}  // namespace python
