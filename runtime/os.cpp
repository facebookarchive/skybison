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

namespace py {

byte* OS::allocateMemory(word size, word* allocated_size) {
  size = Utils::roundUp(size, kPageSize);
  if (allocated_size != nullptr) *allocated_size = size;
  int prot = PROT_READ | PROT_WRITE;
  int flags = MAP_PRIVATE | MAP_ANONYMOUS;
  void* result = ::mmap(nullptr, size, prot, flags, -1, 0);
  CHECK(result != MAP_FAILED, "mmap failure");
  return static_cast<byte*>(result);
}

bool OS::access(const char* path, int mode) {
  return ::access(path, mode) == 0;
}

int OS::pageSize() { return ::sysconf(_SC_PAGESIZE); }

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
    case kReadExecute:
      prot = PROT_READ | PROT_EXEC;
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

void* OS::mmap(void* addr, word size, int prot, int flags, int fd,
               off_t offset) {
  if (fd == -1) {
    flags |= MAP_ANONYMOUS;
  } else {
    UNIMPLEMENTED("Should reach the _unimplemented in managed code");
  }
  size = Utils::roundUp(size, kPageSize);
  DCHECK(size >= 0, "invalid size %ld", size);
  void* result = ::mmap(addr, size, prot, flags, fd, offset);
  CHECK(result != MAP_FAILED, "mmap failure");
  return static_cast<byte*>(result);
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

char* OS::readFile(FILE* fp, word* len_out) {
  std::fseek(fp, 0, SEEK_END);
  word length = std::ftell(fp);
  CHECK(length != -1, "fseek failure");
  std::fseek(fp, 0, SEEK_SET);
  std::unique_ptr<char[]> buffer(new char[length]);
  {
    word result;
    do {
      result = std::fread(buffer.get(), 1, length, fp);
    } while (result == EOF && errno == EINTR);
    if (result != length) {
      fprintf(stderr, "file read error: %s\n", std::strerror(errno));
      return nullptr;
    }
  }
  *len_out = length;
  return buffer.release();
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

bool OS::dirExists(const char* dir) {
  struct stat st;
  int err = ::stat(dir, &st);
  if (err == 0 && (st.st_mode & S_IFDIR)) {
    return true;
  }
  if (errno != 0 && errno != ENOENT) {
    std::fprintf(stderr, "stat error: %s %s\n", dir, std::strerror(errno));
  }
  return false;
}

bool OS::fileExists(const char* file) {
  struct stat st;
  int err = ::stat(file, &st);
  if (err == 0 && S_ISREG(st.st_mode)) {
    return true;
  }
  if (errno != 0 && errno != ENOENT) {
    std::fprintf(stderr, "stat error: %s %s\n", file, std::strerror(errno));
  }
  return false;
}

char* OS::readLink(const char* path) {
  char* buffer = nullptr;
  for (ssize_t size = 64;; size *= 2) {
    buffer = reinterpret_cast<char*>(std::realloc(buffer, size));
    CHECK(buffer != nullptr, "out of memory");
    ssize_t res = ::readlink(path, buffer, size);
    if (res == -1) {
      std::free(buffer);
      return nullptr;
    }
    if (res < size - 1) {
      buffer[res] = '\0';
      return buffer;
    }
  }
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

}  // namespace py
