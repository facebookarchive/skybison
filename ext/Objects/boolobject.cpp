// boolobject.c implementation

#include "Python.h"

#include "objects.h"
#include "runtime.h"

namespace python {

void PyBool_Type_Init(void) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();

  ApiTypeHandle* pytype_type =
      runtime->builtinTypeHandles(ExtensionTypes::kType);
  ApiTypeHandle* pybool_type =
      ApiTypeHandle::newTypeHandle("bool", pytype_type);

  runtime->addBuiltinTypeHandle(pybool_type);
}

extern "C" PyTypeObject* PyBool_Type_Ptr() {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  return static_cast<PyTypeObject*>(
      runtime->builtinTypeHandles(ExtensionTypes::kBool));
}

extern "C" PyObject* PyTrue_Ptr() {
  return ApiHandle::fromObject(Boolean::trueObj())->asPyObject();
}

extern "C" PyObject* PyFalse_Ptr() {
  return ApiHandle::fromObject(Boolean::falseObj())->asPyObject();
}

// Start of automatically generated code, from Object/boolobject.c
extern "C" PyObject* PyBool_FromLong(long ok) {
  PyObject* result;

  if (ok) {
    result = Py_True;
  } else {
    result = Py_False;
  }
  Py_INCREF(result);
  return result;
}
// End of automatically generated code

}  // namespace python
