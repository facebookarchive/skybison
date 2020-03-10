#pragma once

#include "handles-decl.h"
#include "modules.h"
#include "objects.h"
#include "runtime.h"
#include "symbols.h"
#include "thread.h"

namespace py {

class UnicodedataModule {
 public:
  static void initialize(Thread* thread, const Module& module);
};

}  // namespace py
