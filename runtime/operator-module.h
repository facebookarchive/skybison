#pragma once

#include "runtime.h"
#include "symbols.h"

namespace py {

class OperatorModule : public ModuleBase<OperatorModule, SymbolId::kOperator> {
 public:
  static const char* const kFrozenData;
};

}  // namespace py
