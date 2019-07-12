#include "space.h"

#include <cstring>

#include "os.h"

namespace python {

Space::Space(word size) {
  raw_ = OS::allocateMemory(size, &size);
  CHECK(raw_ != nullptr, "out of memory");
  start_ = fill_ = reinterpret_cast<uword>(raw_);
  end_ = start_ + size;
}

Space::~Space() {
  if (raw_ != nullptr) {
    OS::freeMemory(raw_, size());
  }
}

void Space::protect() { OS::protectMemory(raw_, size(), OS::kNoAccess); }

void Space::unprotect() { OS::protectMemory(raw_, size(), OS::kReadWrite); }

void Space::reset() {
  std::memset(reinterpret_cast<void*>(start()), 0xFF, size());
  fill_ = start();
}

}  // namespace python
