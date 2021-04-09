#include "capi-handles.h"
#include "runtime.h"

namespace py {

PY_EXPORT int PyMethod_Check_Func(PyObject* obj) {
  return ApiHandle::fromPyObject(obj)->asObject().isBoundMethod();
}

PY_EXPORT int PyInstanceMethod_Check(PyObject* obj) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object obj_obj(&scope, ApiHandle::fromPyObject(obj)->asObject());
  return obj_obj.isInstanceMethod();
}

PY_EXPORT PyObject* PyInstanceMethod_GET_FUNCTION_Func(PyObject* obj) {
  return ApiHandle::borrowedReference(
      Thread::current()->runtime(),
      InstanceMethod::cast(ApiHandle::fromPyObject(obj)->asObject())
          .function());
}

PY_EXPORT PyObject* PyInstanceMethod_New(PyObject* obj) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object callable(&scope, ApiHandle::fromPyObject(obj)->asObject());
  Runtime* runtime = thread->runtime();
  InstanceMethod method(&scope,
                        runtime->newInstanceWithSize(LayoutId::kInstanceMethod,
                                                     InstanceMethod::kSize));
  method.setFunction(*callable);
  return ApiHandle::newReference(runtime, *method);
}

PY_EXPORT PyObject* PyMethod_Function(PyObject* obj) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object method(&scope, ApiHandle::fromPyObject(obj)->asObject());
  if (!method.isBoundMethod()) {
    thread->raiseBadInternalCall();
    return nullptr;
  }
  return ApiHandle::borrowedReference(thread->runtime(),
                                      BoundMethod::cast(*method).function());
}

PY_EXPORT PyObject* PyMethod_GET_FUNCTION_Func(PyObject* obj) {
  return ApiHandle::borrowedReference(
      Thread::current()->runtime(),
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
  Runtime* runtime = thread->runtime();
  return ApiHandle::newReference(
      runtime, runtime->newBoundMethod(callable_obj, self_obj));
}

PY_EXPORT PyObject* PyMethod_Self(PyObject* obj) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object method(&scope, ApiHandle::fromPyObject(obj)->asObject());
  if (!method.isBoundMethod()) {
    thread->raiseBadInternalCall();
    return nullptr;
  }
  return ApiHandle::borrowedReference(thread->runtime(),
                                      BoundMethod::cast(*method).self());
}

PY_EXPORT PyObject* PyMethod_GET_SELF_Func(PyObject* obj) {
  return ApiHandle::borrowedReference(
      Thread::current()->runtime(),
      BoundMethod::cast(ApiHandle::fromPyObject(obj)->asObject()).self());
}

PY_EXPORT PyTypeObject* PyMethod_Type_Ptr() {
  Runtime* runtime = Thread::current()->runtime();
  return reinterpret_cast<PyTypeObject*>(ApiHandle::borrowedReference(
      runtime, runtime->typeAt(LayoutId::kBoundMethod)));
}

}  // namespace py
