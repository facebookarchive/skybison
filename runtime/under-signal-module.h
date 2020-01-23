#pragma once

#include "frame.h"
#include "globals.h"
#include "modules.h"
#include "runtime.h"
#include "thread.h"

namespace py {

class UnderSignalModule {
 public:
  static void initialize(Thread* thread, const Module& module);
};

}  // namespace py
