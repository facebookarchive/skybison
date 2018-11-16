#include "runtime.h"

namespace python {

extern "C" PyObject* PyClassMethod_New(PyObject* /* e */) {
  UNIMPLEMENTED("PyClassMethod_New");
}

extern "C" PyObject* PyStaticMethod_New(PyObject* /* e */) {
  UNIMPLEMENTED("PyStaticMethod_New");
}

extern "C" PyObject* _PyCFunction_FastCallDict(PyObject* /* c */,
                                               PyObject* const* /* s */,
                                               Py_ssize_t /* s */,
                                               PyObject* /* s */) {
  UNIMPLEMENTED("_PyCFunction_FastCallDict");
}

extern "C" PyObject* _PyCFunction_FastCallKeywords(PyObject* /* c */,
                                                   PyObject* const* /* s */,
                                                   Py_ssize_t /* s */,
                                                   PyObject* /* s */) {
  UNIMPLEMENTED("_PyCFunction_FastCallKeywords");
}

extern "C" PyObject* _PyFunction_FastCallDict(PyObject* /* c */,
                                              PyObject* const* /* s */,
                                              Py_ssize_t /* s */,
                                              PyObject* /* s */) {
  UNIMPLEMENTED("_PyFunction_FastCallDict");
}

extern "C" PyObject* _PyFunction_FastCallKeywords(PyObject* /* c */,
                                                  PyObject* const* /* k */,
                                                  Py_ssize_t /* s */,
                                                  PyObject* /* s */) {
  UNIMPLEMENTED("_PyFunction_FastCallKeywords");
}

}  // namespace python
