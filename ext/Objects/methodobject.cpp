#include "capi-handles.h"
#include "function-utils.h"
#include "runtime.h"

namespace py {

RawObject newCFunction(Thread* thread, PyMethodDef* method, const Object& self,
                       const Object& module_name) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Function function(
      &scope, functionFromMethodDef(
                  thread, method->ml_name, bit_cast<void*>(method->ml_meth),
                  method->ml_doc,
                  methodTypeFromMethodFlags(method->ml_flags & ~METH_CLASS &
                                            ~METH_STATIC)));
  if (runtime->isInstanceOfStr(*module_name)) {
    function.setModuleName(*module_name);
  }
  return runtime->newBoundMethod(function, self);
}

PY_EXPORT PyObject* PyCFunction_NewEx(PyMethodDef* method, PyObject* self,
                                      PyObject* module_name) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object self_obj(&scope, self == nullptr
                              ? Unbound::object()
                              : ApiHandle::fromPyObject(self)->asObject());
  Object module_name_obj(&scope,
                         module_name != nullptr
                             ? ApiHandle::fromPyObject(module_name)->asObject()
                             : NoneType::object());
  return ApiHandle::newReference(
      thread, newCFunction(thread, method, self_obj, module_name_obj));
}

PY_EXPORT int PyCFunction_GetFlags(PyObject* /* p */) {
  UNIMPLEMENTED("PyCFunction_GetFlags");
}

PY_EXPORT PyCFunction PyCFunction_GetFunction(PyObject* /* p */) {
  UNIMPLEMENTED("PyCFunction_GetFunction");
}

PY_EXPORT PyObject* PyCFunction_GetSelf(PyObject* /* p */) {
  UNIMPLEMENTED("PyCFunction_GetSelf");
}

PY_EXPORT PyObject* PyCFunction_Call(PyObject* /* c */, PyObject* /* s */,
                                     PyObject* /* s */) {
  UNIMPLEMENTED("PyCFunction_Call");
}

}  // namespace py
