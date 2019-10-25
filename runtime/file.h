#pragma once

#include <sys/types.h>

#include "globals.h"

namespace py {

class File {
 public:
  // Return 0. Return -errno on error.
  static int close(int fd);

  // Return 1 if fd is a tty. Return 0 if it is not. Return -errno on error.
  static int isatty(int fd);

  // Return 1 if fd is a directory. Return 0 if it is not. Return -errno on
  // error.
  static int isDirectory(int fd);

  // Return 1 if fd is inheritable. Return 0 if it is not. Return -errno on
  // error.
  static int isInheritable(int fd);

  // Return opened fd. Return -errno on error.
  static int open(const char* path, int flags, int mode);

  // Return number of bytes read. Return -errno on error.
  static ssize_t read(int fd, void* buffer, size_t count);

  // Return 0. Return -errno on error.
  static int setNoInheritable(int fd);

  // Return resulting offset location from start of file. Return -errno on
  // error.
  static int64_t seek(int fd, int64_t offset, int whence);

  // Return file size according to fstat. Return -errno on error.
  static int64_t size(int fd);

  // Return 0. Return -errno on error.
  static int truncate(int fd, int64_t size);

  // This should be non-zero (O_BINARY) on NT.
  static const word kBinaryFlag;

  // This should be O_CLOEXEC on posix and O_NOINHERIT on NT.
  static const word kNoInheritFlag;
};

}  // namespace py
