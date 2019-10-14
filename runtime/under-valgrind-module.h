#pragma once

#include "objects.h"
#include "runtime.h"

namespace python {

class Frame;
class Thread;

class UnderValgrindModule
    : public ModuleBase<UnderValgrindModule, SymbolId::kUnderValgrind> {
 public:
  static RawObject callgrindDumpStatsAt(Thread* thread, Frame* frame,
                                        word nargs);

  static const BuiltinMethod kBuiltinMethods[];
  static const char* const kFrozenData;
};

}  // namespace python
