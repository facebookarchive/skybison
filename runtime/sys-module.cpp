#include "sys-module.h"
#include <unistd.h>

#include <cstdio>

#include "builtins-module.h"
#include "exception-builtins.h"
#include "frame.h"
#include "frozen-modules.h"
#include "globals.h"
#include "handles.h"
#include "objects.h"
#include "os.h"
#include "runtime.h"
#include "thread.h"

namespace python {

const char* const SysModule::kFrozenData = kSysModuleData;

static void writeImpl(Thread* thread, std::ostream* stream, const char* format,
                      va_list va) {
  static constexpr int buf_size = 1001;
  char buffer[buf_size];

  HandleScope scope(thread);
  Object type(&scope, thread->pendingExceptionType());
  Object value(&scope, thread->pendingExceptionValue());
  Object tb(&scope, thread->pendingExceptionTraceback());
  thread->clearPendingException();

  // TODO(T41323917): Use sys.stdout/sys.stderr once we have stream support.
  int written = std::vsnprintf(buffer, buf_size, format, va);
  *stream << buffer;
  if (written >= buf_size) *stream << "... truncated";

  thread->clearPendingException();
  thread->setPendingExceptionType(*type);
  thread->setPendingExceptionValue(*value);
  thread->setPendingExceptionTraceback(*tb);
}

void writeStdout(Thread* thread, const char* format, ...) {
  va_list va;
  va_start(va, format);
  writeStdoutV(thread, format, va);
  va_end(va);
}

void writeStdoutV(Thread* thread, const char* format, va_list va) {
  writeImpl(thread, builtinStdout, format, va);
}

void writeStderr(Thread* thread, const char* format, ...) {
  va_list va;
  va_start(va, format);
  writeStderrV(thread, format, va);
  va_end(va);
}

void writeStderrV(Thread* thread, const char* format, va_list va) {
  writeImpl(thread, builtinStderr, format, va);
}

RawObject SysModule::displayhook(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object obj(&scope, args.get(0));
  if (obj.isNoneType()) {
    return NoneType::object();
  }
  UNIMPLEMENTED("sys.displayhook()");
}

RawObject SysModule::excepthook(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  // type argument is ignored
  Object value(&scope, args.get(1));
  Object tb(&scope, args.get(2));
  displayException(thread, value, tb);
  return NoneType::object();
}

}  // namespace python
