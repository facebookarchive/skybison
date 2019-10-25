#pragma once

#include "runtime.h"

namespace py {

class UnderThreadModule
    : public ModuleBase<UnderThreadModule, SymbolId::kUnderThread> {
 public:
  static const char* const kFrozenData;
};

}  // namespace py
