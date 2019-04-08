#include "sys-module.h"
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <iostream>

#include "builtins-module.h"
#include "exception-builtins.h"
#include "file.h"
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

const BuiltinMethod SysModule::kBuiltinMethods[] = {
    {SymbolId::kExcInfo, excInfo},
    {SymbolId::kExcepthook, excepthook},
    {SymbolId::kUnderFdWrite, underFdWrite},
    {SymbolId::kSentinelId, nullptr},
};

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

RawObject SysModule::excInfo(Thread* thread, Frame* /* frame */,
                             word /* nargs */) {
  HandleScope scope(thread);
  Tuple result(&scope, thread->runtime()->newTuple(3));
  if (thread->hasCaughtException()) {
    result.atPut(0, thread->caughtExceptionType());
    result.atPut(1, thread->caughtExceptionValue());
    result.atPut(2, thread->caughtExceptionTraceback());
  } else {
    result.atPut(0, RawNoneType::object());
    result.atPut(1, RawNoneType::object());
    result.atPut(2, RawNoneType::object());
  }
  return *result;
}

RawObject SysModule::underFdWrite(Thread* thread, Frame* frame_frame,
                                  word nargs) {
  Arguments args(frame_frame, nargs);
  HandleScope scope(thread);
  Object fd_obj(&scope, args.get(0));
  if (!fd_obj.isSmallInt()) {
    return thread->raiseRequiresType(fd_obj, SymbolId::kInt);
  }
  int fd = SmallInt::cast(*fd_obj).value();

  Runtime* runtime = thread->runtime();
  Object bytes_obj(&scope, args.get(1));
  // TODO(matthiasb): Remove this and use str.encode() in sys.py once it is
  // avilable.
  if (runtime->isInstanceOfStr(*bytes_obj)) {
    Str str(&scope, *bytes_obj);
    word str_length = str.length();
    std::unique_ptr<byte[]> buf(new byte[str_length]);
    for (word i = 0; i < str_length; i++) {
      buf[i] = str.charAt(i);
    }
    bytes_obj = runtime->newBytesWithAll(View<byte>(buf.get(), str_length));
  }

  if (!runtime->isInstanceOfBytes(*bytes_obj)) {
    return thread->raiseRequiresType(bytes_obj, SymbolId::kBytes);
  }
  Bytes bytes(&scope, *bytes_obj);

  FILE* out;
  if (fd == kStdoutFd) {
    out = runtime->stdoutFile();
  } else if (fd == kStderrFd) {
    out = runtime->stderrFile();
  } else {
    return thread->raiseValueErrorWithCStr(
        "_fd_write called with unknown file descriptor");
  }

  // This is a slow way to write. We eventually expect this whole function to
  // get replaced with something else anyway.
  word length = bytes.length();
  for (word i = 0; i < length; i++) {
    if (fputc(bytes.byteAt(i), out) == EOF) {
      int errno_value = errno;
      // TODO(matthiasb): Pick apropriate OSError subclass.
      return thread->raiseWithFmt(LayoutId::kOSError, "[Errno %d] %s",
                                  errno_value, std::strerror(errno_value));
    }
  }
  return runtime->newIntFromUnsigned(length);
}

}  // namespace python
