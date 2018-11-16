// boolobject.c implementation

#include "cpython-data.h"
#include "objects.h"
#include "runtime.h"

namespace python {

extern "C" PyObject* PyTrue_Ptr() {
  return ApiHandle::fromObject(Bool::trueObj())->asPyObject();
}

extern "C" PyObject* PyFalse_Ptr() {
  return ApiHandle::fromObject(Bool::falseObj())->asPyObject();
}

extern "C" int PyBool_Check_Func(PyObject* obj) {
  return ApiHandle::fromPyObject(obj)->asObject()->isBool();
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
