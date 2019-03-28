#pragma once

#include "globals.h"
#include "runtime.h"
#include "symbols.h"

namespace python {

class UnderFunctoolsModule
    : public ModuleBase<UnderFunctoolsModule, SymbolId::kUnderFunctools> {
 public:
  static const char* const kFrozenData;
};

}  // namespace python
