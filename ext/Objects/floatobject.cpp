// floatobject.c implementation

#include <cfloat>

#include "float-builtins.h"
#include "objects.h"
#include "runtime.h"

namespace python {

PY_EXPORT PyObject* PyFloat_FromDouble(double fval) {
  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Object flt(&scope, runtime->newFloat(fval));
  return ApiHandle::newReference(thread, *flt);
}

PY_EXPORT double PyFloat_AsDouble(PyObject* op) {
  Thread* thread = Thread::current();
  if (op == nullptr) {
    thread->raiseBadArgument();
    return -1;
  }

  // Object is float
  HandleScope scope(thread);
  Object obj(&scope, ApiHandle::fromPyObject(op)->asObject());
  Object float_or_err(&scope, asFloatObject(thread, obj));
  if (float_or_err.isError()) return -1;
  return Float::cast(*float_or_err).value();
}

PY_EXPORT int PyFloat_CheckExact_Func(PyObject* obj) {
  return ApiHandle::fromPyObject(obj)->asObject().isFloat();
}

PY_EXPORT int PyFloat_Check_Func(PyObject* obj) {
  return Thread::current()->runtime()->isInstanceOfFloat(
      ApiHandle::fromPyObject(obj)->asObject());
}

PY_EXPORT int PyFloat_ClearFreeList() { return 0; }

PY_EXPORT PyObject* PyFloat_FromString(PyObject* /* v */) {
  UNIMPLEMENTED("PyFloat_FromString");
}

PY_EXPORT PyObject* PyFloat_GetInfo() { UNIMPLEMENTED("PyFloat_GetInfo"); }

PY_EXPORT double PyFloat_GetMax() { return DBL_MAX; }

PY_EXPORT double PyFloat_GetMin() { return DBL_MIN; }

}  // namespace python
