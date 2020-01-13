#include "complex-builtins.h"

#include "float-builtins.h"
#include "frame.h"
#include "int-builtins.h"
#include "objects.h"
#include "runtime.h"

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

const BuiltinMethod ComplexBuiltins::kBuiltinMethods[] = {
    {SymbolId::kDunderAdd, dunderAdd},
    {SymbolId::kDunderHash, dunderHash},
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
  Complex self(&scope, complexUnderlying(*self_obj));
  return SmallInt::fromWord(complexHash(*self));
}

RawObject ComplexBuiltins::dunderAdd(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfComplex(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kComplex);
  }
  Complex self(&scope, complexUnderlying(*self_obj));
  Object other_obj(&scope, args.get(1));
  double other_real, other_imag;
  if (runtime->isInstanceOfInt(*other_obj)) {
    Int other_int(&scope, intUnderlying(*other_obj));
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
