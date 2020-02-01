#pragma once

#include "frame.h"
#include "modules.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace py {

RawObject FUNC(_valgrind, callgrind_dump_stats)(Thread* thread, Frame* frame,
                                                word nargs);
RawObject FUNC(_valgrind, callgrind_start_instrumentation)(Thread* thread,
                                                           Frame* frame,
                                                           word nargs);
RawObject FUNC(_valgrind, callgrind_stop_instrumentation)(Thread* thread,
                                                          Frame* frame,
                                                          word nargs);
RawObject FUNC(_valgrind, callgrind_zero_stats)(Thread* thread, Frame* frame,
                                                word nargs);

class UnderValgrindModule {
 public:
  static void initialize(Thread* thread, const Module& module);

 private:
  static const BuiltinFunction kBuiltinFunctions[];
};

}  // namespace py
