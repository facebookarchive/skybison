#include "capi-handles.h"
#include "function-utils.h"
#include "runtime.h"

namespace py {

RawObject newCFunction(Thread* thread, PyMethodDef* method, const Object& name,
                       const Object& self, const Object& module_name) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Function function(&scope,
                    runtime->newExtensionFunction(
                        thread, name, reinterpret_cast<void*>(method->ml_meth),
                        methodTypeFromMethodFlags(method->ml_flags)));
  if (method->ml_doc != nullptr) {
    function.setDoc(runtime->newStrFromCStr(method->ml_doc));
  }
  if (runtime->isInstanceOfStr(*module_name)) {
    function.setModuleName(*module_name);
  }
  return runtime->newBoundMethod(function, self);
}

static RawObject getExtensionFunction(RawObject object) {
  if (!object.isBoundMethod()) return Error::notFound();
  RawObject function_obj = BoundMethod::cast(object).function();
  if (!function_obj.isFunction()) return Error::notFound();
  RawFunction function = Function::cast(function_obj);
  if (!function.isExtension()) return Error::notFound();
  return function;
}

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
  return ApiHandle::newReference(
      thread, newCFunction(thread, method, name, self_obj, module_name_obj));
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
    PyErr_BadInternalCall();
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
    PyErr_BadInternalCall();
    return nullptr;
  }
  Object self(&scope, BoundMethod::cast(*bound_method).self());
  if (self.isUnbound()) {
    return nullptr;
  }
  return ApiHandle::borrowedReference(thread, *self);
}

PY_EXPORT PyObject* PyCFunction_Call(PyObject* /* c */, PyObject* /* s */,
                                     PyObject* /* s */) {
  UNIMPLEMENTED("PyCFunction_Call");
}

}  // namespace py
