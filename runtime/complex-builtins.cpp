#include "complex-builtins.h"

#include "frame.h"
#include "objects.h"
#include "runtime.h"
#include "trampolines-inl.h"

namespace python {

const BuiltinMethod ComplexBuiltins::kBuiltinMethods[] = {
    {SymbolId::kDunderNew, dunderNew},
    {SymbolId::kSentinelId, nullptr},
};

void ComplexBuiltins::postInitialize(Runtime*, const Type& new_type) {
  new_type.setBuiltinBase(LayoutId::kComplex);
}

RawObject complexGetImag(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfComplex(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kComplex);
  }
  Complex self(&scope, *self_obj);
  return runtime->newFloat(self.imag());
}

RawObject complexGetReal(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfComplex(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kComplex);
  }
  Complex self(&scope, *self_obj);
  return runtime->newFloat(self.real());
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
    real = RawSmallInt::cast(*real_arg).value();
  } else if (real_arg.isFloat()) {
    real = RawFloat::cast(*real_arg).value();
  } else {
    UNIMPLEMENTED("Convert non-numeric to numeric");
  }
  double imag = 0.0;
  if (imag_arg.isSmallInt()) {
    imag = RawSmallInt::cast(*imag_arg).value();
  } else if (imag_arg.isFloat()) {
    imag = RawFloat::cast(*imag_arg).value();
  } else {
    UNIMPLEMENTED("Convert non-numeric to numeric");
  }
  return runtime->newComplex(real, imag);
}

}  // namespace python
