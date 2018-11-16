#include "runtime.h"

namespace python {

PY_EXPORT int PyArg_Parse(PyObject* /* s */, const char* /* t */, ...) {
  UNIMPLEMENTED("PyArg_Parse");
}

PY_EXPORT int PyArg_ParseTupleAndKeywords(PyObject* /* s */, PyObject* /* s */,
                                          const char* /* t */,
                                          char** /* kwlist */, ...) {
  UNIMPLEMENTED("PyArg_ParseTupleAndKeywords");
}

PY_EXPORT int PyArg_UnpackTuple(PyObject* /* s */, const char* /* e */,
                                Py_ssize_t /* n */, Py_ssize_t /* x */, ...) {
  UNIMPLEMENTED("PyArg_UnpackTuple");
}

PY_EXPORT int PyArg_VaParse(PyObject* /* s */, const char* /* t */,
                            va_list /* a */) {
  UNIMPLEMENTED("PyArg_VaParse");
}

PY_EXPORT int PyArg_VaParseTupleAndKeywords(PyObject* /* s */,
                                            PyObject* /* s */,
                                            const char* /* t */,
                                            char** /* kwlist */,
                                            va_list /* a */) {
  UNIMPLEMENTED("PyArg_VaParseTupleAndKeywords");
}

PY_EXPORT int PyArg_ValidateKeywordArguments(PyObject* /* s */) {
  UNIMPLEMENTED("PyArg_ValidateKeywordArguments");
}

PY_EXPORT int _PyArg_ParseTupleAndKeywords_SizeT(PyObject* /* s */,
                                                 PyObject* /* s */,
                                                 const char* /* t */,
                                                 char** /* kwlist */, ...) {
  UNIMPLEMENTED("_PyArg_ParseTupleAndKeywords_SizeT");
}

PY_EXPORT int _PyArg_ParseTuple_SizeT(PyObject* /* s */, const char* /* t */,
                                      ...) {
  UNIMPLEMENTED("_PyArg_ParseTuple_SizeT");
}

PY_EXPORT int _PyArg_Parse_SizeT(PyObject* /* s */, const char* /* t */, ...) {
  UNIMPLEMENTED("_PyArg_Parse_SizeT");
}

PY_EXPORT int _PyArg_VaParseTupleAndKeywords_SizeT(PyObject* /* s */,
                                                   PyObject* /* s */,
                                                   const char* /* t */,
                                                   char** /* kwlist */,
                                                   va_list /* a */) {
  UNIMPLEMENTED("_PyArg_VaParseTupleAndKeywords_SizeT");
}

PY_EXPORT int _PyArg_VaParse_SizeT(PyObject* /* s */, const char* /* t */,
                                   va_list /* a */) {
  UNIMPLEMENTED("_PyArg_VaParse_SizeT");
}

}  // namespace python
