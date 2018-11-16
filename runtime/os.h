#pragma once

#include "globals.h"

namespace python {

class OS {
 public:
  enum { kPageSize = 4 * kKiB };

  enum Protection { kNoAccess, kReadWrite };

  static byte* allocateMemory(word size);

  static bool freeMemory(byte* ptr, word size);

  static bool protectMemory(byte* ptr, word size, Protection);

  static bool secureRandom(byte* ptr, word size);

  static char* readFile(const char* filename, word* len_out = nullptr);

  static void writeFileExcl(const char* filename, const char* contents,
                            word len = -1);

  static char* temporaryDirectory(const char* prefix);

  static const char* getenv(const char* var);

  static const char* name();

  static bool dirExists(const char* dir);

  static bool fileExists(const char* file);

  static double currentTime();

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(OS);
};

}  // namespace python
