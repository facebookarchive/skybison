#pragma once

#include "frame.h"
#include "globals.h"
#include "modules.h"
#include "objects.h"
#include "runtime.h"

namespace py {

class UnderIoModule {
 public:
  static void initialize(Thread* thread, const Module& module);

 private:
  static const BuiltinType kBuiltinTypes[];
};

void initializeUnderIOTypes(Thread* thread);

}  // namespace py
