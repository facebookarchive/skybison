#pragma once

#include "runtime.h"

namespace py {

word complexHash(RawObject value);

class ComplexBuiltins
    : public Builtins<ComplexBuiltins, ID(complex), LayoutId::kComplex> {
 public:
  static void postInitialize(Runtime*, const Type& new_type);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(ComplexBuiltins);
};

}  // namespace py
