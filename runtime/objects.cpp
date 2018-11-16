#include "objects.h"
#include "runtime.h"

#include <cassert>
#include <cstring>

namespace python {

// SmallString

Object* SmallString::fromCString(const char* value) {
  word len = strlen(value);
  return fromBytes(view(reinterpret_cast<const byte*>(value), len));
}

Object* SmallString::fromBytes(View<byte> data) {
  word length = data.length();
  assert(0 <= length);
  assert(length <= kMaxLength);
  word result = 0;
  for (word i = length; i > 0;) {
    i -= 1;
    result = (result << kBitsPerByte) | data.get(i);
  }
  result = (result << kBitsPerByte) | (length << kTagSize) | kTag;
  return reinterpret_cast<SmallString*>(result);
}

char* SmallString::toCString() {
  word length = this->length();
  byte* result = static_cast<byte*>(std::malloc(length + 1));
  assert(result != nullptr);
  copyTo(result, length);
  result[length] = '\0';
  return reinterpret_cast<char*>(result);
}

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

void LargeString::copyTo(byte* dst, word length) {
  assert(length >= 0);
  assert(length <= this->length());
  std::memcpy(dst, reinterpret_cast<const byte*>(address()), length);
}

char* LargeString::toCString() {
  word length = this->length();
  byte* result = static_cast<byte*>(std::malloc(length + 1));
  assert(result != nullptr);
  copyTo(result, length);
  result[length] = '\0';
  return reinterpret_cast<char*>(result);
}

} // namespace python
