#pragma once

#include "frame.h"
#include "modules.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace py {

class UnderWarningsModule {
 public:
  static void initialize(Thread* thread, const Module& module);

  static RawObject warn(Thread* thread, Frame* frame, word nargs);

 private:
  static const BuiltinFunction kBuiltinFunctions[];
};

}  // namespace py
