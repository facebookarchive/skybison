#include "runtime.h"

namespace python {

PY_EXPORT char* PyBytes_AsString(PyObject* /* p */) {
  UNIMPLEMENTED("PyBytes_AsString");
}

PY_EXPORT PyObject* PyBytes_FromString(const char* /* r */) {
  UNIMPLEMENTED("PyBytes_FromString");
}

PY_EXPORT PyObject* PyBytes_FromStringAndSize(const char* /* r */,
                                              Py_ssize_t /* e */) {
  UNIMPLEMENTED("PyBytes_FromStringAndSize");
}

PY_EXPORT int PyBytes_AsStringAndSize(PyObject* /* j */, char** /* s */,
                                      Py_ssize_t* /* n */) {
  UNIMPLEMENTED("PyBytes_AsStringAndSize");
}

PY_EXPORT void PyBytes_Concat(PyObject** /* pv */, PyObject* /* w */) {
  UNIMPLEMENTED("PyBytes_Concat");
}

PY_EXPORT void PyBytes_ConcatAndDel(PyObject** /* pv */, PyObject* /* w */) {
  UNIMPLEMENTED("PyBytes_ConcatAndDel");
}

PY_EXPORT PyObject* PyBytes_DecodeEscape(const char* /* s */,
                                         Py_ssize_t /* n */,
                                         const char* /* s */,
                                         Py_ssize_t /* e */,
                                         const char* /* g */) {
  UNIMPLEMENTED("PyBytes_DecodeEscape");
}

PY_EXPORT PyObject* PyBytes_FromFormat(const char* /* t */, ...) {
  UNIMPLEMENTED("PyBytes_FromFormat");
}

PY_EXPORT PyObject* PyBytes_FromFormatV(const char* /* t */, va_list /* s */) {
  UNIMPLEMENTED("PyBytes_FromFormatV");
}

PY_EXPORT PyObject* PyBytes_FromObject(PyObject* /* x */) {
  UNIMPLEMENTED("PyBytes_FromObject");
}

PY_EXPORT PyObject* PyBytes_Repr(PyObject* /* j */, int /* s */) {
  UNIMPLEMENTED("PyBytes_Repr");
}

PY_EXPORT Py_ssize_t PyBytes_Size(PyObject* /* p */) {
  UNIMPLEMENTED("PyBytes_Size");
}

}  // namespace python
