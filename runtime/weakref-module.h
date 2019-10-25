#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"

namespace py {

class UnderWeakrefModule
    : public ModuleBase<UnderWeakrefModule, SymbolId::kUnderWeakRef> {
 public:
  static const BuiltinType kBuiltinTypes[];
  static const char* const kFrozenData;
};

}  // namespace py
