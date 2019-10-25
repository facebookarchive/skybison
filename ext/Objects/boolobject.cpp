// boolobject.c implementation

#include "capi-handles.h"
#include "cpython-data.h"
#include "objects.h"
#include "runtime.h"

namespace py {

PY_EXPORT PyObject* PyTrue_Ptr() {
  return ApiHandle::borrowedReference(Thread::current(), Bool::trueObj());
}

PY_EXPORT PyObject* PyFalse_Ptr() {
  return ApiHandle::borrowedReference(Thread::current(), Bool::falseObj());
}

PY_EXPORT int PyBool_Check_Func(PyObject* obj) {
  return ApiHandle::fromPyObject(obj)->asObject().isBool();
}

PY_EXPORT PyObject* PyBool_FromLong(long v) {
  return ApiHandle::newReference(Thread::current(), Bool::fromBool(v));
}

}  // namespace py
