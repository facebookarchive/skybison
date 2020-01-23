#pragma once

#include "frame.h"
#include "modules.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace py {

class UnderOsModule {
 public:
  static void initialize(Thread* thread, const Module& module);

  static RawObject close(Thread* thread, Frame* frame, word nargs);
  static RawObject fstatSize(Thread* thread, Frame* frame, word nargs);
  static RawObject ftruncate(Thread* thread, Frame* frame, word nargs);
  static RawObject isatty(Thread* thread, Frame* frame, word nargs);
  static RawObject isdir(Thread* thread, Frame* frame, word nargs);
  static RawObject lseek(Thread* thread, Frame* frame, word nargs);
  static RawObject open(Thread* thread, Frame* frame, word nargs);
  static RawObject parseMode(Thread* thread, Frame* frame, word nargs);
  static RawObject read(Thread* thread, Frame* frame, word nargs);
  static RawObject setNoinheritable(Thread* thread, Frame* frame, word nargs);

 private:
  static const BuiltinFunction kBuiltinFunctions[];
};

}  // namespace py
