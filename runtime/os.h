#pragma once

#include "globals.h"

namespace python {

class OS {
 public:
  enum { kPageSize = 4 * KiB };

  enum Protection { kNoAccess, kReadWrite };

  static byte* allocateMemory(word size);

  static bool freeMemory(byte* ptr, word size);

  static bool protectMemory(byte* ptr, word size, Protection);

  static bool secureRandom(byte* ptr, word size);

  static char* readFile(const char* filename);

  static char* temporaryDirectory(const char* prefix);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(OS);
};

} // namespace python
