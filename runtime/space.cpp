#include "space.h"

#include "os.h"

namespace python {

Space::Space(intptr_t size) {
  intptr_t rounded = Utils::roundUp(size, Os::kPageSize);
  raw_ = Os::allocateMemory(rounded);
  assert(raw_ != nullptr);
  start_ = fill_ = reinterpret_cast<uword>(raw_);
  end_ = start_ + rounded;
}

Space::~Space() {
  if (raw_ != nullptr) {
    Os::freeMemory(raw_, size());
  }
}

void Space::protect() {
  Os::protectMemory(raw_, size(), Os::kNoAccess);
}

void Space::unprotect() {
  Os::protectMemory(raw_, size(), Os::kReadWrite);
}

} // namespace python
