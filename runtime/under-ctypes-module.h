#pragma once

#include "handles.h"
#include "modules.h"
#include "runtime.h"
#include "thread.h"

namespace py {

class UnderCtypesModule {
 public:
  static void initialize(Thread* thread, const Module& module);
};

}  // namespace py
