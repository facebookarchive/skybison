#include "bytecode.h"
#include "globals.h"

#include <stdio.h>
#include <cassert>
using namespace python;

static const char* const byteCodeName[] = {
#define NAME(name, value) #name,
    FOREACH_BYTECODE(NAME)
#undef NAME
};

const char* bytecode::name(Bytecode bc) {
  if (bc < ARRAYSIZE(byteCodeName)) {
    return byteCodeName[bc];
  }
  static char buffer[64];
  uword size =
      1 + snprintf(nullptr, 0, "invalid byte code %ld", static_cast<word>(bc));
  assert(size < ARRAYSIZE(buffer));
  snprintf(buffer, size, "invalid byte code %ld", static_cast<word>(bc));
  return buffer;
}
