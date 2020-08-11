#pragma once

#include <cstdarg>

#include "globals.h"
#include "handles-decl.h"
#include "thread.h"

namespace py {

enum class SysFlag {
  kDebug,
  kInspect,
  kInteractive,
  kOptimize,
  kDontWriteBytecode,
  kNoUserSite,
  kNoSite,
  kIgnoreEnvironment,
  kVerbose,
  kBytesWarning,
  kQuiet,
  kHashRandomization,
  kIsolated,
  kDevMode,
  kUTF8Mode,
  kNumFlags,
};

void initializeRuntimePaths(Thread* thread);

// Initializes sys module with data that can vary between startups.  This must
// be called after the runtime constructor and before Runtime::initialize().
RawObject initializeSys(Thread* thread, const Str& executable,
                        const List& python_path, const Tuple& flags_data,
                        const List& warnoptions);

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

}  // namespace py
