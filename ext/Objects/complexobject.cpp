#include <cerrno>
#include <cmath>

#include "float-builtins.h"
#include "runtime.h"
#include "type-builtins.h"

namespace python {

PY_EXPORT Py_complex _Py_c_diff(Py_complex x, Py_complex y) {
  return {x.real - y.real, x.imag - y.imag};
}

PY_EXPORT Py_complex _Py_c_neg(Py_complex x) { return {-x.real, -x.imag}; }
PY_EXPORT Py_complex _Py_c_quot(Py_complex x, Py_complex y) {
  double real;
  double imag;
  if (y.imag == 0.0) {
    errno = EDOM;
    real = 0.0;
    imag = 0.0;
  } else if (std::fabs(y.real) >= std::fabs(y.imag)) {
    double ratio = y.imag / y.real;
    double den = y.real + y.imag * ratio;
    real = (x.real + x.imag * ratio) / den;
    imag = (x.imag - x.real * ratio) / den;
  } else if (std::fabs(y.imag) >= std::fabs(y.real)) {
    double ratio = y.real / y.imag;
    double den = y.real * ratio + y.imag;
    real = (x.real * ratio + x.imag) / den;
    imag = (x.imag * ratio - x.real) / den;
  } else {
    real = NAN;
    imag = NAN;
  }
  return {real, imag};
}

PY_EXPORT int PyComplex_CheckExact_Func(PyObject* p) {
  return ApiHandle::fromPyObject(p)->asObject().isComplex();
}

PY_EXPORT int PyComplex_Check_Func(PyObject* p) {
  return Thread::current()->runtime()->isInstanceOfComplex(
      ApiHandle::fromPyObject(p)->asObject());
}

PY_EXPORT Py_complex PyComplex_AsCComplex(PyObject* pycomplex) {
  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Object obj(&scope, ApiHandle::fromPyObject(pycomplex)->asObject());
  if (runtime->isInstanceOfComplex(*obj)) {
    // TODO(T36619862): strict subclass of complex
    Complex comp(&scope, *obj);
    return {comp.real(), comp.imag()};
  }

  // Try calling __complex__
  Frame* frame = thread->currentFrame();
  Type type(&scope, runtime->typeOf(*obj));
  Object comp_method(
      &scope, typeLookupSymbolInMro(thread, type, SymbolId::kDunderComplex));
  if (!comp_method.isError()) {
    Object result(&scope,
                  Interpreter::callMethod1(thread, frame, comp_method, obj));
    if (result.isError()) {
      return {-1.0, 0.0};
    }
    if (!runtime->isInstanceOfComplex(*result)) {
      thread->raiseWithFmt(LayoutId::kTypeError,
                           "__complex__ should returns a complex object");
      return {-1.0, 0.0};
    }
    // TODO(T36619862): strict subclass of complex
    Complex comp(&scope, *result);
    return {comp.real(), comp.imag()};
  }

  // Try calling __float__ for the real part and set the imaginary part to 0
  Object float_or_err(&scope, asFloatObject(thread, obj));
  if (float_or_err.isError()) return {-1.0, 0.0};
  Float flt(&scope, *float_or_err);
  return {flt.value(), 0.0};
}

PY_EXPORT PyObject* PyComplex_FromCComplex(Py_complex cmp) {
  Thread* thread = Thread::current();
  return ApiHandle::newReference(
      thread, thread->runtime()->newComplex(cmp.real, cmp.imag));
}

PY_EXPORT double PyComplex_ImagAsDouble(PyObject* pycomplex) {
  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Object obj(&scope, ApiHandle::fromPyObject(pycomplex)->asObject());
  if (!runtime->isInstanceOfComplex(*obj)) return 0.0;
  // TODO(T36619862): strict subclass of complex
  Complex comp(&scope, *obj);
  return comp.imag();
}

PY_EXPORT double PyComplex_RealAsDouble(PyObject* pycomplex) {
  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Object obj(&scope, ApiHandle::fromPyObject(pycomplex)->asObject());
  if (!runtime->isInstanceOfComplex(*obj)) {
    Object float_or_err(&scope, asFloatObject(thread, obj));
    if (float_or_err.isError()) return -1;
    return RawFloat::cast(*float_or_err).value();
  }
  // TODO(T36619862): strict subclass of complex
  Complex comp(&scope, *obj);
  return comp.real();
}

PY_EXPORT PyObject* PyComplex_FromDoubles(double real, double imag) {
  Thread* thread = Thread::current();
  return ApiHandle::newReference(thread,
                                 thread->runtime()->newComplex(real, imag));
}

}  // namespace python
