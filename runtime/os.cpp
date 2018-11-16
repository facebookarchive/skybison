#include "os.h"
#include "utils.h"

#include <sys/mman.h>

#include <cassert>

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
  int result = ::munmap(ptr, size);
  assert(result != -1);
  return result == 0;
}

} // namespace python
