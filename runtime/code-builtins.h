#pragma once

#include "runtime.h"

namespace python {

class CodeBuiltins
    : public Builtins<CodeBuiltins, SymbolId::kCode, LayoutId::kCode> {
 public:
  static const BuiltinAttribute kAttributes[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(CodeBuiltins);
};

}  // namespace python
