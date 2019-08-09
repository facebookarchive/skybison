#pragma once

#include "globals.h"

namespace python {

// This should be used only for bytes objects. For strings, use isUnicodeSpace.
inline bool isAsciiSpace(byte ch) {
  switch (ch) {
    case 0x09:
    case 0x0A:
    case 0x0B:
    case 0x0C:
    case 0x0D:
    case 0x1C:
    case 0x1D:
    case 0x1E:
    case 0x1F:
    case 0x20:
      return true;
    default:
      return false;
  }
}

// TODO(T43723300): implement isUnicodeSpace

}  // namespace python
