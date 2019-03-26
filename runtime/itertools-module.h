#pragma once

#include "globals.h"
#include "runtime.h"
#include "symbols.h"

namespace python {

class ItertoolsModule
    : public ModuleBase<ItertoolsModule, SymbolId::kItertools> {
 public:
  static const char* const kFrozenData;
};

}  // namespace python
