#pragma once

#include "runtime.h"

namespace py {

class LayoutBuiltins
    : public Builtins<LayoutBuiltins, ID(layout), LayoutId::kLayout> {
 public:
  static const BuiltinAttribute kAttributes[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(LayoutBuiltins);
};

}  // namespace py
