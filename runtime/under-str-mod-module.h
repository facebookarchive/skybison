#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"

namespace python {

class UnderStrModModule
    : public ModuleBase<UnderStrModModule, SymbolId::kUnderStrMod> {
 public:
  static const char* const kFrozenData;
};

}  // namespace python
