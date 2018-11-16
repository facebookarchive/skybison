#include "objects.h"
#include "runtime.h"

#include <cassert>
#include <cstring>

namespace python {

// String

bool String::equals(Object* that) {
  if (this == that) {
    return true;
  }
  if (!that->isString()) {
    return false;
  }
  String* thatStr = String::cast(that);
  if (length() != thatStr->length()) {
    return false;
  }
  auto s1 = reinterpret_cast<void*>(address());
  auto s2 = reinterpret_cast<void*>(thatStr->address());
  return std::memcmp(s1, s2, length()) == 0;
}

bool String::equalsCString(const char* c_string) {
  const char* cp = c_string;
  for (word i = 0; i < length(); i++, cp++) {
    char ch = *cp;
    if (ch == '\0' || ch != charAt(i)) {
      return false;
    }
  }
  return *cp == '\0';
}

} // namespace python
