#pragma once

#include "globals.h"

namespace python {

class Os {
 public:
  enum { kPageSize = 4 * KiB };

  enum Protection { kNoAccess, kReadWrite };

  static byte* allocateMemory(word size);

  static bool freeMemory(byte* ptr, word size);

  static bool protectMemory(byte* ptr, word size, Protection);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(Os);
};

} // namespace python
