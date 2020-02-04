#pragma once

#include "runtime.h"

namespace py {

class SliceBuiltins
    : public Builtins<SliceBuiltins, ID(slice), LayoutId::kSlice> {
 public:
  static const BuiltinAttribute kAttributes[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(SliceBuiltins);
};

}  // namespace py
