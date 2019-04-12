#pragma once

#include <cstdarg>

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"

namespace python {

static const word kStdoutFd = 100;
static const word kStderrFd = 101;

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

class SysModule {
 public:
  static RawObject displayhook(Thread* thread, Frame* frame, word nargs);
  static RawObject excInfo(Thread* thread, Frame* frame, word nargs);
  static RawObject excepthook(Thread* thread, Frame* frame, word nargs);
  static RawObject underFdWrite(Thread* thread, Frame* frame, word nargs);
  static RawObject underFdFlush(Thread* thread, Frame* frame, word nargs);

  static const BuiltinMethod kBuiltinMethods[];
  static const char* const kFrozenData;
};

}  // namespace python
