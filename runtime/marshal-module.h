#pragma once

#include "frame.h"
#include "globals.h"
#include "modules.h"
#include "objects.h"
#include "runtime.h"

namespace py {

extern word marshalVersion;

class MarshalModule : public ModuleBase<MarshalModule, SymbolId::kMarshal> {
 public:
  static RawObject loads(Thread* thread, Frame* frame, word nargs);

  static const BuiltinFunction kBuiltinFunctions[];
  static const char* const kFrozenData;
};

}  // namespace py
