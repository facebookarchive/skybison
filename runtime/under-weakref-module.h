#pragma once

#include "modules.h"
#include "symbols.h"

namespace py {

class UnderWeakrefModule {
 public:
  static void initialize(Thread* thread, const Module& module);

 private:
  static const BuiltinType kBuiltinTypes[];
};

}  // namespace py
