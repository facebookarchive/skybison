#include "os.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <cassert>
#include <cerrno>

#include "utils.h"

namespace python {

byte* Os::allocateMemory(word size) {
  size = Utils::roundUp(size, 4 * KiB);
  int prot = PROT_READ | PROT_WRITE;
  int flags = MAP_PRIVATE | MAP_ANONYMOUS;
  void* result = ::mmap(nullptr, size, prot, flags, -1, 0);
  assert(result != MAP_FAILED);
  return static_cast<byte*>(result);
}

bool Os::protectMemory(byte* address, word size, Protection mode) {
  assert(size >= 0);
  int prot;
  switch (mode) {
    case kNoAccess:
      prot = PROT_NONE;
      break;
    case kReadWrite:
      prot = PROT_READ | PROT_WRITE;
      break;
    default:
      assert(0);
  }
  int result = mprotect(reinterpret_cast<void*>(address), size, prot);
  assert(result == 0);
  return result == 0;
}

bool Os::freeMemory(byte* ptr, word size) {
  assert(size >= 0);
  int result = ::munmap(ptr, size);
  assert(result != -1);
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

  int get() const {
    return fd_;
  }

 private:
  int fd_;
  DISALLOW_COPY_AND_ASSIGN(ScopedFd);
};

bool Os::secureRandom(byte* ptr, word size) {
  assert(size >= 0);
  ScopedFd fd(open("/dev/urandom", O_RDONLY));
  if (fd.get() == -1) {
    return false;
  }
  do {
    word result;
    do {
      result = read(fd.get(), ptr, size);
    } while (result == -1 && errno == EINTR);
    if (result == -1) {
      break;
    }
    size -= result;
    ptr += result;
  } while (size > 0);
  return size == 0;
}

} // namespace python
