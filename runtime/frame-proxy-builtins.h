#pragma once

#include "globals.h"
#include "runtime.h"

namespace py {

class FrameProxyBuiltins
    : public Builtins<FrameProxyBuiltins, ID(frame), LayoutId::kFrameProxy> {
 public:
  static const BuiltinAttribute kAttributes[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(FrameProxyBuiltins);
};

}  // namespace py
