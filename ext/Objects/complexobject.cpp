#include "float-builtins.h"
#include "runtime.h"

namespace python {

PY_EXPORT int PyComplex_CheckExact_Func(PyObject* p) {
  return ApiHandle::fromPyObject(p)->asObject()->isComplex();
}

PY_EXPORT int PyComplex_Check_Func(PyObject* p) {
  return Thread::currentThread()->runtime()->isInstanceOfComplex(
      ApiHandle::fromPyObject(p)->asObject());
}

PY_EXPORT Py_complex PyComplex_AsCComplex(PyObject* pycomplex) {
  Thread* thread = Thread::currentThread();
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
  Object comp_method(&scope, runtime->lookupSymbolInMro(
                                 thread, type, SymbolId::kDunderComplex));
  if (!comp_method.isError()) {
    Object result(&scope,
                  Interpreter::callMethod1(thread, frame, comp_method, obj));
    if (result.isError()) {
      return {-1.0, 0.0};
    }
    if (!runtime->isInstanceOfComplex(*result)) {
      thread->raiseTypeErrorWithCStr(
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
  Thread* thread = Thread::currentThread();
  return ApiHandle::newReference(
      thread, thread->runtime()->newComplex(cmp.real, cmp.imag));
}

PY_EXPORT double PyComplex_ImagAsDouble(PyObject* pycomplex) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Object obj(&scope, ApiHandle::fromPyObject(pycomplex)->asObject());
  if (!runtime->isInstanceOfComplex(*obj)) return 0.0;
  // TODO(T36619862): strict subclass of complex
  Complex comp(&scope, *obj);
  return comp.imag();
}

PY_EXPORT double PyComplex_RealAsDouble(PyObject* pycomplex) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Object obj(&scope, ApiHandle::fromPyObject(pycomplex)->asObject());
  if (!runtime->isInstanceOfComplex(*obj)) {
    Object float_or_err(&scope, asFloatObject(thread, obj));
    if (float_or_err.isError()) return -1;
    return RawFloat::cast(*float_or_err)->value();
  }
  // TODO(T36619862): strict subclass of complex
  Complex comp(&scope, *obj);
  return comp.real();
}

PY_EXPORT PyObject* PyComplex_FromDoubles(double real, double imag) {
  Thread* thread = Thread::currentThread();
  return ApiHandle::newReference(thread,
                                 thread->runtime()->newComplex(real, imag));
}

}  // namespace python
