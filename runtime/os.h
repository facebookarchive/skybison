#pragma once

#include <cstdio>

#include "globals.h"

namespace py {

typedef void (*SignalHandler)(int);

class OS {
 public:
  enum { kPageSize = 4 * kKiB };

  enum Protection { kNoAccess, kReadWrite, kReadExecute };

  static const word kNumSignals;

  static const int kRtldGlobal;
  static const int kRtldLocal;
  static const int kRtldNow;

  // Allocate a page-sized chunk of memory. If allocated_size is not nullptr,
  // the rounded-up size will be written to it.
  static byte* allocateMemory(word size, word* allocated_size = nullptr);

  // Returns whether the user has access to the specified path with the given
  // mode (which represents a bit mask of flags for the file existing, being
  // readable, writable, or executable).
  static bool access(const char* path, int mode);

  // Returns an absolute path to the current executable. The path may contain
  // unresolved symlinks.
  static char* executablePath();

  static bool freeMemory(byte* ptr, word size);

  // Returns the system page size
  static int pageSize();

  static bool protectMemory(byte* address, word size, Protection);

  static bool secureRandom(byte* ptr, word size);

  static SignalHandler setSignalHandler(int signum, SignalHandler handler);
  static SignalHandler signalHandler(int signum);

  static char* readFile(FILE* fp, word* len_out);

  static char* temporaryDirectory(const char* prefix);

  static char* temporaryFile(const char* prefix, int* fd);

  static const char* name();

  static bool dirExists(const char* dir);

  static bool fileExists(const char* file);

  // Read value of symbolic link and return a null-terminated string. Returns
  // nullptr if path is not a link or cannot be read. Caller is responsible for
  // freeing the return value with free().
  static char* readLink(const char* path);

  static double currentTime();

  static void* openSharedObject(const char* filename, int mode,
                                const char** error_msg);

  static void* sharedObjectSymbolAddress(void* handle, const char* symbol,
                                         const char** error_msg);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(OS);
};

}  // namespace py
