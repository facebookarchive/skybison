// boolobject.c implementation

#include "cpython-data.h"
#include "objects.h"
#include "runtime.h"

namespace python {

void PyBool_Type_Init() {
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
  return ApiHandle::fromObject(Bool::trueObj())->asPyObject();
}

extern "C" PyObject* PyFalse_Ptr() {
  return ApiHandle::fromObject(Bool::falseObj())->asPyObject();
}

// TODO(eelizondo): Remove once boolobject.c is complied in
extern "C" PyObject* PyBool_FromLong(long ok) {
  PyObject* result;

  if (ok) {
    result = Py_True;
  } else {
    result = Py_False;
  }
  // TODO(eelizondo): increment the reference count through ApiHandle
  result->ob_refcnt++;
  return result;
}

}  // namespace python
