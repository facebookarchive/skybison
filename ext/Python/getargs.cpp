#include "runtime.h"

namespace python {

extern "C" int PyArg_Parse(PyObject* /* s */, const char* /* t */, ...) {
  UNIMPLEMENTED("PyArg_Parse");
}

extern "C" int PyArg_ParseTupleAndKeywords(PyObject* /* s */, PyObject* /* s */,
                                           const char* /* t */,
                                           char** /* kwlist */, ...) {
  UNIMPLEMENTED("PyArg_ParseTupleAndKeywords");
}

extern "C" int PyArg_UnpackTuple(PyObject* /* s */, const char* /* e */,
                                 Py_ssize_t /* n */, Py_ssize_t /* x */, ...) {
  UNIMPLEMENTED("PyArg_UnpackTuple");
}

extern "C" int PyArg_VaParse(PyObject* /* s */, const char* /* t */,
                             va_list /* a */) {
  UNIMPLEMENTED("PyArg_VaParse");
}

extern "C" int PyArg_VaParseTupleAndKeywords(PyObject* /* s */,
                                             PyObject* /* s */,
                                             const char* /* t */,
                                             char** /* kwlist */,
                                             va_list /* a */) {
  UNIMPLEMENTED("PyArg_VaParseTupleAndKeywords");
}

extern "C" int PyArg_ValidateKeywordArguments(PyObject* /* s */) {
  UNIMPLEMENTED("PyArg_ValidateKeywordArguments");
}

extern "C" int _PyArg_ParseTupleAndKeywords_SizeT(PyObject* /* s */,
                                                  PyObject* /* s */,
                                                  const char* /* t */,
                                                  char** /* kwlist */, ...) {
  UNIMPLEMENTED("_PyArg_ParseTupleAndKeywords_SizeT");
}

extern "C" int _PyArg_ParseTuple_SizeT(PyObject* /* s */, const char* /* t */,
                                       ...) {
  UNIMPLEMENTED("_PyArg_ParseTuple_SizeT");
}

extern "C" int _PyArg_Parse_SizeT(PyObject* /* s */, const char* /* t */, ...) {
  UNIMPLEMENTED("_PyArg_Parse_SizeT");
}

extern "C" int _PyArg_VaParseTupleAndKeywords_SizeT(PyObject* /* s */,
                                                    PyObject* /* s */,
                                                    const char* /* t */,
                                                    char** /* kwlist */,
                                                    va_list /* a */) {
  UNIMPLEMENTED("_PyArg_VaParseTupleAndKeywords_SizeT");
}

extern "C" int _PyArg_VaParse_SizeT(PyObject* /* s */, const char* /* t */,
                                    va_list /* a */) {
  UNIMPLEMENTED("_PyArg_VaParse_SizeT");
}

}  // namespace python
