#include "capi-handles.h"
#include "function-utils.h"
#include "runtime.h"

namespace py {

PY_EXPORT int PyCFunction_ClearFreeList() { return 0; }

PY_EXPORT PyObject* PyCFunction_NewEx(PyMethodDef* method, PyObject* self,
                                      PyObject* module) {
  DCHECK(self != nullptr || module != nullptr,
         "PyCFunction_NewEx needs at least a self or module argument");
  DCHECK(self == nullptr || module == nullptr,
         "PyCFunction_NewEx can't have both a self and module argument");
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object function(&scope, NoneType::object());
  if (module != nullptr) {
    DCHECK(PyModule_Check(module), "module must be a module");
    function = functionFromModuleMethodDef(
        thread, method->ml_name, bit_cast<void*>(method->ml_meth),
        method->ml_doc, methodTypeFromMethodFlags(method->ml_flags));
  } else {
    Function self_func(
        &scope,
        functionFromMethodDef(thread, method->ml_name,
                              bit_cast<void*>(method->ml_meth), method->ml_doc,
                              methodTypeFromMethodFlags(method->ml_flags)));
    // Pass self as the first argument
    Object self_obj(&scope, ApiHandle::fromPyObject(self)->asObject());
    function = thread->runtime()->newBoundMethod(self_func, self_obj);
  }
  return ApiHandle::newReference(thread, *function);
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
