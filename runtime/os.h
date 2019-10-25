#pragma once

#include "globals.h"
#include "mutex.h"

namespace py {

class OS {
 public:
  enum { kPageSize = 4 * kKiB };

  enum Protection { kNoAccess, kReadWrite, kReadExecute };

  // Allocate a page-sized chunk of memory. If allocated_size is not nullptr,
  // the rounded-up size will be written to it.
  static byte* allocateMemory(word size, word* allocated_size = nullptr);

  // Returns an absolute path to the current executable. The path may contain
  // unresolved symlinks.
  static char* executablePath();

  static bool freeMemory(byte* ptr, word size);

  static bool protectMemory(byte* address, word size, Protection);

  static bool secureRandom(byte* ptr, word size);

  static char* readFile(const char* filename, word* len_out = nullptr);

  static void writeFileExcl(const char* filename, const char* contents,
                            word len = -1);

  static char* temporaryDirectory(const char* prefix);

  static char* temporaryFile(const char* prefix, int* fd);

  static const char* name();

  static bool dirExists(const char* dir);

  static bool fileExists(const char* file);

  static double currentTime();

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(OS);
};

}  // namespace py
