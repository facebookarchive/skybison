#pragma once

#include "runtime.h"

namespace py {

word complexHash(RawObject value);

RawObject METH(complex, __add__)(Thread*, Frame*, word);
RawObject METH(complex, __hash__)(Thread*, Frame*, word);
RawObject METH(complex, __neg__)(Thread*, Frame*, word);
RawObject METH(complex, __pos__)(Thread*, Frame*, word);
RawObject METH(complex, __rsub__)(Thread*, Frame*, word);
RawObject METH(complex, __sub__)(Thread*, Frame*, word);

class ComplexBuiltins
    : public Builtins<ComplexBuiltins, ID(complex), LayoutId::kComplex> {
 public:
  static void postInitialize(Runtime*, const Type& new_type);

  static const BuiltinMethod kBuiltinMethods[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(ComplexBuiltins);
};

}  // namespace py
