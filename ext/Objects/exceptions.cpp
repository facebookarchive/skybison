#include "runtime.h"

namespace python {

extern "C" PyObject* PyException_GetTraceback(PyObject* /* f */) {
  UNIMPLEMENTED("PyException_GetTraceback");
}

extern "C" void PyException_SetCause(PyObject* /* f */, PyObject* /* e */) {
  UNIMPLEMENTED("PyException_SetCause");
}

extern "C" PyObject* PyException_GetCause(PyObject* /* f */) {
  UNIMPLEMENTED("PyException_GetCause");
}

extern "C" PyObject* PyException_GetContext(PyObject* /* f */) {
  UNIMPLEMENTED("PyException_GetContext");
}

extern "C" void PyException_SetContext(PyObject* /* f */, PyObject* /* t */) {
  UNIMPLEMENTED("PyException_SetContext");
}

extern "C" int PyException_SetTraceback(PyObject* /* f */, PyObject* /* b */) {
  UNIMPLEMENTED("PyException_SetTraceback");
}

extern "C" PyObject* PyUnicodeDecodeError_Create(
    const char* /* g */, const char* /* t */, Py_ssize_t /* h */,
    Py_ssize_t /* t */, Py_ssize_t /* d */, const char* /* n */) {
  UNIMPLEMENTED("PyUnicodeDecodeError_Create");
}

extern "C" PyObject* PyUnicodeDecodeError_GetEncoding(PyObject* /* c */) {
  UNIMPLEMENTED("PyUnicodeDecodeError_GetEncoding");
}

extern "C" int PyUnicodeDecodeError_GetEnd(PyObject* /* c */,
                                           Py_ssize_t* /* d */) {
  UNIMPLEMENTED("PyUnicodeDecodeError_GetEnd");
}

extern "C" PyObject* PyUnicodeDecodeError_GetObject(PyObject* /* c */) {
  UNIMPLEMENTED("PyUnicodeDecodeError_GetObject");
}

extern "C" PyObject* PyUnicodeDecodeError_GetReason(PyObject* /* c */) {
  UNIMPLEMENTED("PyUnicodeDecodeError_GetReason");
}

extern "C" int PyUnicodeDecodeError_GetStart(PyObject* /* c */,
                                             Py_ssize_t* /* t */) {
  UNIMPLEMENTED("PyUnicodeDecodeError_GetStart");
}

extern "C" int PyUnicodeDecodeError_SetEnd(PyObject* /* c */,
                                           Py_ssize_t /* d */) {
  UNIMPLEMENTED("PyUnicodeDecodeError_SetEnd");
}

extern "C" int PyUnicodeDecodeError_SetReason(PyObject* /* c */,
                                              const char* /* n */) {
  UNIMPLEMENTED("PyUnicodeDecodeError_SetReason");
}

extern "C" int PyUnicodeDecodeError_SetStart(PyObject* /* c */,
                                             Py_ssize_t /* t */) {
  UNIMPLEMENTED("PyUnicodeDecodeError_SetStart");
}

extern "C" PyObject* PyUnicodeEncodeError_GetEncoding(PyObject* /* c */) {
  UNIMPLEMENTED("PyUnicodeEncodeError_GetEncoding");
}

extern "C" int PyUnicodeEncodeError_GetEnd(PyObject* /* c */,
                                           Py_ssize_t* /* d */) {
  UNIMPLEMENTED("PyUnicodeEncodeError_GetEnd");
}

extern "C" PyObject* PyUnicodeEncodeError_GetObject(PyObject* /* c */) {
  UNIMPLEMENTED("PyUnicodeEncodeError_GetObject");
}

extern "C" PyObject* PyUnicodeEncodeError_GetReason(PyObject* /* c */) {
  UNIMPLEMENTED("PyUnicodeEncodeError_GetReason");
}

extern "C" int PyUnicodeEncodeError_GetStart(PyObject* /* c */,
                                             Py_ssize_t* /* t */) {
  UNIMPLEMENTED("PyUnicodeEncodeError_GetStart");
}

extern "C" int PyUnicodeEncodeError_SetEnd(PyObject* /* c */,
                                           Py_ssize_t /* d */) {
  UNIMPLEMENTED("PyUnicodeEncodeError_SetEnd");
}

extern "C" int PyUnicodeEncodeError_SetReason(PyObject* /* c */,
                                              const char* /* n */) {
  UNIMPLEMENTED("PyUnicodeEncodeError_SetReason");
}

extern "C" int PyUnicodeEncodeError_SetStart(PyObject* /* c */,
                                             Py_ssize_t /* t */) {
  UNIMPLEMENTED("PyUnicodeEncodeError_SetStart");
}

extern "C" int PyUnicodeTranslateError_GetEnd(PyObject* /* c */,
                                              Py_ssize_t* /* t */) {
  UNIMPLEMENTED("PyUnicodeTranslateError_GetEnd");
}

extern "C" PyObject* PyUnicodeTranslateError_GetObject(PyObject* /* c */) {
  UNIMPLEMENTED("PyUnicodeTranslateError_GetObject");
}

extern "C" PyObject* PyUnicodeTranslateError_GetReason(PyObject* /* c */) {
  UNIMPLEMENTED("PyUnicodeTranslateError_GetReason");
}

extern "C" int PyUnicodeTranslateError_GetStart(PyObject* /* c */,
                                                Py_ssize_t* /* t */) {
  UNIMPLEMENTED("PyUnicodeTranslateError_GetStart");
}

extern "C" int PyUnicodeTranslateError_SetEnd(PyObject* /* c */,
                                              Py_ssize_t /* d */) {
  UNIMPLEMENTED("PyUnicodeTranslateError_SetEnd");
}

extern "C" int PyUnicodeTranslateError_SetReason(PyObject* /* c */,
                                                 const char* /* n */) {
  UNIMPLEMENTED("PyUnicodeTranslateError_SetReason");
}

extern "C" int PyUnicodeTranslateError_SetStart(PyObject* /* c */,
                                                Py_ssize_t /* t */) {
  UNIMPLEMENTED("PyUnicodeTranslateError_SetStart");
}

}  // namespace python
