#include "runtime.h"

namespace python {

PY_EXPORT int PyByteArray_CheckExact_Func(PyObject* pyobj) {
  return ApiHandle::fromPyObject(pyobj)->asObject()->isByteArray();
}

PY_EXPORT int PyByteArray_Check_Func(PyObject* pyobj) {
  return Thread::currentThread()->runtime()->isInstanceOfByteArray(
      ApiHandle::fromPyObject(pyobj)->asObject());
}

PY_EXPORT PyObject* PyByteArray_FromStringAndSize(const char* str,
                                                  Py_ssize_t size) {
  Thread* thread = Thread::currentThread();
  if (size < 0) {
    thread->raiseSystemErrorWithCStr(
        "Negative size passed to PyByteArray_FromStringAndSize");
    return nullptr;
  }
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  ByteArray result(&scope, runtime->newByteArray());
  if (size > 0) {
    word capacity = static_cast<word>(size);
    if (str == nullptr) {
      result.setBytes(runtime->newBytes(capacity, 0));
    } else {
      View<byte> view(reinterpret_cast<const byte*>(str), capacity);
      result.setBytes(runtime->newBytesWithAll(view));
    }
    result.setNumBytes(capacity);
  }
  return ApiHandle::newReference(thread, *result);
}

PY_EXPORT char* PyByteArray_AsString(PyObject* /* f */) {
  UNIMPLEMENTED("PyByteArray_AsString");
}

PY_EXPORT PyObject* PyByteArray_Concat(PyObject* /* a */, PyObject* /* b */) {
  UNIMPLEMENTED("PyByteArray_Concat");
}

PY_EXPORT PyObject* PyByteArray_FromObject(PyObject* /* t */) {
  UNIMPLEMENTED("PyByteArray_FromObject");
}

PY_EXPORT int PyByteArray_Resize(PyObject* pyobj, Py_ssize_t newsize) {
  DCHECK(pyobj != nullptr, "null argument to PyByteArray_Resize");
  DCHECK(newsize >= 0, "negative size");
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Object obj(&scope, ApiHandle::fromPyObject(pyobj)->asObject());
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfByteArray(*obj)) {
    thread->raiseBadArgument();
    return -1;
  }
  ByteArray array(&scope, *obj);
  word requested = static_cast<word>(newsize);
  word current = array.numBytes();
  if (requested == current) return 0;
  if (requested < current) {
    // Ensure no leftover bytes remain past the new end of the array
    Bytes bytes(&scope, array.bytes());
    array.setBytes(runtime->bytesSubseq(thread, bytes, 0, requested));
  } else {
    runtime->byteArrayEnsureCapacity(thread, array, requested - 1);
  }
  array.setNumBytes(requested);
  return 0;
}

PY_EXPORT Py_ssize_t PyByteArray_Size(PyObject* pyobj) {
  DCHECK(pyobj != nullptr, "null argument to PyByteArray_Size");
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Object obj(&scope, ApiHandle::fromPyObject(pyobj)->asObject());
  if (!thread->runtime()->isInstanceOfByteArray(*obj)) {
    thread->raiseBadArgument();
    return -1;
  }
  return static_cast<Py_ssize_t>(ByteArray::cast(*obj)->numBytes());
}

}  // namespace python
