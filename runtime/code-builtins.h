#pragma once

#include "runtime.h"

namespace py {

class CodeBuiltins : public Builtins<CodeBuiltins, ID(code), LayoutId::kCode> {
 public:
  static const BuiltinAttribute kAttributes[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(CodeBuiltins);
};

}  // namespace py
