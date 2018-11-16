#include "objects.h"
#include "runtime.h"

#include <cassert>
#include <cstring>

namespace python {

// String

bool LargeString::equals(Object* that) {
  if (this == that) {
    return true;
  }
  if (!that->isLargeString()) {
    return false;
  }
  auto* thatStr = LargeString::cast(that);
  if (length() != thatStr->length()) {
    return false;
  }
  auto s1 = reinterpret_cast<void*>(address());
  auto s2 = reinterpret_cast<void*>(thatStr->address());
  return std::memcmp(s1, s2, length()) == 0;
}

bool LargeString::equalsCString(const char* c_string) {
  const char* cp = c_string;
  for (word i = 0; i < length(); i++, cp++) {
    char ch = *cp;
    if (ch == '\0' || ch != charAt(i)) {
      return false;
    }
  }
  return *cp == '\0';
}

void LargeString::copyTo(byte* dst, word length) {
  assert(length >= 0);
  assert(length <= this->length());
  std::memcpy(dst, reinterpret_cast<byte*>(address()), length);
}

} // namespace python
