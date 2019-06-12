#include "os.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "globals.h"
#include "utils.h"

namespace python {

byte* OS::allocateMemory(word size) {
  size = Utils::roundUp(size, 4 * kKiB);
  int prot = PROT_READ | PROT_WRITE;
  int flags = MAP_PRIVATE | MAP_ANONYMOUS;
  void* result = ::mmap(nullptr, size, prot, flags, -1, 0);
  CHECK(result != MAP_FAILED, "mmap failure");
  return static_cast<byte*>(result);
}

bool OS::protectMemory(byte* address, word size, Protection mode) {
  DCHECK(size >= 0, "invalid size %ld", size);
  int prot;
  switch (mode) {
    case kNoAccess:
      prot = PROT_NONE;
      break;
    case kReadWrite:
      prot = PROT_READ | PROT_WRITE;
      break;
    default:
      std::abort();
  }
  int result = mprotect(address, size, prot);
  CHECK(result == 0, "mprotect failure");
  return result == 0;
}

bool OS::freeMemory(byte* ptr, word size) {
  DCHECK(size >= 0, "invalid size %ld", size);
  int result = ::munmap(ptr, size);
  CHECK(result != -1, "munmap failure");
  return result == 0;
}

class ScopedFd {
 public:
  explicit ScopedFd(int fd) : fd_(fd) {}

  ~ScopedFd() {
    if (fd_ != -1) {
      close(fd_);
    }
  }

  int get() const { return fd_; }

 private:
  int fd_;
  DISALLOW_COPY_AND_ASSIGN(ScopedFd);
};

bool OS::secureRandom(byte* ptr, word size) {
  DCHECK(size >= 0, "invalid size %ld", size);
  ScopedFd fd(::open("/dev/urandom", O_RDONLY));
  if (fd.get() == -1) {
    return false;
  }
  do {
    word result;
    do {
      result = ::read(fd.get(), ptr, size);
    } while (result == -1 && errno == EINTR);
    if (result == -1) {
      break;
    }
    size -= result;
    ptr += result;
  } while (size > 0);
  return size == 0;
}

char* OS::readFile(const char* filename, word* len_out) {
  ScopedFd fd(::open(filename, O_RDONLY));
  if (fd.get() == -1) {
    fprintf(stderr, "open error: %s %s\n", filename, std::strerror(errno));
  }
  CHECK(fd.get() != -1, "get failure");
  word length = ::lseek(fd.get(), 0, SEEK_END);
  CHECK(length != -1, "lseek failure");
  ::lseek(fd.get(), 0, SEEK_SET);
  char* buffer = new char[length + 1];
  {
    word result;
    do {
      result = ::read(fd.get(), buffer, length);
    } while (result == -1 && errno == EINTR);
  }
  if (len_out != nullptr) {
    *len_out = length;
  }
  buffer[length] = '\0';
  return buffer;
}

void OS::writeFileExcl(const char* filename, const char* contents, word len) {
  ScopedFd fd(::open(filename, O_RDWR | O_CREAT | O_EXCL, 0644));
  CHECK(fd.get() != -1, "get failure");
  if (len < 0) {
    len = strlen(contents);
  }
  word result;
  do {
    result = ::write(fd.get(), contents, len);
  } while (result == -1 && errno == EINTR);
  CHECK(result == len, "Incomplete write");
}

char* OS::temporaryDirectory(const char* prefix) {
  const char* tmpdir = std::getenv("TMPDIR");
  if (tmpdir == nullptr) {
    tmpdir = "/tmp";
  }
  const char format[] = "%s/%s.XXXXXXXX";
  word length = std::snprintf(nullptr, 0, format, tmpdir, prefix);
  char* buffer = new char[length];
  std::snprintf(buffer, length, format, tmpdir, prefix);
  char* result = ::mkdtemp(buffer);
  CHECK(result != nullptr, "mkdtemp failure");
  return result;
}

char* OS::temporaryFile(const char* prefix, int* fd) {
  const char* tmpdir = std::getenv("TMPDIR");
  if (tmpdir == nullptr) {
    tmpdir = "/tmp";
  }
  const char format[] = "%s/%s.XXXXXXXX";
  word length = std::snprintf(nullptr, 0, format, tmpdir, prefix) + 1;
  char* buffer = new char[length];
  std::snprintf(buffer, length, format, tmpdir, prefix);
  *fd = mkstemp(buffer);
  DCHECK(*fd != -1, "Temporary file could not be created");
  return buffer;
}

const char* OS::getenv(const char* var) { return ::getenv(var); }

bool OS::dirExists(const char* dir) {
  struct stat st;
  int err = ::stat(dir, &st);
  if (err == 0 && (st.st_mode & S_IFDIR)) {
    return true;
  }
  if (errno != ENOENT) {
    fprintf(stderr, "stat error: %s %s\n", dir, std::strerror(errno));
  }
  return false;
}

bool OS::fileExists(const char* file) {
  struct stat st;
  int err = ::stat(file, &st);
  if (err == 0) {
    return true;
  }
  if (errno != ENOENT) {
    fprintf(stderr, "stat error: %s %s\n", file, std::strerror(errno));
  }
  return false;
}

double OS::currentTime() {
  timespec ts;
  int err = clock_gettime(CLOCK_REALTIME, &ts);
  CHECK(!err, "clock_gettime failure");

  if (ts.tv_nsec == 0) {
    return static_cast<double>(ts.tv_sec);
  }

  int64_t nanoseconds = ts.tv_sec * kNanosecondsPerSecond;
  nanoseconds += ts.tv_nsec;
  double result = static_cast<double>(nanoseconds);
  result /= kNanosecondsPerSecond;
  return result;
}

}  // namespace python
