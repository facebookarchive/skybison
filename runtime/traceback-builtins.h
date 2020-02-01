#pragma once

#include "runtime.h"

namespace py {

class TracebackBuiltins
    : public Builtins<TracebackBuiltins, ID(traceback), LayoutId::kTraceback> {
 public:
  static const BuiltinAttribute kAttributes[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(TracebackBuiltins);
};

}  // namespace py
