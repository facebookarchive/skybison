#pragma once

#include "objects.h"
#include "runtime.h"

namespace python {

class Frame;
class Thread;

class UnderWarningsModule
    : public ModuleBase<UnderWarningsModule, SymbolId::kUnderWarnings> {
 public:
  static RawObject warn(Thread* thread, Frame* frame, word nargs);

  static const BuiltinMethod kBuiltinMethods[];
  static const char* const kFrozenData;
};

}  // namespace python
