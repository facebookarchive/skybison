#pragma once

#include "frame.h"
#include "modules.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace py {

RawObject FUNC(_os, close)(Thread* thread, Frame* frame, word nargs);
RawObject FUNC(_os, fstat_size)(Thread* thread, Frame* frame, word nargs);
RawObject FUNC(_os, ftruncate)(Thread* thread, Frame* frame, word nargs);
RawObject FUNC(_os, isatty)(Thread* thread, Frame* frame, word nargs);
RawObject FUNC(_os, isdir)(Thread* thread, Frame* frame, word nargs);
RawObject FUNC(_os, lseek)(Thread* thread, Frame* frame, word nargs);
RawObject FUNC(_os, open)(Thread* thread, Frame* frame, word nargs);
RawObject FUNC(_os, parse_mode)(Thread* thread, Frame* frame, word nargs);
RawObject FUNC(_os, read)(Thread* thread, Frame* frame, word nargs);
RawObject FUNC(_os, set_noinheritable)(Thread* thread, Frame* frame,
                                       word nargs);

class UnderOsModule {
 public:
  static void initialize(Thread* thread, const Module& module);

 private:
  static const BuiltinFunction kBuiltinFunctions[];
};

}  // namespace py
