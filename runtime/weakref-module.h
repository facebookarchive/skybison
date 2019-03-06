#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"

namespace python {

class UnderWeakrefModule
    : public ModuleBase<UnderWeakrefModule, SymbolId::kUnderWeakRef> {
 public:
  static void postInitialize(Thread* thread, Runtime* runtime,
                             const Module& module);

  static const BuiltinType kBuiltinTypes[];
};

}  // namespace python
