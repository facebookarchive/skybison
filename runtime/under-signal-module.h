#pragma once

#include "runtime.h"

namespace py {

class UnderSignalModule {
 public:
  static const BuiltinMethod kBuiltinMethods[];
  static const char* const kFrozenData;
};

}  // namespace py
