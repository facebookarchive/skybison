#include "api-handle.h"
#include "bytearray-builtins.h"
#include "bytearrayobject-utils.h"
#include "runtime.h"

namespace py {

PY_EXPORT PyTypeObject* PyByteArrayIter_Type_Ptr() {
  Runtime* runtime = Thread::current()->runtime();
  return reinterpret_cast<PyTypeObject*>(ApiHandle::borrowedReference(
      runtime, runtime->typeAt(LayoutId::kBytearrayIterator)));
}

char* bytearrayAsString(Runtime* runtime, ApiHandle* handle,
                        const Bytearray& array) {
  if (void* cache = handle->cache(runtime)) std::free(cache);
  word len = array.numItems();
  auto buffer = static_cast<byte*>(std::malloc(len + 1));
  array.copyTo(buffer, len);
  buffer[len] = '\0';
  handle->setCache(runtime, buffer);
  return reinterpret_cast<char*>(buffer);
}

PY_EXPORT char* PyByteArray_AsString(PyObject* pyobj) {
  DCHECK(pyobj != nullptr, "null argument to PyByteArray_AsString");
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ApiHandle* handle = ApiHandle::fromPyObject(pyobj);
  Object obj(&scope, handle->asObject());
  Runtime* runtime = thread->runtime();
  DCHECK(runtime->isInstanceOfBytearray(*obj),
         "argument to PyByteArray_AsString is not a bytearray");
  Bytearray array(&scope, *obj);
  return bytearrayAsString(runtime, handle, array);
}

PY_EXPORT int PyByteArray_CheckExact_Func(PyObject* pyobj) {
  return ApiHandle::fromPyObject(pyobj)->asObject().isBytearray();
}

PY_EXPORT int PyByteArray_Check_Func(PyObject* pyobj) {
  return Thread::current()->runtime()->isInstanceOfBytearray(
      ApiHandle::fromPyObject(pyobj)->asObject());
}

PY_EXPORT PyObject* PyByteArray_Concat(PyObject* a, PyObject* b) {
  DCHECK(a != nullptr, "null argument to PyByteArray_Concat");
  DCHECK(b != nullptr, "null argument to PyByteArray_Concat");
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object left(&scope, ApiHandle::fromPyObject(a)->asObject());
  Object right(&scope, ApiHandle::fromPyObject(b)->asObject());
  Runtime* runtime = thread->runtime();
  if (!runtime->isByteslike(*left) || !runtime->isByteslike(*right)) {
    thread->raiseWithFmt(LayoutId::kTypeError, "can't concat %T to %T", &left,
                         &right);
    return nullptr;
  }
  Object result(&scope, runtime->newBytearray());
  result = thread->invokeFunction2(ID(operator), ID(iconcat), result, left);
  if (result.isError()) return nullptr;
  result = thread->invokeFunction2(ID(operator), ID(iconcat), result, right);
  if (result.isError()) return nullptr;
  return ApiHandle::newReferenceWithManaged(runtime, *result);
}

PY_EXPORT PyObject* PyByteArray_FromStringAndSize(const char* str,
                                                  Py_ssize_t size) {
  Thread* thread = Thread::current();
  if (size < 0) {
    thread->raiseWithFmt(
        LayoutId::kSystemError,
        "Negative size passed to PyByteArray_FromStringAndSize");
    return nullptr;
  }

  Runtime* runtime = thread->runtime();
  if (size == 0) {
    return ApiHandle::newReferenceWithManaged(runtime, runtime->newBytearray());
  }

  HandleScope scope(thread);
  Bytearray result(&scope, runtime->newBytearray());
  if (str == nullptr) {
    runtime->bytearrayEnsureCapacity(thread, result, size);
    result.setNumItems(size);
  } else {
    runtime->bytearrayExtend(thread, result,
                             {reinterpret_cast<const byte*>(str), size});
  }
  return ApiHandle::newReferenceWithManaged(runtime, *result);
}

PY_EXPORT PyObject* PyByteArray_FromObject(PyObject* obj) {
  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  if (obj == nullptr) {
    return ApiHandle::newReferenceWithManaged(runtime, runtime->newBytearray());
  }
  HandleScope scope(thread);
  Object src(&scope, ApiHandle::fromPyObject(obj)->asObject());
  Object result(&scope,
                thread->invokeFunction1(ID(builtins), ID(bytearray), src));
  return result.isError() ? nullptr : ApiHandle::newReference(runtime, *result);
}

PY_EXPORT int PyByteArray_Resize(PyObject* pyobj, Py_ssize_t newsize) {
  DCHECK(pyobj != nullptr, "null argument to PyByteArray_Resize");
  DCHECK(newsize >= 0, "negative size");
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object obj(&scope, ApiHandle::fromPyObject(pyobj)->asObject());
  Runtime* runtime = thread->runtime();
  DCHECK(runtime->isInstanceOfBytearray(*obj),
         "argument to PyByteArray_Resize is not a bytearray");
  Bytearray array(&scope, *obj);
  word requested = static_cast<word>(newsize);
  word current = array.numItems();
  if (requested == current) return 0;
  if (requested < current) {
    array.downsize(requested);
  } else {
    runtime->bytearrayEnsureCapacity(thread, array, requested);
  }
  array.setNumItems(requested);
  return 0;
}

PY_EXPORT Py_ssize_t PyByteArray_Size(PyObject* pyobj) {
  DCHECK(pyobj != nullptr, "null argument to PyByteArray_Size");
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object obj(&scope, ApiHandle::fromPyObject(pyobj)->asObject());
  DCHECK(thread->runtime()->isInstanceOfBytearray(*obj),
         "argument to PyByteArray_Size is not a bytearray");
  return static_cast<Py_ssize_t>(Bytearray::cast(*obj).numItems());
}

PY_EXPORT PyTypeObject* PyByteArray_Type_Ptr() {
  Runtime* runtime = Thread::current()->runtime();
  return reinterpret_cast<PyTypeObject*>(ApiHandle::borrowedReference(
      runtime, runtime->typeAt(LayoutId::kBytearray)));
}

}  // namespace py
