#include "sys-module.h"

#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstring>

#include "builtins-module.h"
#include "bytes-builtins.h"
#include "dict-builtins.h"
#include "exception-builtins.h"
#include "frame.h"
#include "frozen-modules.h"
#include "globals.h"
#include "handles.h"
#include "int-builtins.h"
#include "module-builtins.h"
#include "objects.h"
#include "os.h"
#include "runtime.h"
#include "thread.h"

namespace py {

const char* const SysModule::kFrozenData = kSysModuleData;

static void writeImpl(Thread* thread, const Object& file, FILE* fallback_fp,
                      const char* format, va_list va) {
  HandleScope scope(thread);
  Object type(&scope, thread->pendingExceptionType());
  Object value(&scope, thread->pendingExceptionValue());
  Object tb(&scope, thread->pendingExceptionTraceback());
  thread->clearPendingException();

  static const char truncated[] = "... truncated";
  static constexpr int message_size = 1001;
  char buffer[message_size + sizeof(truncated) - 1];
  int written = std::vsnprintf(buffer, message_size, format, va);
  CHECK(written >= 0, "vsnprintf failed");
  if (written >= message_size) {
    std::memcpy(&buffer[message_size - 1], truncated, sizeof(truncated));
    written = message_size + sizeof(truncated) - 2;
  }
  Str str(&scope, thread->runtime()->newStrWithAll(
                      View<byte>(reinterpret_cast<byte*>(buffer), written)));
  if (file.isNoneType() ||
      thread->invokeMethod2(file, SymbolId::kWrite, str).isError()) {
    fwrite(buffer, 1, written, fallback_fp);
  }

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
  HandleScope scope(thread);
  ValueCell sys_stdout_cell(&scope, thread->runtime()->sysStdout());
  Object sys_stdout(&scope, NoneType::object());
  if (!sys_stdout_cell.isUnbound()) {
    sys_stdout = sys_stdout_cell.value();
  }
  writeImpl(thread, sys_stdout, stdout, format, va);
}

void writeStderr(Thread* thread, const char* format, ...) {
  va_list va;
  va_start(va, format);
  writeStderrV(thread, format, va);
  va_end(va);
}

void writeStderrV(Thread* thread, const char* format, va_list va) {
  HandleScope scope(thread);
  ValueCell sys_stderr_cell(&scope, thread->runtime()->sysStderr());
  Object sys_stderr(&scope, NoneType::object());
  if (!sys_stderr_cell.isUnbound()) {
    sys_stderr = sys_stderr_cell.value();
  }
  writeImpl(thread, sys_stderr, stderr, format, va);
}

const BuiltinMethod SysModule::kBuiltinMethods[] = {
    {SymbolId::kExcInfo, excInfo},
    {SymbolId::kExcepthook, excepthook},
    {SymbolId::kSentinelId, nullptr},
};

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

}  // namespace py
