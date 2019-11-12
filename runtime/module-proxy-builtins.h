#pragma once

#include "runtime.h"

namespace py {

class ModuleProxyBuiltins
    : public Builtins<ModuleProxyBuiltins, SymbolId::kModuleProxy,
                      LayoutId::kModuleProxy> {
 public:
  static const BuiltinAttribute kAttributes[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(ModuleProxyBuiltins);
};

}  // namespace py
