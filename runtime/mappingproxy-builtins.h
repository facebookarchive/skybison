#pragma once

#include "runtime.h"

namespace py {

class MappingProxyBuiltins
    : public Builtins<MappingProxyBuiltins, ID(mappingproxy),
                      LayoutId::kMappingProxy> {
 public:
  static const BuiltinAttribute kAttributes[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(MappingProxyBuiltins);
};

}  // namespace py
