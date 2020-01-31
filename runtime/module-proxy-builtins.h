#pragma once

#include "runtime.h"

namespace py {

class ModuleProxyBuiltins
    : public Builtins<ModuleProxyBuiltins, ID(module_proxy),
                      LayoutId::kModuleProxy> {
 public:
  static const BuiltinAttribute kAttributes[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(ModuleProxyBuiltins);
};

}  // namespace py
