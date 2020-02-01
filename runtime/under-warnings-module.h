#pragma once

#include "frame.h"
#include "modules.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace py {

RawObject FUNC(_warnings, warn)(Thread* thread, Frame* frame, word nargs);

class UnderWarningsModule {
 public:
  static void initialize(Thread* thread, const Module& module);

 private:
  static const BuiltinFunction kBuiltinFunctions[];
};

}  // namespace py
