#pragma once

#include "runtime.h"

namespace python {

class UnderThreadModule
    : public ModuleBase<UnderThreadModule, SymbolId::kUnderThread> {
 public:
  static const char* const kFrozenData;
};

}  // namespace python
