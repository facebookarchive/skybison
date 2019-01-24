#pragma once

#include "runtime.h"

namespace python {

class SliceBuiltins {
 public:
  static void initialize(Runtime* runtime);

 private:
  static const BuiltinAttribute kAttributes[];

  DISALLOW_IMPLICIT_CONSTRUCTORS(SliceBuiltins);
};

}  // namespace python
