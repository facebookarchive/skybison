#pragma once

#include "frame.h"
#include "globals.h"
#include "modules.h"
#include "objects.h"
#include "runtime.h"

namespace py {

extern word marshalVersion;

RawObject FUNC(marshal, loads)(Thread* thread, Frame* frame, word nargs);

class MarshalModule {
 public:
  static void initialize(Thread* thread, const Module& module);

 private:
  static const BuiltinFunction kBuiltinFunctions[];
};

}  // namespace py
