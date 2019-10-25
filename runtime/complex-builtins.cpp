#include "complex-builtins.h"

#include "float-builtins.h"
#include "frame.h"
#include "int-builtins.h"
#include "objects.h"
#include "runtime.h"

namespace py {

RawSmallInt complexHash(RawObject value) {
  RawComplex value_complex = RawComplex::cast(value);
  uword hash_real =
      static_cast<uword>(doubleHash(value_complex.real()).value());
  uword hash_imag =
      static_cast<uword>(doubleHash(value_complex.imag()).value());

  uword result = hash_real + kHashImag * hash_imag;
  if (result == static_cast<uword>(word{-1})) {
    result--;
  }
  return RawSmallInt::fromWordTruncated(static_cast<word>(result));
}

const BuiltinMethod ComplexBuiltins::kBuiltinMethods[] = {
    {SymbolId::kDunderAdd, dunderAdd},
    {SymbolId::kDunderHash, dunderHash},
    {SymbolId::kDunderNew, dunderNew},
    {SymbolId::kSentinelId, nullptr},
};

void ComplexBuiltins::postInitialize(Runtime*, const Type& new_type) {
  new_type.setBuiltinBase(LayoutId::kComplex);
}

RawObject ComplexBuiltins::dunderHash(Thread* thread, Frame* frame,
                                      word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);

  Object self_obj(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfComplex(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kComplex);
  }
  if (!self_obj.isComplex()) {
    UNIMPLEMENTED("Subclasses of complex");
  }
  return complexHash(*self_obj);
}

RawObject ComplexBuiltins::dunderNew(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();

  Object type_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfType(*type_obj)) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "complex.__new__(X): X is not a type object");
  }

  Type type(&scope, *type_obj);
  if (type.builtinBase() != LayoutId::kComplex) {
    return thread->raiseWithFmt(
        LayoutId::kTypeError,
        "complex.__new__(X): X is not a subtype of complex");
  }

  Layout layout(&scope, type.instanceLayout());
  if (layout.id() != LayoutId::kComplex) {
    // TODO(T32518507): Implement __new__ with subtypes of complex.
    UNIMPLEMENTED("complex.__new__(<subtype of complex>, ...)");
  }

  Object real_arg(&scope, args.get(1));
  Object imag_arg(&scope, args.get(2));
  // If it's already exactly a complex, return it immediately.
  if (real_arg.isComplex()) {
    return *real_arg;
  }

  // TODO(T32518507): Implement the rest of the conversions.
  // For now, it will only work with small integers and floats.
  double real = 0.0;
  if (real_arg.isSmallInt()) {
    real = SmallInt::cast(*real_arg).value();
  } else if (real_arg.isFloat()) {
    real = Float::cast(*real_arg).value();
  } else {
    UNIMPLEMENTED("Convert non-numeric to numeric");
  }
  double imag = 0.0;
  if (imag_arg.isSmallInt()) {
    imag = SmallInt::cast(*imag_arg).value();
  } else if (imag_arg.isFloat()) {
    imag = Float::cast(*imag_arg).value();
  } else {
    UNIMPLEMENTED("Convert non-numeric to numeric");
  }
  return runtime->newComplex(real, imag);
}

RawObject ComplexBuiltins::dunderAdd(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfComplex(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kComplex);
  }
  if (!self_obj.isComplex()) {
    UNIMPLEMENTED("Subclasses of complex");
  }
  Complex self(&scope, *self_obj);
  Object other_obj(&scope, args.get(1));
  double other_real, other_imag;
  if (runtime->isInstanceOfInt(*other_obj)) {
    Int other_int(&scope, intUnderlying(thread, other_obj));
    Object result(&scope, convertIntToDouble(thread, other_int, &other_real));
    if (result.isError()) return *result;
    other_imag = 0.0;
  } else {
    UNIMPLEMENTED("complex.__add__(non-int)");
  }
  return runtime->newComplex(self.real() + other_real,
                             self.imag() + other_imag);
}

}  // namespace py
