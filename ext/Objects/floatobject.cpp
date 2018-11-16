// floatobject.c implementation

#include "objects.h"
#include "runtime.h"

namespace python {

void PyFloat_Type_Init() {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();

  ApiTypeHandle* pytype_type =
      runtime->builtinTypeHandles(ExtensionTypes::kType);
  ApiTypeHandle* pyfloat_type =
      ApiTypeHandle::newTypeHandle("float", pytype_type);

  runtime->addBuiltinTypeHandle(pyfloat_type);
}

extern "C" PyTypeObject* PyFloat_Type_Ptr() {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  return static_cast<PyTypeObject*>(
      runtime->builtinTypeHandles(ExtensionTypes::kFloat));
}

extern "C" PyObject* PyFloat_FromDouble(double fval) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Handle<Object> flt(&scope, runtime->newDouble(fval));
  return ApiHandle::fromObject(*flt)->asPyObject();
}

extern "C" double PyFloat_AsDouble(PyObject*) {
  UNIMPLEMENTED("PyFloat_AsDouble");
}

}  // namespace python
