#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"

namespace python {

class UnderIoModule : public ModuleBase<UnderIoModule, SymbolId::kUnderIo> {
 public:
  static void postInitialize(Thread* thread, Runtime* runtime,
                             const Module& module);
  static RawObject underReadFile(Thread* thread, Frame* frame, word nargs);
  static RawObject underReadBytes(Thread* thread, Frame* frame, word nargs);

  static const BuiltinMethod kBuiltinMethods[];
};

}  // namespace python
