#pragma once

#include "modules.h"
#include "symbols.h"

namespace py {

class UnderWeakrefModule
    : public ModuleBase<UnderWeakrefModule, SymbolId::kUnderWeakref> {
 public:
  static const BuiltinType kBuiltinTypes[];
  static const char* const kFrozenData;
};

}  // namespace py
