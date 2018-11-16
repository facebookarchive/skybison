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

word Slice::adjustIndices(word length, word* start, word* stop, word step) {
  CHECK(step != 0, "Step should be non zero");

  if (*start < 0) {
    *start += length;
    if (*start < 0) {
      *start = (step < 0) ? -1 : 0;
    }
  } else if (*start >= length) {
    *start = (step < 0) ? length - 1 : length;
  }

  if (*stop < 0) {
    *stop += length;
    if (*stop < 0) {
      *stop = (step < 0) ? -1 : 0;
    }
  } else if (*stop >= length) {
    *stop = (step < 0) ? length - 1 : length;
  }

  if (step < 0) {
    if (*stop < *start) {
      return (*start - *stop - 1) / (-step) + 1;
    }
  } else {
    if (*start < *stop) {
      return (*stop - *start - 1) / step + 1;
    }
  }
  return 0;
}

} // namespace python
