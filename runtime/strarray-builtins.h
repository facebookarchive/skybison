#pragma once

#include "runtime.h"

namespace py {

class StrArrayBuiltins
    : public Builtins<StrArrayBuiltins, ID(_strarray), LayoutId::kStrArray> {
 public:
  static const BuiltinAttribute kAttributes[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(StrArrayBuiltins);
};

}  // namespace py
