#pragma once

#include "frame.h"
#include "globals.h"
#include "modules.h"
#include "objects.h"
#include "runtime.h"

namespace py {

void importAcquireLock(Thread* thread);
bool importReleaseLock(Thread* thread);

class UnderImpModule {
 public:
  static void initialize(Thread* thread, const Module& module);
};

}  // namespace py
