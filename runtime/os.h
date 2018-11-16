#pragma once

#include "globals.h"

namespace python {

class Os {
 public:
  enum { kPageSize = 4 * KiB };

  enum Protection { kNoAccess, kReadWrite };

  static byte* allocateMemory(intptr_t size);

  static bool freeMemory(byte* ptr, intptr_t size);

  static bool protectMemory(byte* ptr, intptr_t size, Protection);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(Os);
};

} // namespace python
