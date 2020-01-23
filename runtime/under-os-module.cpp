#include "under-os-module.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cerrno>

#include "file.h"
#include "frozen-modules.h"
#include "module-builtins.h"
#include "modules.h"
#include "runtime.h"
#include "symbols.h"

namespace py {

const BuiltinFunction UnderOsModule::kBuiltinFunctions[] = {
    {SymbolId::kClose, close},
    {SymbolId::kFstatSize, fstatSize},
    {SymbolId::kFtruncate, ftruncate},
    {SymbolId::kIsatty, isatty},
    {SymbolId::kIsdir, isdir},
    {SymbolId::kLseek, lseek},
    {SymbolId::kOpen, open},
    {SymbolId::kParseMode, parseMode},
    {SymbolId::kRead, read},
    {SymbolId::kSetNoinheritable, setNoinheritable},
    {SymbolId::kSentinelId, nullptr},
};

RawObject UnderOsModule::close(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  CHECK(args.get(0).isSmallInt(), "fd must be small int");
  word fd = SmallInt::cast(args.get(0)).value();
  int result = File::close(fd);
  if (result < 0) return thread->raiseOSErrorFromErrno(-result);
  return NoneType::object();
}

RawObject UnderOsModule::fstatSize(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  CHECK(args.get(0).isSmallInt(), "fd must be small int");
  word fd = SmallInt::cast(args.get(0)).value();
  int64_t result = File::size(fd);
  if (result < 0) return thread->raiseOSErrorFromErrno(-result);
  return thread->runtime()->newInt(result);
}

RawObject UnderOsModule::ftruncate(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  CHECK(args.get(0).isSmallInt(), "fd must be small int");
  word fd = SmallInt::cast(args.get(0)).value();
  CHECK(args.get(1).isSmallInt(), "size must be small int");
  word size = SmallInt::cast(args.get(1)).value();
  int result = File::truncate(fd, size);
  if (result < 0) return thread->raiseOSErrorFromErrno(-result);
  return NoneType::object();
}

RawObject UnderOsModule::isatty(Thread*, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  CHECK(args.get(0).isSmallInt(), "fd must be small int");
  word fd = SmallInt::cast(args.get(0)).value();
  int result = File::isatty(fd);
  return Bool::fromBool(result < 0 ? false : result);
}

RawObject UnderOsModule::isdir(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  CHECK(args.get(0).isSmallInt(), "fd must be small int");
  word fd = SmallInt::cast(args.get(0)).value();
  int result = File::isDirectory(fd);
  if (result < 0) return thread->raiseOSErrorFromErrno(-result);
  return Bool::fromBool(result);
}

RawObject UnderOsModule::lseek(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  CHECK(args.get(0).isSmallInt(), "fd must be small int");
  word fd = SmallInt::cast(args.get(0)).value();
  CHECK(args.get(1).isInt(), "offset must be int");
  word offset = Int::cast(args.get(1)).asWord();
  CHECK(args.get(2).isSmallInt(), "whence must be int");
  word whence = SmallInt::cast(args.get(2)).value();
  int64_t result = File::seek(fd, offset, whence);
  if (result < 0) return thread->raiseOSErrorFromErrno(-result);
  return thread->runtime()->newInt(result);
}

RawObject UnderOsModule::open(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object path_obj(&scope, args.get(0));
  if (!path_obj.isStr()) {
    UNIMPLEMENTED("_os_open with non-str path");
  }
  Str path(&scope, *path_obj);
  Object flags_obj(&scope, args.get(1));
  CHECK(flags_obj.isSmallInt(), "flags must be small int");
  word flags = SmallInt::cast(*flags_obj).value();
  Object mode_obj(&scope, args.get(2));
  CHECK(mode_obj.isSmallInt(), "mode must be small int");
  word mode = SmallInt::cast(*mode_obj).value();
  Object dir_fd_obj(&scope, args.get(3));
  if (!dir_fd_obj.isNoneType()) {
    UNIMPLEMENTED("_os_open with dir_fd");
  }
  unique_c_ptr<char> path_cstr(path.toCStr());
  int result = File::open(path_cstr.get(), flags, mode);
  if (result < 0) return thread->raiseOSErrorFromErrno(-result);
  return SmallInt::fromWord(result);
}

static bool charInMode(const char c, const Str& mode) {
  for (word i = 0; i < mode.charLength(); i++) {
    if (c == mode.charAt(i)) return true;
  }
  return false;
}

static word flagsFromMode(const Str& mode) {
  word flags = 0;
  bool readable = false, writable = false;
  if (charInMode('x', mode)) {
    writable = true;
    flags = O_EXCL | O_CREAT;
  } else if (charInMode('r', mode)) {
    readable = true;
  } else if (charInMode('w', mode)) {
    writable = true;
    flags = O_CREAT | O_TRUNC;
  } else if (charInMode('a', mode)) {
    writable = true;
    flags = O_APPEND | O_CREAT;
  }
  if (charInMode('+', mode)) {
    readable = writable = true;
  }
  if (readable && writable) {
    flags |= O_RDWR;
  } else if (readable) {
    flags |= O_RDONLY;
  } else {
    flags |= O_WRONLY;
  }
  flags |= File::kBinaryFlag;
  flags |= File::kNoInheritFlag;
  return flags;
}

RawObject UnderOsModule::parseMode(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Str mode(&scope, args.get(0));
  return thread->runtime()->newInt(flagsFromMode(mode));
}

RawObject UnderOsModule::read(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object fd_obj(&scope, args.get(0));
  CHECK(fd_obj.isSmallInt(), "fd must be small int");
  Object count_obj(&scope, args.get(1));
  CHECK(count_obj.isSmallInt(), "count must be small int");
  CHECK(!Int::cast(*count_obj).isNegative(), "count must be non-negative");
  size_t count = SmallInt::cast(*count_obj).value();
  std::unique_ptr<byte[]> buffer(new byte[count]{0});
  int fd = SmallInt::cast(*fd_obj).value();
  ssize_t result = File::read(fd, buffer.get(), count);
  if (result < 0) return thread->raiseOSErrorFromErrno(-result);
  return thread->runtime()->newBytesWithAll(View<byte>(buffer.get(), result));
}

RawObject UnderOsModule::setNoinheritable(Thread* thread, Frame* frame,
                                          word nargs) {
  Arguments args(frame, nargs);
  int fd = SmallInt::cast(args.get(0)).value();
  int result = File::setNoInheritable(fd);
  if (result < 0) return thread->raiseOSErrorFromErrno(-result);
  return NoneType::object();
}

const char* const UnderOsModule::kFrozenData = kUnderOsModuleData;

}  // namespace py
