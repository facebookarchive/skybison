#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"

namespace python {

class FaulthandlerModule
    : public ModuleBase<FaulthandlerModule, SymbolId::kFaulthandler> {
 public:
  static RawObject dumpTraceback(Thread* thread, Frame* frame, word nargs);

  static const BuiltinMethod kBuiltinMethods[];
  static const char* const kFrozenData;
};

}  // namespace python
