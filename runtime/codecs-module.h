#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"

namespace python {

class UnderCodecsModule
    : public ModuleBase<UnderCodecsModule, SymbolId::kUnderCodecs> {
 public:
  static const BuiltinMethod kBuiltinMethods[];
  static const char* const kFrozenData;
};

}  // namespace python
