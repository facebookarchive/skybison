#pragma once

#include "runtime.h"

namespace py {

class StrArrayBuiltins
    : public Builtins<StrArrayBuiltins, ID(_strarray), LayoutId::kStrArray> {
 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(StrArrayBuiltins);
};

}  // namespace py
