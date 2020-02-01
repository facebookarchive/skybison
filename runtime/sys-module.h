#pragma once

#include <cstdarg>

#include "frame.h"
#include "globals.h"
#include "modules.h"
#include "objects.h"
#include "runtime.h"

namespace py {

static const int kStdinFd = 0;
static const int kStdoutFd = 1;
static const int kStderrFd = 2;

// Internal equivalents to PySys_Write(Stdout|Stderr): Write a formatted string
// to sys.stdout or sys.stderr, or stdout or stderr if writing to the Python
// streams fails. No more than 1000 characters will be written; if the output is
// truncated, it will be followed by "... truncated".
//
// May be called with a pending exception, which will be saved and restored; any
// exceptions raised while writing to the stream are ignored.
void writeStdout(Thread* thread, const char* format, ...)
    FORMAT_ATTRIBUTE(2, 3);
void writeStdoutV(Thread* thread, const char* format, va_list va);
void writeStderr(Thread* thread, const char* format, ...)
    FORMAT_ATTRIBUTE(2, 3);
void writeStderrV(Thread* thread, const char* format, va_list va);

RawObject initialSysPath(Thread* thread);

RawObject FUNC(sys, exc_info)(Thread* thread, Frame* frame, word nargs);
RawObject FUNC(sys, excepthook)(Thread* thread, Frame* frame, word nargs);

class SysModule {
 public:
  static void initialize(Thread* thread, const Module& module);

 private:
  static const BuiltinFunction kBuiltinFunctions[];
};

}  // namespace py
