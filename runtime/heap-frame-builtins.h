#pragma once

#include "globals.h"
#include "runtime.h"

namespace py {

class HeapFrameBuiltins
    : public Builtins<HeapFrameBuiltins, ID(frame), LayoutId::kHeapFrame> {
 public:
  static const BuiltinAttribute kAttributes[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(HeapFrameBuiltins);
};

}  // namespace py
