#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cerrno>

#include "builtins.h"
#include "file.h"
#include "module-builtins.h"
#include "modules.h"
#include "os.h"
#include "runtime.h"
#include "symbols.h"

namespace py {

RawObject FUNC(_os, access)(Thread*, Arguments args) {
  CHECK(args.get(0).isStr(), "path must be str");
  unique_c_ptr<char> path(Str::cast(args.get(0)).toCStr());
  CHECK(args.get(1).isSmallInt(), "mode must be int");
  int result = OS::access(path.get(), SmallInt::cast(args.get(1)).value());
  return Bool::fromBool(result);
}

RawObject FUNC(_os, close)(Thread* thread, Arguments args) {
  CHECK(args.get(0).isSmallInt(), "fd must be small int");
  word fd = SmallInt::cast(args.get(0)).value();
  int result = File::close(fd);
  if (result < 0) return thread->raiseOSErrorFromErrno(-result);
  return NoneType::object();
}

RawObject FUNC(_os, fstat_size)(Thread* thread, Arguments args) {
  CHECK(args.get(0).isSmallInt(), "fd must be small int");
  word fd = SmallInt::cast(args.get(0)).value();
  int64_t result = File::size(fd);
  if (result < 0) return thread->raiseOSErrorFromErrno(-result);
  return thread->runtime()->newInt(result);
}

RawObject FUNC(_os, ftruncate)(Thread* thread, Arguments args) {
  CHECK(args.get(0).isSmallInt(), "fd must be small int");
  word fd = SmallInt::cast(args.get(0)).value();
  CHECK(args.get(1).isSmallInt(), "size must be small int");
  word size = SmallInt::cast(args.get(1)).value();
  int result = File::truncate(fd, size);
  if (result < 0) return thread->raiseOSErrorFromErrno(-result);
  return NoneType::object();
}

RawObject FUNC(_os, isatty)(Thread*, Arguments args) {
  CHECK(args.get(0).isSmallInt(), "fd must be small int");
  word fd = SmallInt::cast(args.get(0)).value();
  int result = File::isatty(fd);
  return Bool::fromBool(result < 0 ? false : result);
}

RawObject FUNC(_os, isdir)(Thread* thread, Arguments args) {
  CHECK(args.get(0).isSmallInt(), "fd must be small int");
  word fd = SmallInt::cast(args.get(0)).value();
  int result = File::isDirectory(fd);
  if (result < 0) return thread->raiseOSErrorFromErrno(-result);
  return Bool::fromBool(result);
}

RawObject FUNC(_os, lseek)(Thread* thread, Arguments args) {
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

RawObject FUNC(_os, open)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
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
  Object path_obj(&scope, args.get(0));
  int result;
  if (path_obj.isStr()) {
    unique_c_ptr<char> path_cstr(Str::cast(*path_obj).toCStr());
    result = File::open(path_cstr.get(), flags, mode);
  } else if (path_obj.isBytes()) {
    unique_c_ptr<char> path_cstr(Bytes::cast(*path_obj).toCStr());
    result = File::open(path_cstr.get(), flags, mode);
  } else {
    UNIMPLEMENTED("_os_open with non-str/bytes path");
  }
  if (result < 0) return thread->raiseOSErrorFromErrno(-result);
  return SmallInt::fromWord(result);
}

static bool charInMode(const char c, const Str& mode) {
  for (word i = 0; i < mode.length(); i++) {
    if (c == mode.byteAt(i)) return true;
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

RawObject FUNC(_os, parse_mode)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Str mode(&scope, args.get(0));
  return thread->runtime()->newInt(flagsFromMode(mode));
}

RawObject FUNC(_os, read)(Thread* thread, Arguments args) {
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

RawObject FUNC(_os, set_noinheritable)(Thread* thread, Arguments args) {
  int fd = SmallInt::cast(args.get(0)).value();
  int result = File::setNoInheritable(fd);
  if (result < 0) return thread->raiseOSErrorFromErrno(-result);
  return NoneType::object();
}

}  // namespace py
