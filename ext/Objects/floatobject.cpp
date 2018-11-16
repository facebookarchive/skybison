// floatobject.c implementation

#include "objects.h"
#include "runtime.h"

namespace python {

PY_EXPORT PyObject* PyFloat_FromDouble(double fval) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Handle<Object> flt(&scope, runtime->newFloat(fval));
  return ApiHandle::fromObject(*flt);
}

PY_EXPORT double PyFloat_AsDouble(PyObject*) {
  UNIMPLEMENTED("PyFloat_AsDouble");
}

PY_EXPORT int PyFloat_CheckExact_Func(PyObject* obj) {
  return ApiHandle::fromPyObject(obj)->asObject()->isFloat();
}

PY_EXPORT int PyFloat_Check_Func(PyObject* obj) {
  if (PyFloat_CheckExact_Func(obj)) {
    return true;
  }
  return ApiHandle::fromPyObject(obj)->isSubClass(Thread::currentThread(),
                                                  LayoutId::kFloat);
}

PY_EXPORT PyObject* PyFloat_FromString(PyObject* /* v */) {
  UNIMPLEMENTED("PyFloat_FromString");
}

PY_EXPORT PyObject* PyFloat_GetInfo(void) { UNIMPLEMENTED("PyFloat_GetInfo"); }

PY_EXPORT double PyFloat_GetMax(void) { UNIMPLEMENTED("PyFloat_GetMax"); }

PY_EXPORT double PyFloat_GetMin(void) { UNIMPLEMENTED("PyFloat_GetMin"); }

}  // namespace python
