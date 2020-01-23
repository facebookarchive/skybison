#pragma once

#include "modules.h"
#include "runtime.h"

namespace py {

class UnderSignalModule {
 public:
  static const BuiltinFunction kBuiltinFunctions[];
  static const char* const kFrozenData;
};

}  // namespace py
