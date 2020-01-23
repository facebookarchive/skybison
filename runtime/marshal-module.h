#pragma once

#include "frame.h"
#include "globals.h"
#include "modules.h"
#include "objects.h"
#include "runtime.h"

namespace py {

extern word marshalVersion;

class MarshalModule {
 public:
  static void initialize(Thread* thread, const Module& module);

  static RawObject loads(Thread* thread, Frame* frame, word nargs);

 private:
  static const BuiltinFunction kBuiltinFunctions[];
};

}  // namespace py
