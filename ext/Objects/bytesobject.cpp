#include "runtime.h"

namespace python {

extern "C" char* PyBytes_AsString(PyObject* /* p */) {
  UNIMPLEMENTED("PyBytes_AsString");
}

extern "C" PyObject* PyBytes_FromString(const char* /* r */) {
  UNIMPLEMENTED("PyBytes_FromString");
}

extern "C" PyObject* PyBytes_FromStringAndSize(const char* /* r */,
                                               Py_ssize_t /* e */) {
  UNIMPLEMENTED("PyBytes_FromStringAndSize");
}

extern "C" int PyBytes_AsStringAndSize(PyObject* /* j */, char** /* s */,
                                       Py_ssize_t* /* n */) {
  UNIMPLEMENTED("PyBytes_AsStringAndSize");
}

extern "C" void PyBytes_Concat(PyObject** /* pv */, PyObject* /* w */) {
  UNIMPLEMENTED("PyBytes_Concat");
}

extern "C" void PyBytes_ConcatAndDel(PyObject** /* pv */, PyObject* /* w */) {
  UNIMPLEMENTED("PyBytes_ConcatAndDel");
}

extern "C" PyObject* PyBytes_DecodeEscape(const char* /* s */,
                                          Py_ssize_t /* n */,
                                          const char* /* s */,
                                          Py_ssize_t /* e */,
                                          const char* /* g */) {
  UNIMPLEMENTED("PyBytes_DecodeEscape");
}

extern "C" PyObject* PyBytes_FromFormat(const char* /* t */, ...) {
  UNIMPLEMENTED("PyBytes_FromFormat");
}

extern "C" PyObject* PyBytes_FromFormatV(const char* /* t */, va_list /* s */) {
  UNIMPLEMENTED("PyBytes_FromFormatV");
}

extern "C" PyObject* PyBytes_FromObject(PyObject* /* x */) {
  UNIMPLEMENTED("PyBytes_FromObject");
}

extern "C" PyObject* PyBytes_Repr(PyObject* /* j */, int /* s */) {
  UNIMPLEMENTED("PyBytes_Repr");
}

extern "C" Py_ssize_t PyBytes_Size(PyObject* /* p */) {
  UNIMPLEMENTED("PyBytes_Size");
}

}  // namespace python
