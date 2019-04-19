#include "sys-module.h"
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstring>

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
    {SymbolId::kUnderFdFlush, underFdFlush},
    {SymbolId::kUnderFdWrite, underFdWrite},
    {SymbolId::kUnderGetframeCode, underGetframeCode},
    {SymbolId::kUnderGetframeGlobals, underGetframeGlobals},
    {SymbolId::kUnderGetframeLineno, underGetframeLineno},
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

static RawObject raiseOSErrorFromErrno(Thread* thread) {
  int errno_value = errno;
  // TODO(matthiasb): Pick apropriate OSError subclass.
  return thread->raiseWithFmt(LayoutId::kOSError, "[Errno %d] %s", errno_value,
                              std::strerror(errno_value));
}

static RawObject fileFromFd(Thread* thread, const Object& object,
                            FILE** result) {
  if (!object.isSmallInt()) {
    return thread->raiseRequiresType(object, SymbolId::kInt);
  }
  int fd = SmallInt::cast(*object).value();
  if (fd == kStdoutFd) {
    *result = thread->runtime()->stdoutFile();
  } else if (fd == kStderrFd) {
    *result = thread->runtime()->stderrFile();
  } else {
    HandleScope scope(thread);
    Function function(&scope, thread->currentFrame()->function());
    Str function_name(&scope, function.name());
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "'%S' called with unknown file descriptor",
                                &function_name);
  }
  return NoneType::object();
}

RawObject SysModule::underFdFlush(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object fd(&scope, args.get(0));
  FILE* file;
  Object file_from_fd_result(&scope, fileFromFd(thread, fd, &file));
  if (file_from_fd_result.isError()) return *file_from_fd_result;
  int res = fflush(file);
  if (res != 0) return raiseOSErrorFromErrno(thread);
  return NoneType::object();
}

RawObject SysModule::underFdWrite(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object fd(&scope, args.get(0));
  FILE* file;
  Object file_from_fd_result(&scope, fileFromFd(thread, fd, &file));
  if (file_from_fd_result.isError()) return *file_from_fd_result;

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

  // This is a slow way to write. We eventually expect this whole function to
  // get replaced with something else anyway.
  word length = bytes.length();
  for (word i = 0; i < length; i++) {
    if (fputc(bytes.byteAt(i), file) == EOF) {
      return raiseOSErrorFromErrno(thread);
    }
  }
  return runtime->newIntFromUnsigned(length);
}

class UserVisibleFrameVisitor : public FrameVisitor {
 public:
  UserVisibleFrameVisitor(word depth) : target_depth_(depth) {}

  bool visit(Frame* frame) {
    if (current_depth_ == target_depth_) {
      target_ = frame;
      return false;
    }
    current_depth_++;
    return true;
  }

  Frame* target() { return target_; }

 private:
  word current_depth_ = 0;
  const word target_depth_;
  Frame* target_ = nullptr;
};

static Frame* frameAtDepth(Thread* thread, word depth) {
  DCHECK(depth >= 0, "depth must be nonnegative");
  UserVisibleFrameVisitor visitor(depth + 1);
  thread->visitFrames(&visitor);
  return visitor.target();
}

RawObject SysModule::underGetframeCode(Thread* thread, Frame* frame,
                                       word nargs) {
  Arguments args(frame, nargs);
  DCHECK(args.get(0).isSmallInt(), "depth must be smallint");
  word depth = SmallInt::cast(args.get(0)).value();
  frame = frameAtDepth(thread, depth);
  if (frame == nullptr) {
    return thread->raiseValueErrorWithCStr("call stack is not deep enough");
  }
  if (frame->isNativeFrame()) {
    return NoneType::object();
  }
  return frame->code();
}

RawObject SysModule::underGetframeGlobals(Thread* thread, Frame* frame,
                                          word nargs) {
  Arguments args(frame, nargs);
  DCHECK(args.get(0).isSmallInt(), "depth must be smallint");
  word depth = SmallInt::cast(args.get(0)).value();
  frame = frameAtDepth(thread, depth);
  if (frame == nullptr) {
    return thread->raiseValueErrorWithCStr("call stack is not deep enough");
  }
  return frame->globals();
}

RawObject SysModule::underGetframeLineno(Thread* thread, Frame* frame,
                                         word nargs) {
  Arguments args(frame, nargs);
  DCHECK(args.get(0).isSmallInt(), "depth must be smallint");
  word depth = SmallInt::cast(args.get(0)).value();
  frame = frameAtDepth(thread, depth);
  if (frame == nullptr) {
    return thread->raiseValueErrorWithCStr("call stack is not deep enough");
  }
  if (frame->isNativeFrame()) {
    return SmallInt::fromWord(-1);
  }
  HandleScope scope(thread);
  Code code(&scope, frame->code());
  word pc = frame->virtualPC();
  word lineno = thread->runtime()->codeOffsetToLineNum(thread, code, pc);
  return SmallInt::fromWord(lineno);
}

}  // namespace python
