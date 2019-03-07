// boolobject.c implementation

#include "cpython-data.h"
#include "objects.h"
#include "runtime.h"

namespace python {

PY_EXPORT PyObject* PyTrue_Ptr() {
  return ApiHandle::borrowedReference(Thread::currentThread(), Bool::trueObj());
}

PY_EXPORT PyObject* PyFalse_Ptr() {
  return ApiHandle::borrowedReference(Thread::currentThread(),
                                      Bool::falseObj());
}

PY_EXPORT int PyBool_Check_Func(PyObject* obj) {
  return ApiHandle::fromPyObject(obj)->asObject().isBool();
}

PY_EXPORT PyObject* PyBool_FromLong(long v) {
  return ApiHandle::newReference(Thread::currentThread(), Bool::fromBool(v));
}

}  // namespace python
