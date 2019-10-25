#include "bytearray-builtins.h"
#include "capi-handles.h"
#include "runtime.h"

namespace py {

PY_EXPORT char* PyByteArray_AsString(PyObject* pyobj) {
  DCHECK(pyobj != nullptr, "null argument to PyByteArray_AsString");
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ApiHandle* handle = ApiHandle::fromPyObject(pyobj);
  Object obj(&scope, handle->asObject());
  Runtime* runtime = thread->runtime();
  DCHECK(runtime->isInstanceOfByteArray(*obj),
         "argument to PyByteArray_AsString is not a bytearray");
  if (void* cache = handle->cache()) std::free(cache);
  ByteArray array(&scope, *obj);
  word len = array.numItems();
  auto buffer = static_cast<byte*>(std::malloc(len + 1));
  array.copyTo(buffer, len);
  buffer[len] = '\0';
  handle->setCache(buffer);
  return reinterpret_cast<char*>(buffer);
}

PY_EXPORT int PyByteArray_CheckExact_Func(PyObject* pyobj) {
  return ApiHandle::fromPyObject(pyobj)->asObject().isByteArray();
}

PY_EXPORT int PyByteArray_Check_Func(PyObject* pyobj) {
  return Thread::current()->runtime()->isInstanceOfByteArray(
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
  Object result(&scope, runtime->newByteArray());
  result = thread->invokeFunction2(SymbolId::kOperator, SymbolId::kIconcat,
                                   result, left);
  if (result.isError()) return nullptr;
  result = thread->invokeFunction2(SymbolId::kOperator, SymbolId::kIconcat,
                                   result, right);
  if (result.isError()) return nullptr;
  return ApiHandle::newReference(thread, *result);
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
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  ByteArray result(&scope, runtime->newByteArray());
  if (size <= 0) {
    return ApiHandle::newReference(thread, *result);
  }
  if (str == nullptr) {
    runtime->byteArrayEnsureCapacity(thread, result, size);
  } else {
    runtime->byteArrayExtend(thread, result,
                             {reinterpret_cast<const byte*>(str), size});
  }
  result.setNumItems(size);
  return ApiHandle::newReference(thread, *result);
}

PY_EXPORT PyObject* PyByteArray_FromObject(PyObject* obj) {
  Thread* thread = Thread::current();
  if (obj == nullptr) {
    return ApiHandle::newReference(thread, thread->runtime()->newByteArray());
  }
  HandleScope scope(thread);
  Object src(&scope, ApiHandle::fromPyObject(obj)->asObject());
  Object result(&scope, thread->invokeFunction1(SymbolId::kBuiltins,
                                                SymbolId::kByteArray, src));
  return result.isError() ? nullptr : ApiHandle::newReference(thread, *result);
}

PY_EXPORT int PyByteArray_Resize(PyObject* pyobj, Py_ssize_t newsize) {
  DCHECK(pyobj != nullptr, "null argument to PyByteArray_Resize");
  DCHECK(newsize >= 0, "negative size");
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object obj(&scope, ApiHandle::fromPyObject(pyobj)->asObject());
  Runtime* runtime = thread->runtime();
  DCHECK(runtime->isInstanceOfByteArray(*obj),
         "argument to PyByteArray_Resize is not a bytearray");
  ByteArray array(&scope, *obj);
  word requested = static_cast<word>(newsize);
  word current = array.numItems();
  if (requested == current) return 0;
  if (requested < current) {
    array.downsize(requested);
  } else {
    runtime->byteArrayEnsureCapacity(thread, array, requested);
  }
  array.setNumItems(requested);
  return 0;
}

PY_EXPORT Py_ssize_t PyByteArray_Size(PyObject* pyobj) {
  DCHECK(pyobj != nullptr, "null argument to PyByteArray_Size");
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object obj(&scope, ApiHandle::fromPyObject(pyobj)->asObject());
  DCHECK(thread->runtime()->isInstanceOfByteArray(*obj),
         "argument to PyByteArray_Size is not a bytearray");
  return static_cast<Py_ssize_t>(ByteArray::cast(*obj).numItems());
}

}  // namespace py
