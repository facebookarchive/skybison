// boolobject.c implementation

#include "cpython-data.h"
#include "objects.h"
#include "runtime.h"

namespace python {

extern "C" PyObject* PyTrue_Ptr() {
  return ApiHandle::fromObject(Bool::trueObj());
}

extern "C" PyObject* PyFalse_Ptr() {
  return ApiHandle::fromObject(Bool::falseObj());
}

extern "C" int PyBool_Check_Func(PyObject* obj) {
  return ApiHandle::fromPyObject(obj)->asObject()->isBool();
}

extern "C" PyObject* PyBool_FromLong(long v) {
  return ApiHandle::fromObject(Bool::fromBool(v));
}

}  // namespace python
