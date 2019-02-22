#pragma once

#include "runtime.h"

namespace python {

RawObject complexGetImag(Thread* thread, Frame* frame, word nargs);
RawObject complexGetReal(Thread* thread, Frame* frame, word nargs);

class ComplexBuiltins
    : public Builtins<ComplexBuiltins, SymbolId::kComplex, LayoutId::kComplex> {
 public:
  static RawObject dunderNew(Thread*, Frame*, word);

  static const View<BuiltinMethod> kBuiltinMethods;

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(ComplexBuiltins);
};

}  // namespace python
