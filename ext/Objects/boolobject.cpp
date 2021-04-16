// boolobject.c implementation

#include "cpython-data.h"

#include "api-handle.h"
#include "objects.h"
#include "runtime.h"

namespace py {

PY_EXPORT PyTypeObject* PyBool_Type_Ptr() {
  Runtime* runtime = Thread::current()->runtime();
  return reinterpret_cast<PyTypeObject*>(
      ApiHandle::borrowedReference(runtime, runtime->typeAt(LayoutId::kBool)));
}

PY_EXPORT PyObject* PyTrue_Ptr() {
  return ApiHandle::borrowedReference(Thread::current()->runtime(),
                                      Bool::trueObj());
}

PY_EXPORT PyObject* PyFalse_Ptr() {
  return ApiHandle::borrowedReference(Thread::current()->runtime(),
                                      Bool::falseObj());
}

PY_EXPORT int PyBool_Check_Func(PyObject* obj) {
  return ApiHandle::fromPyObject(obj)->asObject().isBool();
}

PY_EXPORT PyObject* PyBool_FromLong(long v) {
  return ApiHandle::newReference(Thread::current()->runtime(),
                                 Bool::fromBool(v));
}

}  // namespace py
