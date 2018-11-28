#include "runtime.h"

namespace python {

PY_EXPORT int PyBytes_CheckExact_Func(PyObject* obj) {
  return ApiHandle::fromPyObject(obj)->asObject()->isBytes();
}

PY_EXPORT int PyBytes_Check_Func(PyObject* obj) {
  if (PyBytes_CheckExact_Func(obj)) {
    return true;
  }
  return ApiHandle::fromPyObject(obj)->isSubclass(Thread::currentThread(),
                                                  LayoutId::kBytes);
}

PY_EXPORT char* PyBytes_AsString(PyObject* /* p */) {
  UNIMPLEMENTED("PyBytes_AsString");
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

PY_EXPORT PyObject* PyBytes_FromStringAndSize(const char* str,
                                              Py_ssize_t size) {
  Thread* thread = Thread::currentThread();

  if (size < 0) {
    thread->raiseSystemErrorWithCStr(
        "Negative size passed to PyBytes_FromStringAndSize");
    return nullptr;
  }

  if (str == nullptr) {
    UNIMPLEMENTED("mutable, uninitialized bytes");
  }

  // TODO(T36997048): intern 1-element byte arrays

  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  View<byte> view(reinterpret_cast<const byte*>(str), size);
  Object bytes_obj(&scope, runtime->newBytesWithAll(view));

  return ApiHandle::fromObject(*bytes_obj);
}

PY_EXPORT PyObject* PyBytes_FromString(const char* str) {
  DCHECK(str, "nullptr argument");
  Thread* thread = Thread::currentThread();

  uword size = strlen(str);
  if (size > kMaxWord) {
    thread->raiseOverflowErrorWithCStr("byte string is too large");
    return nullptr;
  }

  return PyBytes_FromStringAndSize(str, static_cast<word>(size));
}

PY_EXPORT PyObject* PyBytes_Repr(PyObject* /* j */, int /* s */) {
  UNIMPLEMENTED("PyBytes_Repr");
}

PY_EXPORT Py_ssize_t PyBytes_Size(PyObject* /* p */) {
  UNIMPLEMENTED("PyBytes_Size");
}

}  // namespace python
