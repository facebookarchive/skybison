#include "file.h"

#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cerrno>

#include "utils.h"

namespace python {

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
  return ::fstat(fd, &statbuf) < 0 ? -errno : S_ISDIR(statbuf.st_mode);
}

int File::isInheritable(int fd) {
  int result = ::fcntl(fd, F_GETFD);
  return result < 0 ? -errno : result & FD_CLOEXEC;
}

int File::open(const char* path, int flags, int mode) {
  int result;
  do {
    errno = 0;
    // Set non-inheritable by default
    result = ::open(path, flags | O_CLOEXEC, mode);
  } while (result == -1 && errno == EINTR);
  DCHECK(errno != EINTR, "this should have been handled in the loop");
  return result < 0 ? -errno : result;
}

ssize_t File::read(int fd, void* buffer, size_t count) {
  int result;
  do {
    errno = 0;
    result = ::read(fd, buffer, count);
  } while (result == -1 && errno == EINTR);
  DCHECK(errno != EINTR, "this should have been handled in the loop");
  return result < 0 ? -errno : result;
}

int File::setNoInheritable(int fd) {
  int result = ::ioctl(fd, FIOCLEX, nullptr);
  return result < 0 ? -errno : result;
}

int64_t File::seek(int fd, int64_t offset, int whence) {
  int result = ::lseek(fd, offset, whence);
  return result < 0 ? -errno : result;
}

int64_t File::size(int fd) {
  struct stat statbuf;
  int result = ::fstat(fd, &statbuf);
  return result < 0 ? -errno : statbuf.st_size;
}

int File::truncate(int fd, int64_t size) {
  int result = ::ftruncate(fd, size);
  return result < 0 ? -errno : 0;
}

const word File::kBinaryFlag = 0;
const word File::kNoInheritFlag = O_CLOEXEC;

}  // namespace python
