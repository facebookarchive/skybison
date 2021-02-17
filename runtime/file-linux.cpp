#include "file.h"

#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cerrno>

#include "utils.h"

namespace py {

int File::close(int fd) { return ::close(fd) < 0 ? -errno : 0; }

int File::isatty(int fd) {
  errno = 0;
  int result = ::isatty(fd);
  if (result == 1) return 1;
  int saved_errno = errno;
  return saved_errno == 0 ? 0 : -saved_errno;
}

int File::isDirectory(int fd) {
  struct stat statbuf;
  int result = TEMP_FAILURE_RETRY(::fstat(fd, &statbuf));
  return result < 0 ? -errno : S_ISDIR(statbuf.st_mode);
}

int File::isInheritable(int fd) {
  int result = ::fcntl(fd, F_GETFD);
  return result < 0 ? -errno : result & FD_CLOEXEC;
}

int File::open(const char* path, int flags, int mode) {
  // Set non-inheritable by default
  int result = TEMP_FAILURE_RETRY(::open(path, flags | O_CLOEXEC, mode));
  return result < 0 ? -errno : result;
}

ssize_t File::read(int fd, void* buffer, size_t count) {
  ssize_t result = TEMP_FAILURE_RETRY(::read(fd, buffer, count));
  return result < 0 ? -errno : result;
}

int File::setNoInheritable(int fd) {
  int result = ::ioctl(fd, FIOCLEX, nullptr);
  return result < 0 ? -errno : result;
}

int64_t File::seek(int fd, int64_t offset, int whence) {
  off_t result = ::lseek(fd, offset, whence);
  return result < 0 ? -errno : result;
}

int64_t File::size(int fd) {
  struct stat statbuf;
  int result = TEMP_FAILURE_RETRY(::fstat(fd, &statbuf));
  return result < 0 ? -errno : statbuf.st_size;
}

int File::truncate(int fd, int64_t size) {
  int result = TEMP_FAILURE_RETRY(::ftruncate(fd, size));
  return result < 0 ? -errno : 0;
}

ssize_t File::write(int fd, const void* buffer, size_t size) {
  ssize_t result = TEMP_FAILURE_RETRY(::write(fd, buffer, size));
  return result < 0 ? -errno : result;
}

const word File::kBinaryFlag = 0;
const word File::kCreate = O_CREAT;
const word File::kNoInheritFlag = O_CLOEXEC;
const word File::kStderr = STDERR_FILENO;
const word File::kStdin = STDIN_FILENO;
const word File::kStdout = STDOUT_FILENO;
const word File::kTruncate = O_TRUNC;
const word File::kWriteOnly = O_WRONLY;

}  // namespace py
