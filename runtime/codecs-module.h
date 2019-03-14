#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"

namespace python {

class UnderCodecsModule
    : public ModuleBase<UnderCodecsModule, SymbolId::kUnderCodecs> {
 public:
  static void postInitialize(Thread* thread, Runtime* runtime,
                             const Module& module);

  static const BuiltinMethod kBuiltinMethods[];
};

}  // namespace python
