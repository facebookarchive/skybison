#pragma once

#include <cstdint>

#include "globals.h"
#include "utils.h"

namespace py {

// Functions for ASCII code points. These should only be used for bytes-like
// objects or when a code point is guaranteed to be valid ASCII.
class ASCII {
 public:
  // Predicates
  static bool isAlnum(byte b);
  static bool isAlpha(byte b);
  static bool isDecimal(byte b);
  static bool isDigit(byte b);
  static bool isLinebreak(byte b);
  static bool isLower(byte b);
  static bool isNumeric(byte b);
  static bool isPrintable(byte b);
  static bool isUpper(byte b);
  static bool isSpace(byte b);
  static bool isXidContinue(byte b);
  static bool isXidStart(byte b);

  // Conversion
  static byte toLower(byte b);
  static byte toUpper(byte b);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(ASCII);
};

// Represents the possible result of casing a codepoint. Since lower-, upper-,
// and title-casing a codepoint can be a one-to-many mapping, this cannot be
// represented as a single value.
struct FullCasing {
  int32_t code_points[3];
};

class UTF8 {
 public:
  static const word kMaxLength = 4;

  // Predicates
  static bool isLeadByte(byte b);
  static bool isTrailByte(byte b);

  // Given the lead byte of a UTF-8 code point, return its length.
  static word numChars(byte lead_byte);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(UTF8);
};

// Functions for Unicode code points.
class Unicode {
 public:
  // Constants
  static const int32_t kAliasStart = 0xf0000;
  static const int32_t kHangulSyllableStart = 0xac00;
  static const int32_t kHangulLeadStart = 0x1100;
  static const int32_t kHangulVowelStart = 0x1161;
  static const int32_t kHangulTrailStart = 0x11a7;
  static const int32_t kNamedSequenceStart = 0xf0200;

  static const int kAliasCount = 468;
  static const int kHangulLeadCount = 19;
  static const int kHangulVowelCount = 21;
  static const int kHangulTrailCount = 28;
  static const int kHangulCodaCount = kHangulVowelCount * kHangulTrailCount;
  static const int kHangulSyllableCount = kHangulLeadCount * kHangulCodaCount;
  static const int kNamedSequenceCount = 442;

  // Predicates
  static bool isASCII(int32_t code_point);
  static bool isAlias(int32_t code_point);
  static bool isAlpha(int32_t code_point);
  static bool isHangulLead(int32_t code_point);
  static bool isHangulSyllable(int32_t code_point);
  static bool isHangulTrail(int32_t code_point);
  static bool isHangulVowel(int32_t code_point);
  static bool isLinebreak(int32_t code_point);
  static bool isLower(int32_t code_point);
  static bool isNamedSequence(int32_t code_point);
  static bool isPrintable(int32_t code_point);
  static bool isSpace(int32_t code_point);
  static bool isTitle(int32_t code_point);
  static bool isUpper(int32_t code_point);
  static bool isXidContinue(int32_t code_point);
  static bool isXidStart(int32_t code_point);

  // Conversion
  static FullCasing toLower(int32_t code_point);
  static int32_t toTitle(int32_t code_point);
  static FullCasing toUpper(int32_t code_point);

 private:
  // Slow paths that use the Unicode database.
  static bool isLinebreakDB(int32_t code_point);
  static bool isLowerDB(int32_t code_point);
  static bool isSpaceDB(int32_t code_point);
  static bool isTitleDB(int32_t code_point);
  static bool isUpperDB(int32_t code_point);
  static bool isXidContinueDB(int32_t code_point);
  static bool isXidStartDB(int32_t code_point);
  static FullCasing toLowerDB(int32_t code_point);
  static FullCasing toUpperDB(int32_t code_point);

  DISALLOW_IMPLICIT_CONSTRUCTORS(Unicode);
};

// ASCII

inline bool ASCII::isAlnum(byte b) { return isDigit(b) || isAlpha(b); }

inline bool ASCII::isAlpha(byte b) { return isUpper(b) || isLower(b); }

inline bool ASCII::isDecimal(byte b) { return isDigit(b); }

inline bool ASCII::isDigit(byte b) { return '0' <= b && b <= '9'; }

inline bool ASCII::isLinebreak(byte b) {
  switch (b) {
    case '\n':
    case '\x0B':
    case '\x0C':
    case '\r':
    case '\x1C':
    case '\x1D':
    case '\x1E':
      return true;
    default:
      return false;
  }
}

inline bool ASCII::isLower(byte b) { return 'a' <= b && b <= 'z'; }

inline bool ASCII::isNumeric(byte b) { return isDigit(b); }

inline bool ASCII::isPrintable(byte b) { return ' ' <= b && b < kMaxASCII; }

inline bool ASCII::isSpace(byte b) {
  switch (b) {
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

inline bool ASCII::isUpper(byte b) { return 'A' <= b && b <= 'Z'; }

inline bool ASCII::isXidContinue(byte b) { return isXidStart(b) || isDigit(b); }

inline bool ASCII::isXidStart(byte b) { return isAlpha(b) || b == '_'; }

inline byte ASCII::toLower(byte b) { return isUpper(b) ? b + ('a' - 'A') : b; }

inline byte ASCII::toUpper(byte b) { return isLower(b) ? b - ('a' - 'A') : b; }

// UTF-8

inline bool UTF8::isLeadByte(byte b) {
  DCHECK(b < 0xF8, "invalid UTF-8 byte");
  return (b & 0xC0) != 0x80;
}

inline bool UTF8::isTrailByte(byte b) { return (b & 0xC0) == 0x80; }

inline word UTF8::numChars(byte lead_byte) {
  if (lead_byte <= kMaxASCII) {
    return 1;
  }
  if (lead_byte < 0xE0) {
    DCHECK(lead_byte >= 0xC0, "invalid lead byte");
    return 2;
  }
  if (lead_byte < 0xF0) {
    return 3;
  }
  DCHECK(lead_byte < 0xF8, "invalid lead byte");
  return 4;
}

// Unicode

inline bool Unicode::isASCII(int32_t code_point) {
  return code_point <= kMaxASCII;
}

inline bool Unicode::isAlias(int32_t code_point) {
  return (kAliasStart <= code_point) &&
         (code_point < kAliasStart + kAliasCount);
}

inline bool Unicode::isAlpha(int32_t code_point) {
  if (isASCII(code_point)) {
    return ASCII::isAlpha(code_point);
  }
  // TODO(T57791326) support non-ASCII
  UNIMPLEMENTED("non-ASCII characters");
}

inline bool Unicode::isHangulLead(int32_t code_point) {
  return (kHangulLeadStart <= code_point) &&
         (code_point < kHangulLeadStart + kHangulLeadCount);
}

inline bool Unicode::isHangulSyllable(int32_t code_point) {
  return (kHangulSyllableStart <= code_point) &&
         (code_point < kHangulSyllableStart + kHangulSyllableCount);
}

inline bool Unicode::isHangulTrail(int32_t code_point) {
  return (kHangulTrailStart <= code_point) &&
         (code_point < kHangulTrailStart + kHangulTrailCount);
}

inline bool Unicode::isHangulVowel(int32_t code_point) {
  return (kHangulVowelStart <= code_point) &&
         (code_point < kHangulVowelStart + kHangulVowelCount);
}

inline bool Unicode::isLinebreak(int32_t code_point) {
  if (isASCII(code_point)) {
    return ASCII::isLinebreak(code_point);
  }
  return isLinebreakDB(code_point);
}

inline bool Unicode::isLower(int32_t code_point) {
  if (isASCII(code_point)) {
    return ASCII::isLower(code_point);
  }
  return isLowerDB(code_point);
}

inline bool Unicode::isNamedSequence(int32_t code_point) {
  return (kNamedSequenceStart <= code_point) &&
         (code_point < kNamedSequenceStart + kNamedSequenceCount);
}

inline bool Unicode::isPrintable(int32_t code_point) {
  // TODO(T55176519): implement using Unicode database
  if (isASCII(code_point)) {
    return ASCII::isPrintable(code_point);
  }
  return true;
}

inline bool Unicode::isSpace(int32_t code_point) {
  if (isASCII(code_point)) {
    return ASCII::isSpace(code_point);
  }
  return isSpaceDB(code_point);
}

inline bool Unicode::isTitle(int32_t code_point) {
  if (isASCII(code_point)) {
    return false;
  }
  return isTitleDB(code_point);
}

inline bool Unicode::isUpper(int32_t code_point) {
  if (isASCII(code_point)) {
    return ASCII::isUpper(code_point);
  }
  return isUpperDB(code_point);
}

inline bool Unicode::isXidContinue(int32_t code_point) {
  if (isASCII(code_point)) {
    return ASCII::isXidContinue(code_point);
  }
  return isXidContinueDB(code_point);
}

inline bool Unicode::isXidStart(int32_t code_point) {
  if (isASCII(code_point)) {
    return ASCII::isXidStart(code_point);
  }
  return isXidStartDB(code_point);
}

inline FullCasing Unicode::toLower(int32_t code_point) {
  if (isASCII(code_point)) {
    return {ASCII::toLower(code_point), -1};
  }
  return toLowerDB(code_point);
}

inline int32_t Unicode::toTitle(int32_t code_point) {
  if (isASCII(code_point)) {
    if (ASCII::isLower(code_point)) {
      code_point += ('A' - 'a');
    }
    return code_point;
  }
  // TODO(T57791326) support non-ASCII
  UNIMPLEMENTED("non-ASCII characters");
}

inline FullCasing Unicode::toUpper(int32_t code_point) {
  if (isASCII(code_point)) {
    return {ASCII::toUpper(code_point), -1};
  }
  return toUpperDB(code_point);
}

}  // namespace py
