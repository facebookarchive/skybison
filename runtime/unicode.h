#pragma once

#include "globals.h"

namespace py {

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

// Returns 1 for Unicode characters having the bidirectional
// type 'WS', 'B' or 'S' or the category 'Zs', 0 otherwise.
inline bool isSpaceUnicode(int32_t cp) {
  switch (cp) {
    case 0x0009:
    case 0x000A:
    case 0x000B:
    case 0x000C:
    case 0x000D:
    case 0x001C:
    case 0x001D:
    case 0x001E:
    case 0x001F:
    case 0x0020:
    case 0x0085:
    case 0x00A0:
    case 0x1680:
    case 0x2000:
    case 0x2001:
    case 0x2002:
    case 0x2003:
    case 0x2004:
    case 0x2005:
    case 0x2006:
    case 0x2007:
    case 0x2008:
    case 0x2009:
    case 0x200A:
    case 0x2028:
    case 0x2029:
    case 0x202F:
    case 0x205F:
    case 0x3000:
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

inline bool isPrintableASCII(byte b) { return ' ' <= b && b < kMaxASCII; }

inline bool isPrintableUnicode(int32_t cp) {
  // TODO(T55176519): implement using Unicode database
  if (cp <= kMaxASCII) {
    return isPrintableASCII(cp);
  }
  return true;
}

// TODO(T43723300): implement isUnicodeSpace

inline bool isUTF8Continuation(byte b) {
  return (b & 0xC0) == 0x80;  // Test for 0b10xxxxxx
}

}  // namespace py
