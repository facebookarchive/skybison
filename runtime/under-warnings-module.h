#pragma once

#include "frame.h"
#include "modules.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace py {

class UnderWarningsModule
    : public ModuleBase<UnderWarningsModule, SymbolId::kUnderWarnings> {
 public:
  static RawObject warn(Thread* thread, Frame* frame, word nargs);

  static const BuiltinMethod kBuiltinMethods[];
  static const char* const kFrozenData;
};

}  // namespace py
