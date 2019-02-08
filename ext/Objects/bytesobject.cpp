#include "bytes-builtins.h"
#include "cpython-data.h"
#include "cpython-func.h"
#include "runtime.h"
#include "utils.h"

namespace python {

PY_EXPORT int PyBytes_CheckExact_Func(PyObject* obj) {
  return ApiHandle::fromPyObject(obj)->asObject()->isBytes();
}

PY_EXPORT int PyBytes_Check_Func(PyObject* obj) {
  return Thread::currentThread()->runtime()->isInstanceOfBytes(
      ApiHandle::fromPyObject(obj)->asObject());
}

PY_EXPORT char* PyBytes_AsString(PyObject* pyobj) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  ApiHandle* handle = ApiHandle::fromPyObject(pyobj);
  Object obj(&scope, handle->asObject());
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfBytes(*obj)) {
    thread->raiseBadArgument();
    return nullptr;
  }
  if (void* cache = handle->cache()) return static_cast<char*>(cache);
  Bytes bytes(&scope, *obj);
  word len = bytes.length();
  auto cache = static_cast<byte*>(std::malloc(len + 1));
  bytes.copyTo(cache, len);
  cache[len] = '\0';
  handle->setCache(cache);
  return reinterpret_cast<char*>(cache);
}

PY_EXPORT int PyBytes_AsStringAndSize(PyObject* pybytes, char** buffer,
                                      Py_ssize_t* length) {
  if (buffer == nullptr) {
    PyErr_BadInternalCall();
    return -1;
  }
  char* str = PyBytes_AsString(pybytes);
  if (str == nullptr) return -1;
  Py_ssize_t len = PyBytes_Size(pybytes);
  if (length != nullptr) {
    *length = len;
  } else if (std::strlen(str) != static_cast<size_t>(len)) {
    PyErr_SetString(PyExc_ValueError, "embedded null byte");
    return -1;
  }
  *buffer = str;
  return 0;
}

PY_EXPORT void PyBytes_Concat(PyObject** pyobj, PyObject* newpart) {
  CHECK(pyobj != nullptr, "reference to bytes object must be non-null");
  if (*pyobj == nullptr) return;
  if (newpart == nullptr) {
    PyObject* tmp = *pyobj;
    *pyobj = nullptr;
    Py_DECREF(tmp);
    return;
  }

  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  ApiHandle* obj_handle = ApiHandle::fromPyObject(*pyobj);
  Object obj(&scope, obj_handle->asObject());
  Object newpart_obj(&scope, ApiHandle::fromPyObject(newpart)->asObject());
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfBytes(*obj) ||
      !runtime->isInstanceOfBytes(*newpart_obj)) {
    thread->raiseBadArgument();
    *pyobj = nullptr;
    obj_handle->decref();
    return;
  }

  Bytes self(&scope, *obj);
  Bytes other(&scope, *newpart_obj);
  Bytes result(&scope, runtime->bytesConcat(thread, self, other));
  *pyobj = ApiHandle::newReference(thread, *result);
  obj_handle->decref();
}

PY_EXPORT void PyBytes_ConcatAndDel(PyObject** pyobj, PyObject* newpart) {
  PyBytes_Concat(pyobj, newpart);
  Py_XDECREF(newpart);
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

PY_EXPORT PyObject* PyBytes_FromObject(PyObject* pyobj) {
  Thread* thread = Thread::currentThread();
  if (pyobj == nullptr) {
    thread->raiseBadInternalCall();
    return nullptr;
  }

  HandleScope scope(thread);
  ApiHandle* handle = ApiHandle::fromPyObject(pyobj);
  Object obj(&scope, handle->asObject());
  if (obj.isBytes()) {
    handle->incref();
    return pyobj;
  }

  Object result(&scope, bytesFromIterable(thread, obj));
  if (result.isError()) return nullptr;
  return ApiHandle::newReference(thread, *result);
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

  return ApiHandle::newReference(thread, *bytes_obj);
}

PY_EXPORT PyObject* PyBytes_FromString(const char* str) {
  DCHECK(str, "nullptr argument");
  uword size = std::strlen(str);
  if (size > kMaxWord) {
    PyErr_SetString(PyExc_OverflowError, "byte string is too large");
    return nullptr;
  }

  return PyBytes_FromStringAndSize(str, static_cast<Py_ssize_t>(size));
}

PY_EXPORT PyObject* PyBytes_Repr(PyObject* /* j */, int /* s */) {
  UNIMPLEMENTED("PyBytes_Repr");
}

PY_EXPORT Py_ssize_t PyBytes_Size(PyObject* obj) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Object bytes_obj(&scope, ApiHandle::fromPyObject(obj)->asObject());

  if (!runtime->isInstanceOfBytes(*bytes_obj)) {
    thread->raiseTypeErrorWithCStr("PyBytes_Size expected bytes");
    return -1;
  }

  // TODO(wmeehan): handle delegated subtype of bytes

  Bytes bytes(&scope, *bytes_obj);
  return bytes.length();
}

PY_EXPORT int _PyBytes_Resize(PyObject** pyobj, Py_ssize_t newsize) {
  DCHECK(pyobj != nullptr, "_PyBytes_Resize given null argument");
  DCHECK(*pyobj != nullptr, "_PyBytes_Resize given pointer to null");
  Thread* thread = Thread::currentThread();
  ApiHandle* handle = ApiHandle::fromPyObject(*pyobj);
  HandleScope scope(thread);
  Object obj(&scope, handle->asObject());
  Runtime* runtime = thread->runtime();
  if (newsize < 0 || !runtime->isInstanceOfBytes(*obj)) {
    *pyobj = nullptr;
    handle->decref();
    thread->raiseBadInternalCall();
    return -1;
  }
  Bytes bytes(&scope, *obj);
  if (bytes.length() == newsize) return 0;
  // we don't check here that Py_REFCNT(*pyobj) == 1
  Bytes resized(&scope, runtime->newBytes(newsize, 0));
  word min_size = Utils::minimum(bytes.length(), static_cast<word>(newsize));
  for (word i = 0; i < min_size; i++) {
    resized.byteAtPut(i, bytes.byteAt(i));
  }
  *pyobj = ApiHandle::newReference(thread, *resized);
  handle->decref();
  return 0;
}

}  // namespace python
