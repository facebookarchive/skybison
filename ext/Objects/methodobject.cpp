#include "api-handle.h"
#include "function-utils.h"
#include "runtime.h"

namespace py {

PY_EXPORT int PyCFunction_Check_Func(PyObject* obj) {
  return !getExtensionFunction(ApiHandle::fromPyObject(obj)->asObject())
              .isErrorNotFound();
}

PY_EXPORT PyObject* PyCFunction_New(PyMethodDef* method, PyObject* self) {
  return PyCFunction_NewEx(method, self, nullptr);
}

PY_EXPORT PyObject* PyCFunction_NewEx(PyMethodDef* method, PyObject* self,
                                      PyObject* module_name) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object name(&scope, Runtime::internStrFromCStr(thread, method->ml_name));
  Object self_obj(&scope, self == nullptr
                              ? Unbound::object()
                              : ApiHandle::fromPyObject(self)->asObject());
  Object module_name_obj(&scope,
                         module_name != nullptr
                             ? ApiHandle::fromPyObject(module_name)->asObject()
                             : NoneType::object());
  return ApiHandle::newReferenceWithManaged(
      thread->runtime(),
      newCFunction(thread, method, name, self_obj, module_name_obj));
}

PY_EXPORT int PyCFunction_GetFlags(PyObject* /* p */) {
  UNIMPLEMENTED("PyCFunction_GetFlags");
}

PY_EXPORT PyCFunction PyCFunction_GetFunction(PyObject* obj) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object function(
      &scope, getExtensionFunction(ApiHandle::fromPyObject(obj)->asObject()));
  if (function.isErrorNotFound()) {
    thread->raiseBadInternalCall();
    return nullptr;
  }
  return reinterpret_cast<PyCFunction>(
      Int::cast(Function::cast(*function).code()).asCPtr());
}

PY_EXPORT PyObject* PyCFunction_GetSelf(PyObject* obj) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object bound_method(&scope, ApiHandle::fromPyObject(obj)->asObject());
  Object function(&scope, getExtensionFunction(*bound_method));
  if (function.isErrorNotFound()) {
    thread->raiseBadInternalCall();
    return nullptr;
  }
  Object self(&scope, BoundMethod::cast(*bound_method).self());
  if (self.isUnbound()) {
    return nullptr;
  }
  return ApiHandle::borrowedReference(thread->runtime(), *self);
}

PY_EXPORT PyObject* PyCFunction_GET_SELF_Func(PyObject* /* obj */) {
  UNIMPLEMENTED("PyCFunction_GET_SELF_Func");
}

PY_EXPORT PyObject* PyCFunction_Call(PyObject* /* c */, PyObject* /* s */,
                                     PyObject* /* s */) {
  UNIMPLEMENTED("PyCFunction_Call");
}

}  // namespace py
