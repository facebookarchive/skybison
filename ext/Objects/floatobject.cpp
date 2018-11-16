// floatobject.c implementation

#include "objects.h"
#include "runtime.h"

namespace python {

PY_EXPORT PyObject* PyFloat_FromDouble(double fval) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Object flt(&scope, runtime->newFloat(fval));
  return ApiHandle::fromObject(*flt);
}

PY_EXPORT double PyFloat_AsDouble(PyObject* op) {
  Thread* thread = Thread::currentThread();
  if (op == nullptr) {
    thread->raiseSystemErrorWithCStr("bad argument to internal function");
    return -1;
  }

  // Object is float or subclass of float
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object obj(&scope, ApiHandle::fromPyObject(op)->asObject());
  if (runtime->isInstanceOfFloat(*obj)) {
    Float flt(&scope, *obj);
    return flt->value();
  }

  // Try calling __float__
  Frame* frame = thread->currentFrame();
  Object fltmethod(&scope, Interpreter::lookupMethod(thread, frame, obj,
                                                     SymbolId::kDunderFloat));
  if (fltmethod->isError()) {
    thread->raiseTypeErrorWithCStr("must be a real number");
    return -1;
  }
  Object flt_obj(&scope,
                 Interpreter::callMethod1(thread, frame, fltmethod, obj));
  if (!runtime->isInstanceOfFloat(*flt_obj)) {
    thread->raiseTypeErrorWithCStr("__float__ returned non-float");
    return -1;
  }
  Float flt(&scope, *flt_obj);
  return flt->value();
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

PY_EXPORT PyObject* PyFloat_GetInfo() { UNIMPLEMENTED("PyFloat_GetInfo"); }

PY_EXPORT double PyFloat_GetMax() { UNIMPLEMENTED("PyFloat_GetMax"); }

PY_EXPORT double PyFloat_GetMin() { UNIMPLEMENTED("PyFloat_GetMin"); }

}  // namespace python
