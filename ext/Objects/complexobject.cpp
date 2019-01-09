#include "runtime.h"

namespace python {

PY_EXPORT int PyComplex_CheckExact_Func(PyObject* p) {
  return ApiHandle::fromPyObject(p)->asObject()->isComplex();
}

PY_EXPORT int PyComplex_Check_Func(PyObject* p) {
  return Thread::currentThread()->runtime()->isInstanceOfComplex(
      ApiHandle::fromPyObject(p)->asObject());
}

PY_EXPORT Py_complex PyComplex_AsCComplex(PyObject* /* p */) {
  UNIMPLEMENTED("PyComplex_AsCComplex");
}

PY_EXPORT PyObject* PyComplex_FromCComplex(Py_complex cmp) {
  Thread* thread = Thread::currentThread();
  return ApiHandle::newReference(
      thread, thread->runtime()->newComplex(cmp.real, cmp.imag));
}

PY_EXPORT double PyComplex_ImagAsDouble(PyObject* /* p */) {
  UNIMPLEMENTED("PyComplex_ImagAsDouble");
}

PY_EXPORT double PyComplex_RealAsDouble(PyObject* /* p */) {
  UNIMPLEMENTED("PyComplex_RealAsDouble");
}

PY_EXPORT PyObject* PyComplex_FromDoubles(double real, double imag) {
  Thread* thread = Thread::currentThread();
  return ApiHandle::newReference(thread,
                                 thread->runtime()->newComplex(real, imag));
}

}  // namespace python
