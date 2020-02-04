#pragma once

#include "frame.h"
#include "globals.h"
#include "handles.h"
#include "modules.h"
#include "objects.h"
#include "runtime.h"

namespace py {

class UnderBuiltinsModule {
 public:
  static void initialize(Thread* thread, const Module& module);

 private:
  static const SymbolId kIntrinsicIds[];
};

}  // namespace py
