#include "capi-handles.h"
#include "runtime.h"

namespace py {

PY_EXPORT int PyMethod_Check_Func(PyObject* obj) {
  return ApiHandle::fromPyObject(obj)->asObject().isBoundMethod();
}

PY_EXPORT PyObject* PyInstanceMethod_New(PyObject* /* c */) {
  UNIMPLEMENTED("PyInstanceMethod_New");
}

PY_EXPORT PyObject* PyMethod_Function(PyObject* obj) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object method(&scope, ApiHandle::fromPyObject(obj)->asObject());
  if (!method.isBoundMethod()) {
    thread->raiseBadInternalCall();
    return nullptr;
  }
  return ApiHandle::borrowedReference(thread,
                                      BoundMethod::cast(*method).function());
}

PY_EXPORT PyObject* PyMethod_GET_FUNCTION_Func(PyObject* obj) {
  Thread* thread = Thread::current();
  return ApiHandle::borrowedReference(
      thread,
      BoundMethod::cast(ApiHandle::fromPyObject(obj)->asObject()).function());
}

PY_EXPORT PyObject* PyMethod_New(PyObject* callable, PyObject* self) {
  DCHECK(callable != nullptr, "callable must be initialized");
  Thread* thread = Thread::current();
  if (self == nullptr) {
    thread->raiseBadInternalCall();
    return nullptr;
  }
  HandleScope scope(thread);
  Object callable_obj(&scope, ApiHandle::fromPyObject(callable)->asObject());
  Object self_obj(&scope, ApiHandle::fromPyObject(self)->asObject());
  return ApiHandle::newReference(
      thread, thread->runtime()->newBoundMethod(callable_obj, self_obj));
}

PY_EXPORT PyObject* PyMethod_Self(PyObject* obj) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object method(&scope, ApiHandle::fromPyObject(obj)->asObject());
  if (!method.isBoundMethod()) {
    thread->raiseBadInternalCall();
    return nullptr;
  }
  return ApiHandle::borrowedReference(thread,
                                      BoundMethod::cast(*method).self());
}

PY_EXPORT PyObject* PyMethod_GET_SELF_Func(PyObject* obj) {
  Thread* thread = Thread::current();
  return ApiHandle::borrowedReference(
      thread,
      BoundMethod::cast(ApiHandle::fromPyObject(obj)->asObject()).self());
}

PY_EXPORT PyTypeObject* PyMethod_Type_Ptr() {
  Thread* thread = Thread::current();
  return reinterpret_cast<PyTypeObject*>(ApiHandle::borrowedReference(
      thread, thread->runtime()->typeAt(LayoutId::kBoundMethod)));
}

}  // namespace py
