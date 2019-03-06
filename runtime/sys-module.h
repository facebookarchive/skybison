#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"

namespace python {

RawObject initialSysPath(Thread* thread);

class SysModule : public ModuleBase<SysModule, SymbolId::kSys> {
 public:
  static void postInitialize(Thread* thread, Runtime* runtime,
                             const Module& module);
  static RawObject displayhook(Thread* thread, Frame* frame, word nargs);
};

}  // namespace python
