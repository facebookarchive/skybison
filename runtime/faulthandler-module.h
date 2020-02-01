#pragma once

#include "frame.h"
#include "globals.h"
#include "modules.h"
#include "objects.h"
#include "runtime.h"

namespace py {

RawObject FUNC(faulthandler, _read_null)(Thread* thread, Frame* frame,
                                         word nargs);
RawObject FUNC(faulthandler, _sigabrt)(Thread* thread, Frame* frame,
                                       word nargs);
RawObject FUNC(faulthandler, _sigfpe)(Thread* thread, Frame* frame, word nargs);
RawObject FUNC(faulthandler, _sigsegv)(Thread* thread, Frame* frame,
                                       word nargs);
RawObject FUNC(faulthandler, disable)(Thread* thread, Frame* frame, word nargs);
RawObject FUNC(faulthandler, dump_traceback)(Thread* thread, Frame* frame,
                                             word nargs);
RawObject FUNC(faulthandler, enable)(Thread* thread, Frame* frame, word nargs);
RawObject FUNC(faulthandler, is_enabled)(Thread* thread, Frame* frame,
                                         word nargs);

class FaulthandlerModule {
 public:
  static void initialize(Thread* thread, const Module& module);

 private:
  static const BuiltinFunction kBuiltinFunctions[];
};

}  // namespace py
