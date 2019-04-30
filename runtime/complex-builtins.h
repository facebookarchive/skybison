#pragma once

#include "runtime.h"

namespace python {

RawObject complexGetImag(Thread* thread, Frame* frame, word nargs);
RawObject complexGetReal(Thread* thread, Frame* frame, word nargs);

class ComplexBuiltins
    : public Builtins<ComplexBuiltins, SymbolId::kComplex, LayoutId::kComplex> {
 public:
  static void postInitialize(Runtime*, const Type& new_type);

  static RawObject dunderNew(Thread*, Frame*, word);

  static const BuiltinMethod kBuiltinMethods[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(ComplexBuiltins);
};

}  // namespace python
