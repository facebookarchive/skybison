#pragma once

#include "globals.h"

namespace python {

class Os {
 public:
  enum { PAGE_SIZE = 4096 };
  static byte* allocateMemory(int size);
  static bool freeMemory(byte* ptr, int size);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(Os);
};

} // namespace python
