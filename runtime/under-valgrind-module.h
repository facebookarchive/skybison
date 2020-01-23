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

  static RawObject callgrindDumpStats(Thread* thread, Frame* frame, word nargs);
  static RawObject callgrindStartInstrumentation(Thread* thread, Frame* frame,
                                                 word nargs);
  static RawObject callgrindStopInstrumentation(Thread* thread, Frame* frame,
                                                word nargs);
  static RawObject callgrindZeroStats(Thread* thread, Frame* frame, word nargs);

 private:
  static const BuiltinFunction kBuiltinFunctions[];
};

}  // namespace py
