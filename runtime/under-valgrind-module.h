#pragma once

#include "frame.h"
#include "modules.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace py {

class UnderValgrindModule {
 public:
  static void initialize(Thread* thread, const Module& module);
};

}  // namespace py
