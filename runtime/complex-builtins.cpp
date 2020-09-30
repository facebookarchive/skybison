#include "complex-builtins.h"

#include <cmath>

#include "builtins.h"
#include "float-builtins.h"
#include "frame.h"
#include "int-builtins.h"
#include "objects.h"
#include "runtime.h"
#include "type-builtins.h"

namespace py {

word complexHash(RawObject value) {
  RawComplex value_complex = RawComplex::cast(value);
  uword hash_real = static_cast<uword>(doubleHash(value_complex.real()));
  uword hash_imag = static_cast<uword>(doubleHash(value_complex.imag()));

  uword result = hash_real + kHashImag * hash_imag;
  if (result == static_cast<uword>(word{-1})) {
    result--;
  }
  return static_cast<word>(result);
}

static RawObject unpackNumber(Thread* thread, const Object& obj, double* real,
                              double* imag) {
  Runtime* runtime = thread->runtime();
  if (runtime->isInstanceOfInt(*obj)) {
    HandleScope scope(thread);
    Int obj_int(&scope, intUnderlying(*obj));
    *imag = 0.0;
    return convertIntToDouble(thread, obj_int, real);
  }
  if (runtime->isInstanceOfFloat(*obj)) {
    *real = floatUnderlying(*obj).value();
    *imag = 0.0;
    return NoneType::object();
  }
  if (runtime->isInstanceOfComplex(*obj)) {
    RawComplex obj_complex = complexUnderlying(*obj);
    *real = obj_complex.real();
    *imag = obj_complex.imag();
    return NoneType::object();
  }
  return NotImplementedType::object();
}

void initializeComplexType(Thread* thread) {
  HandleScope scope(thread);
  Type type(&scope,
            addBuiltinType(thread, ID(complex), LayoutId::kComplex,
                           /*superclass_id=*/LayoutId::kObject, kNoAttributes,
                           /*basetype=*/true));
  type.setBuiltinBase(LayoutId::kComplex);
}

RawObject METH(complex, __abs__)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfComplex(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(complex));
  }
  Complex self(&scope, complexUnderlying(*self_obj));
  double real = self.real();
  double imag = self.imag();
  double magnitude = std::sqrt(real * real + imag * imag);
  return runtime->newFloat(magnitude);
}

RawObject METH(complex, __add__)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfComplex(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(complex));
  }
  Complex self(&scope, complexUnderlying(*self_obj));
  double other_real, other_imag;
  Object other(&scope, args.get(1));
  other = unpackNumber(thread, other, &other_real, &other_imag);
  if (!other.isNoneType()) {
    return *other;
  }
  return runtime->newComplex(self.real() + other_real,
                             self.imag() + other_imag);
}

RawObject METH(complex, __hash__)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);

  Object self_obj(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfComplex(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(complex));
  }
  Complex self(&scope, complexUnderlying(*self_obj));
  return SmallInt::fromWord(complexHash(*self));
}

RawObject METH(complex, __mul__)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfComplex(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(complex));
  }
  Complex self(&scope, complexUnderlying(*self_obj));
  double other_real, other_imag;
  Object other(&scope, args.get(1));
  other = unpackNumber(thread, other, &other_real, &other_imag);
  if (!other.isNoneType()) {
    return *other;
  }

  double self_real = self.real();
  double self_imag = self.imag();
  double res_real = self_real * other_real - self_imag * other_imag;
  double res_imag = self_real * other_imag + self_imag * other_real;
  return runtime->newComplex(res_real, res_imag);
}

RawObject METH(complex, __neg__)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfComplex(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(complex));
  }
  Complex self(&scope, complexUnderlying(*self_obj));
  return runtime->newComplex(-self.real(), -self.imag());
}

RawObject METH(complex, __pos__)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);

  Object self_obj(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfComplex(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(complex));
  }
  return complexUnderlying(*self_obj);
}

RawObject METH(complex, __rsub__)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfComplex(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(complex));
  }
  Complex self(&scope, complexUnderlying(*self_obj));
  double other_real, other_imag;
  Object other(&scope, args.get(1));
  other = unpackNumber(thread, other, &other_real, &other_imag);
  if (!other.isNoneType()) {
    return *other;
  }
  return runtime->newComplex(other_real - self.real(),
                             other_imag - self.imag());
}

RawObject METH(complex, __sub__)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfComplex(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(complex));
  }
  Complex self(&scope, complexUnderlying(*self_obj));
  double other_real, other_imag;
  Object other(&scope, args.get(1));
  other = unpackNumber(thread, other, &other_real, &other_imag);
  if (!other.isNoneType()) {
    return *other;
  }
  return runtime->newComplex(self.real() - other_real,
                             self.imag() - other_imag);
}

RawObject METH(complex, __truediv__)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfComplex(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(complex));
  }
  Complex self(&scope, complexUnderlying(*self_obj));
  double other_real, other_imag;
  Object other(&scope, args.get(1));
  other = unpackNumber(thread, other, &other_real, &other_imag);
  if (!other.isNoneType()) {
    return *other;
  }

  double self_real = self.real();
  double self_imag = self.imag();
  double abs_other_real = std::abs(other_real);
  double abs_other_imag = std::abs(other_imag);

  double res_real, res_imag;
  if (abs_other_real >= abs_other_imag) {
    if (abs_other_real == 0.0) {
      return thread->raiseWithFmt(LayoutId::kZeroDivisionError,
                                  "complex division by zero");
    }
    double ratio = other_imag / other_real;
    double denom = other_real + other_imag * ratio;

    res_real = (self_real + self_imag * ratio) / denom;
    res_imag = (self_imag - self_real * ratio) / denom;

  } else if (abs_other_real < abs_other_imag) {
    double ratio = other_real / other_imag;
    double denom = other_real * ratio + other_imag;

    res_real = (self_real * ratio + self_imag) / denom;
    res_imag = (self_imag * ratio - self_real) / denom;

  } else {
    res_real = kDoubleNaN;
    res_imag = kDoubleNaN;
  }

  return runtime->newComplex(res_real, res_imag);
}

}  // namespace py
