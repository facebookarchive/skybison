#pragma once

#include "globals.h"

namespace python {

// This should be used only for bytes objects. For strings, use isUnicodeSpace.
inline bool isSpaceASCII(byte ch) {
  switch (ch) {
    case '\t':
    case '\n':
    case '\x0B':
    case '\x0C':
    case '\r':
    case '\x1C':
    case '\x1D':
    case '\x1E':
    case '\x1F':
    case ' ':
      return true;
    default:
      return false;
  }
}

inline bool isDigitASCII(byte b) { return '0' <= b && b <= '9'; }

inline bool isDecimalASCII(byte b) { return isDigitASCII(b); }

inline bool isNumericASCII(byte b) { return isDigitASCII(b); }

inline bool isLowerASCII(byte b) { return 'a' <= b && b <= 'z'; }

inline bool isUpperASCII(byte b) { return 'A' <= b && b <= 'Z'; }

inline bool isAlphaASCII(byte b) { return isUpperASCII(b) || isLowerASCII(b); }

inline bool isAlnumASCII(byte b) { return isDigitASCII(b) || isAlphaASCII(b); }

inline bool isIdStartASCII(byte b) { return isAlphaASCII(b) || b == '_'; }

inline bool isIdContinueASCII(byte b) {
  return isIdStartASCII(b) || isDigitASCII(b);
}

inline bool isPrintableASCII(byte b) { return ' ' <= b && b < '\x7F'; }

// TODO(T43723300): implement isUnicodeSpace

}  // namespace python
