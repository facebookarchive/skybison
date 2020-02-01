#pragma once

#include "globals.h"
#include "runtime.h"

namespace py {

class ValueCellBuiltins
    : public Builtins<ValueCellBuiltins, ID(valuecell), LayoutId::kValueCell> {
 public:
  static const BuiltinAttribute kAttributes[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(ValueCellBuiltins);
};

}  // namespace py
