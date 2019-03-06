#pragma once

#include "objects.h"
#include "runtime.h"

namespace python {

class Frame;
class Thread;

class UnderWarningsModule
    : public ModuleBase<UnderWarningsModule, SymbolId::kUnderWarnings> {
 public:
  static void postInitialize(Thread* thread, Runtime* runtime,
                             const Module& module);
  static RawObject warn(Thread* thread, Frame* frame, word nargs);

  static const BuiltinMethod kBuiltinMethods[];
};

}  // namespace python
