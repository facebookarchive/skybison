#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"

namespace python {

class NamedtupleModule
    : public ModuleBase<NamedtupleModule, SymbolId::kNamedtuple> {
 public:
  static const char* const kFrozenData;
};

}  // namespace python
