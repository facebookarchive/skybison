#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"

namespace python {

class OperatorModule : public ModuleBase<OperatorModule, SymbolId::kOperator> {
 public:
  static void postInitialize(Thread* thread, Runtime* runtime,
                             const Module& module);
};

}  // namespace python
