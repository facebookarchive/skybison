#include "os.h"
#include "utils.h"

#include <sys/mman.h>

#include <cassert>

namespace python {

byte* Os::allocateMemory(int size) {
  size = Utils::roundUp(size, 4 * KiB);
  int prot = PROT_READ | PROT_WRITE;
  int flags = MAP_PRIVATE | MAP_ANONYMOUS;
  void* result = ::mmap(nullptr, size, prot, flags, -1, 0);
  assert(result != MAP_FAILED);
  return static_cast<byte*>(result);
}

bool Os::freeMemory(byte* ptr, int size) {
  int result = ::munmap(ptr, size);
  assert(result != -1);
  return result == 0;
}

} // namespace python
