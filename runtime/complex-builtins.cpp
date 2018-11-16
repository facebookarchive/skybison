#include "complex-builtins.h"

#include "frame.h"
#include "objects.h"
#include "runtime.h"
#include "trampolines-inl.h"

namespace python {

const BuiltinMethod ComplexBuiltins::kMethods[] = {
    {SymbolId::kDunderNew, nativeTrampoline<dunderNew>},
};

void ComplexBuiltins::initialize(Runtime* runtime) {
  HandleScope scope;
  Type type(&scope, runtime->addBuiltinTypeWithMethods(
                        SymbolId::kComplex, LayoutId::kComplex,
                        LayoutId::kObject, kMethods));
  type->setFlag(Type::Flag::kComplexSubclass);
}

RawObject ComplexBuiltins::dunderNew(Thread* thread, Frame* frame, word nargs) {
  if (nargs == 0) {
    return thread->raiseTypeErrorWithCStr(
        "complex.__new__(): not enough arguments");
  }
  if (nargs > 3) {
    return thread->raiseTypeErrorWithCStr(
        "complex() takes at most two arguments");
  }

  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();

  Object type_obj(&scope, args.get(0));
  if (!runtime->hasSubClassFlag(*type_obj, Type::Flag::kTypeSubclass)) {
    return thread->raiseTypeErrorWithCStr(
        "complex.__new__(X): X is not a type object");
  }

  Type type(&scope, *type_obj);
  if (!type->hasFlag(Type::Flag::kComplexSubclass)) {
    return thread->raiseTypeErrorWithCStr(
        "complex.__new__(X): X is not a subtype of complex");
  }

  Layout layout(&scope, type->instanceLayout());
  if (layout->id() != LayoutId::kComplex) {
    // TODO(T32518507): Implement __new__ with subtypes of complex.
    UNIMPLEMENTED("complex.__new__(<subtype of complex>, ...)");
  }

  // If there are no arguments other than the type, return a zero complex.
  if (nargs == 1) {
    return runtime->newComplex(0, 0);
  }

  Object real_arg(&scope, args.get(1));
  Object imag_arg(&scope, nargs == 3 ? args.get(2) : runtime->newFloat(0));
  // If it's already exactly a complex, return it immediately.
  if (real_arg->isComplex()) {
    return *real_arg;
  }

  // TODO(T32518507): Implement the rest of the conversions.
  // For now, it will only work with small integers and floats.
  double real = 0.0;
  if (real_arg->isSmallInt()) {
    real = RawSmallInt::cast(*real_arg)->value();
  } else if (real_arg->isFloat()) {
    real = RawFloat::cast(*real_arg)->value();
  } else {
    UNIMPLEMENTED("Convert non-numeric to numeric");
  }
  double imag = 0.0;
  if (imag_arg->isSmallInt()) {
    imag = RawSmallInt::cast(*imag_arg)->value();
  } else if (imag_arg->isFloat()) {
    imag = RawFloat::cast(*imag_arg)->value();
  } else {
    UNIMPLEMENTED("Convert non-numeric to numeric");
  }
  return runtime->newComplex(real, imag);
}

}  // namespace python
