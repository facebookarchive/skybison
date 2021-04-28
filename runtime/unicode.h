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
  static bool isControlCharacter(byte b);
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
  static int8_t toDecimal(byte b);
  static int8_t toDigit(byte b);
  static byte toLower(byte b);
  static double toNumeric(byte b);
  static byte toUpper(byte b);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(ASCII);
};

// Functions corresponding to "C type" functions in CPython,
// e.g. Py_ISLOWER, Py_TOLOWER, etc.
class Byte {
 public:
  // Predicates
  static bool isAlnum(byte b);
  static bool isAlpha(byte b);
  static bool isDigit(byte b);
  static bool isHexDigit(byte b);
  static bool isLower(byte b);
  static bool isSpace(byte b);
  static bool isUpper(byte b);

  // Conversion
  static int8_t toDigit(byte b);
  static int8_t toHexDigit(byte b);
  static byte toLower(byte b);
  static byte toUpper(byte b);

 private:
  enum Flag : byte {
    kLower = 1 << 0,
    kUpper = 1 << 1,
    kAlpha = kLower | kUpper,
    kDigit = 1 << 2,
    kAlnum = kAlpha | kDigit,
    kSpace = 1 << 4,
    kHexDigit = 1 << 5,
  };

  static constexpr byte kTable[256] = {
      0,                   // 0x0 '\x00'
      0,                   // 0x1 '\x01'
      0,                   // 0x2 '\x02'
      0,                   // 0x3 '\x03'
      0,                   // 0x4 '\x04'
      0,                   // 0x5 '\x05'
      0,                   // 0x6 '\x06'
      0,                   // 0x7 '\x07'
      0,                   // 0x8 '\x08'
      kSpace,              // 0x9 '\t'
      kSpace,              // 0xa '\n'
      kSpace,              // 0xb '\v'
      kSpace,              // 0xc '\f'
      kSpace,              // 0xd '\r'
      0,                   // 0xe '\x0e'
      0,                   // 0xf '\x0f'
      0,                   // 0x10 '\x10'
      0,                   // 0x11 '\x11'
      0,                   // 0x12 '\x12'
      0,                   // 0x13 '\x13'
      0,                   // 0x14 '\x14'
      0,                   // 0x15 '\x15'
      0,                   // 0x16 '\x16'
      0,                   // 0x17 '\x17'
      0,                   // 0x18 '\x18'
      0,                   // 0x19 '\x19'
      0,                   // 0x1a '\x1a'
      0,                   // 0x1b '\x1b'
      0,                   // 0x1c '\x1c'
      0,                   // 0x1d '\x1d'
      0,                   // 0x1e '\x1e'
      0,                   // 0x1f '\x1f'
      kSpace,              // ' '
      0,                   // 0x21 '!'
      0,                   // 0x22 '"'
      0,                   // 0x23 '#'
      0,                   // 0x24 '$'
      0,                   // 0x25 '%'
      0,                   // 0x26 '&'
      0,                   // 0x27 "'"
      0,                   // 0x28 '('
      0,                   // 0x29 ')'
      0,                   // 0x2a '*'
      0,                   // 0x2b '+'
      0,                   // 0x2c ','
      0,                   // 0x2d '-'
      0,                   // 0x2e '.'
      0,                   // 0x2f '/'
      kDigit | kHexDigit,  // 0x30 '0'
      kDigit | kHexDigit,  // 0x31 '1'
      kDigit | kHexDigit,  // 0x32 '2'
      kDigit | kHexDigit,  // 0x33 '3'
      kDigit | kHexDigit,  // 0x34 '4'
      kDigit | kHexDigit,  // 0x35 '5'
      kDigit | kHexDigit,  // 0x36 '6'
      kDigit | kHexDigit,  // 0x37 '7'
      kDigit | kHexDigit,  // 0x38 '8'
      kDigit | kHexDigit,  // 0x39 '9'
      0,                   // 0x3a ':'
      0,                   // 0x3b ';'
      0,                   // 0x3c '<'
      0,                   // 0x3d '='
      0,                   // 0x3e '>'
      0,                   // 0x3f '?'
      0,                   // 0x40 '@'
      kUpper | kHexDigit,  // 0x41 'A'
      kUpper | kHexDigit,  // 0x42 'B'
      kUpper | kHexDigit,  // 0x43 'C'
      kUpper | kHexDigit,  // 0x44 'D'
      kUpper | kHexDigit,  // 0x45 'E'
      kUpper | kHexDigit,  // 0x46 'F'
      kUpper,              // 0x47 'G'
      kUpper,              // 0x48 'H'
      kUpper,              // 0x49 'I'
      kUpper,              // 0x4a 'J'
      kUpper,              // 0x4b 'K'
      kUpper,              // 0x4c 'L'
      kUpper,              // 0x4d 'M'
      kUpper,              // 0x4e 'N'
      kUpper,              // 0x4f 'O'
      kUpper,              // 0x50 'P'
      kUpper,              // 0x51 'Q'
      kUpper,              // 0x52 'R'
      kUpper,              // 0x53 'S'
      kUpper,              // 0x54 'T'
      kUpper,              // 0x55 'U'
      kUpper,              // 0x56 'V'
      kUpper,              // 0x57 'W'
      kUpper,              // 0x58 'X'
      kUpper,              // 0x59 'Y'
      kUpper,              // 0x5a 'Z'
      0,                   // 0x5b '['
      0,                   // 0x5c '\\'
      0,                   // 0x5d ']'
      0,                   // 0x5e '^'
      0,                   // 0x5f '_'
      0,                   // 0x60 '`'
      kLower | kHexDigit,  // 0x61 'a'
      kLower | kHexDigit,  // 0x62 'b'
      kLower | kHexDigit,  // 0x63 'c'
      kLower | kHexDigit,  // 0x64 'd'
      kLower | kHexDigit,  // 0x65 'e'
      kLower | kHexDigit,  // 0x66 'f'
      kLower,              // 0x67 'g'
      kLower,              // 0x68 'h'
      kLower,              // 0x69 'i'
      kLower,              // 0x6a 'j'
      kLower,              // 0x6b 'k'
      kLower,              // 0x6c 'l'
      kLower,              // 0x6d 'm'
      kLower,              // 0x6e 'n'
      kLower,              // 0x6f 'o'
      kLower,              // 0x70 'p'
      kLower,              // 0x71 'q'
      kLower,              // 0x72 'r'
      kLower,              // 0x73 's'
      kLower,              // 0x74 't'
      kLower,              // 0x75 'u'
      kLower,              // 0x76 'v'
      kLower,              // 0x77 'w'
      kLower,              // 0x78 'x'
      kLower,              // 0x79 'y'
      kLower,              // 0x7a 'z'
  };

  static constexpr byte kToLower[256] = {
      0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
      0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
      0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23,
      0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
      0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b,
      0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
      0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73,
      0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
      0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b,
      0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
      0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f, 0x80, 0x81, 0x82, 0x83,
      0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
      0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b,
      0x9c, 0x9d, 0x9e, 0x9f, 0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
      0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf, 0xb0, 0xb1, 0xb2, 0xb3,
      0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
      0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb,
      0xcc, 0xcd, 0xce, 0xcf, 0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7,
      0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf, 0xe0, 0xe1, 0xe2, 0xe3,
      0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
      0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb,
      0xfc, 0xfd, 0xfe, 0xff,
  };

  static constexpr byte kToUpper[256] = {
      0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
      0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
      0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23,
      0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
      0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b,
      0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
      0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x51, 0x52, 0x53,
      0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
      0x60, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b,
      0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
      0x58, 0x59, 0x5a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f, 0x80, 0x81, 0x82, 0x83,
      0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
      0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b,
      0x9c, 0x9d, 0x9e, 0x9f, 0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
      0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf, 0xb0, 0xb1, 0xb2, 0xb3,
      0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
      0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb,
      0xcc, 0xcd, 0xce, 0xcf, 0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7,
      0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf, 0xe0, 0xe1, 0xe2, 0xe3,
      0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
      0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb,
      0xfc, 0xfd, 0xfe, 0xff,
  };
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
  static const byte kSurrogateLeadByte = 0xED;
  static constexpr byte kBOM[] = {0xef, 0xbb, 0xbf};

  // Predicates
  static bool isLeadByte(byte b);
  static bool isTrailByte(byte b);

  // Given the lead byte of a UTF-8 code point, return its length.
  static word numChars(byte lead_byte);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(UTF8);
};

class UTF16 {
 public:
  static constexpr byte kBOMLittleEndian[] = {0xff, 0xfe};
  static constexpr byte kBOMBigEndian[] = {0xfe, 0xff};
};

class UTF32 {
 public:
  static constexpr byte kBOMLittleEndian[] = {0xff, 0xfe, 0, 0};
  static constexpr byte kBOMBigEndian[] = {0, 0, 0xfe, 0xff};
};

// Functions for Unicode code points.
class Unicode {
 public:
  // Constants
  static const int32_t kAliasStart = 0xf0000;
  static const int32_t kHighSurrogateStart = 0xd800;
  static const int32_t kHighSurrogateEnd = 0xdbff;
  static const int32_t kHangulSyllableStart = 0xac00;
  static const int32_t kHangulLeadStart = 0x1100;
  static const int32_t kHangulVowelStart = 0x1161;
  static const int32_t kHangulTrailStart = 0x11a7;
  static const int32_t kLowSurrogateStart = 0xdc00;
  static const int32_t kLowSurrogateEnd = 0xdfff;
  static const int32_t kNamedSequenceStart = 0xf0200;
  static const int32_t kSurrogateMask = 0x03ff;

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
  static bool isAlnum(int32_t code_point);
  static bool isCaseIgnorable(int32_t code_point);
  static bool isCased(int32_t code_point);
  static bool isDecimal(int32_t code_point);
  static bool isDigit(int32_t code_point);
  static bool isHangulLead(int32_t code_point);
  static bool isHangulSyllable(int32_t code_point);
  static bool isHangulTrail(int32_t code_point);
  static bool isHangulVowel(int32_t code_point);
  static bool isHighSurrogate(int32_t code_point);
  static bool isLinebreak(int32_t code_point);
  static bool isLowSurrogate(int32_t code_point);
  static bool isLower(int32_t code_point);
  static bool isNamedSequence(int32_t code_point);
  static bool isNumeric(int32_t code_point);
  static bool isPrintable(int32_t code_point);
  static bool isSpace(int32_t code_point);
  static bool isSurrogate(int32_t code_point);
  static bool isTitle(int32_t code_point);
  static bool isUnfolded(int32_t code_point);
  static bool isUpper(int32_t code_point);
  static bool isXidContinue(int32_t code_point);
  static bool isXidStart(int32_t code_point);

  // Conversion
  static int32_t combineSurrogates(int32_t high_code_point,
                                   int32_t low_code_point);
  static int8_t toDecimal(int32_t code_point);
  static int8_t toDigit(int32_t code_point);
  static FullCasing toFolded(int32_t code_point);
  static FullCasing toLower(int32_t code_point);
  static double toNumeric(int32_t code_point);
  static FullCasing toTitle(int32_t code_point);
  static FullCasing toUpper(int32_t code_point);

 private:
  // Slow paths that use the Unicode database.
  static bool isAlphaDB(int32_t code_point);
  static bool isCaseIgnorableDB(int32_t code_point);
  static bool isCasedDB(int32_t code_point);
  static bool isDecimalDB(int32_t code_point);
  static bool isDigitDB(int32_t code_point);
  static bool isLinebreakDB(int32_t code_point);
  static bool isLowerDB(int32_t code_point);
  static bool isNumericDB(int32_t code_point);
  static bool isPrintableDB(int32_t code_point);
  static bool isSpaceDB(int32_t code_point);
  static bool isTitleDB(int32_t code_point);
  static bool isUnfoldedDB(int32_t code_point);
  static bool isUpperDB(int32_t code_point);
  static bool isXidContinueDB(int32_t code_point);
  static bool isXidStartDB(int32_t code_point);
  static int8_t toDecimalDB(int32_t code_point);
  static int8_t toDigitDB(int32_t code_point);
  static FullCasing toFoldedDB(int32_t code_point);
  static FullCasing toLowerDB(int32_t code_point);
  static double toNumericDB(int32_t code_point);
  static FullCasing toTitleDB(int32_t code_point);
  static FullCasing toUpperDB(int32_t code_point);

  DISALLOW_IMPLICIT_CONSTRUCTORS(Unicode);
};

// ASCII

inline bool ASCII::isAlnum(byte b) { return isDigit(b) || isAlpha(b); }

inline bool ASCII::isAlpha(byte b) { return isUpper(b) || isLower(b); }

inline bool ASCII::isControlCharacter(byte b) { return b <= 0x1f; }

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

inline int8_t ASCII::toDecimal(byte b) { return toDigit(b); }

inline int8_t ASCII::toDigit(byte b) { return isDigit(b) ? b - '0' : -1; }

inline byte ASCII::toLower(byte b) { return isUpper(b) ? b + ('a' - 'A') : b; }

inline double ASCII::toNumeric(byte b) {
  return isNumeric(b) ? static_cast<double>(b - '0') : -1.0;
}

inline byte ASCII::toUpper(byte b) { return isLower(b) ? b - ('a' - 'A') : b; }

// Byte

inline bool Byte::isAlnum(byte b) { return (kTable[b] & kAlnum) != 0; }

inline bool Byte::isAlpha(byte b) { return (kTable[b] & kAlpha) != 0; }

inline bool Byte::isDigit(byte b) { return (kTable[b] & kDigit) != 0; }

inline bool Byte::isLower(byte b) { return (kTable[b] & kLower) != 0; }

inline bool Byte::isSpace(byte b) { return (kTable[b] & kSpace) != 0; }

inline bool Byte::isUpper(byte b) { return (kTable[b] & kUpper) != 0; }

inline bool Byte::isHexDigit(byte b) { return (kTable[b] & kHexDigit) != 0; }

inline int8_t Byte::toDigit(byte b) { return Byte::isDigit(b) ? b - '0' : -1; }

inline int8_t Byte::toHexDigit(byte b) {
  if (Byte::isDigit(b)) {
    return b - '0';
  }
  if ('a' <= b && b <= 'f') {
    return b - 'a' + 10;
  }
  if ('A' <= b && b <= 'F') {
    return b - 'A' + 10;
  }
  return -1;
}

inline byte Byte::toLower(byte b) { return kToLower[b]; }

inline byte Byte::toUpper(byte b) { return kToUpper[b]; }

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

inline bool Unicode::isAlnum(int32_t code_point) {
  if (isASCII(code_point)) {
    return ASCII::isAlnum(code_point);
  }
  return Unicode::isAlphaDB(code_point) || Unicode::isDecimalDB(code_point) ||
         Unicode::isDigitDB(code_point) || Unicode::isNumericDB(code_point);
}

inline bool Unicode::isAlpha(int32_t code_point) {
  if (isASCII(code_point)) {
    return ASCII::isAlpha(code_point);
  }
  return Unicode::isAlphaDB(code_point);
}

inline bool Unicode::isCaseIgnorable(int32_t code_point) {
  if (isASCII(code_point)) {
    return !ASCII::isAlpha(code_point);
  }
  return isCaseIgnorableDB(code_point);
}

inline bool Unicode::isCased(int32_t code_point) {
  if (isASCII(code_point)) {
    return ASCII::isAlpha(code_point);
  }
  return isCasedDB(code_point);
}

inline bool Unicode::isDecimal(int32_t code_point) {
  if (isASCII(code_point)) {
    return ASCII::isDecimal(code_point);
  }
  return isDecimalDB(code_point);
}

inline bool Unicode::isDigit(int32_t code_point) {
  if (isASCII(code_point)) {
    return ASCII::isDigit(code_point);
  }
  return isDigitDB(code_point);
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

inline bool Unicode::isHighSurrogate(int32_t code_point) {
  return (kHighSurrogateStart <= code_point) &&
         (code_point <= kHighSurrogateEnd);
}

inline bool Unicode::isLinebreak(int32_t code_point) {
  if (isASCII(code_point)) {
    return ASCII::isLinebreak(code_point);
  }
  return isLinebreakDB(code_point);
}

inline bool Unicode::isLowSurrogate(int32_t code_point) {
  return (kLowSurrogateStart <= code_point) && (code_point <= kLowSurrogateEnd);
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

inline bool Unicode::isNumeric(int32_t code_point) {
  if (isASCII(code_point)) {
    return ASCII::isNumeric(code_point);
  }
  return Unicode::isNumericDB(code_point);
}

inline bool Unicode::isPrintable(int32_t code_point) {
  if (isASCII(code_point)) {
    return ASCII::isPrintable(code_point);
  }
  return Unicode::isPrintableDB(code_point);
}

inline bool Unicode::isSpace(int32_t code_point) {
  if (isASCII(code_point)) {
    return ASCII::isSpace(code_point);
  }
  return isSpaceDB(code_point);
}

inline bool Unicode::isSurrogate(int32_t code_point) {
  return kHighSurrogateStart <= code_point && code_point <= kLowSurrogateEnd;
}

inline bool Unicode::isTitle(int32_t code_point) {
  if (isASCII(code_point)) {
    return false;
  }
  return isTitleDB(code_point);
}

inline bool Unicode::isUnfolded(int32_t code_point) {
  if (isASCII(code_point)) {
    return false;
  }
  return isUnfoldedDB(code_point);
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

inline int32_t Unicode::combineSurrogates(int32_t high_code_point,
                                          int32_t low_code_point) {
  DCHECK(Unicode::isHighSurrogate(high_code_point), "expected high surrogate");
  DCHECK(Unicode::isLowSurrogate(low_code_point), "expected low surrogate");
  int32_t result = (((high_code_point & kSurrogateMask)) << 10 |
                    (low_code_point & kSurrogateMask)) +
                   0x10000;
  DCHECK(result <= kMaxUnicode, "result must be valid code point");
  return result;
}

inline int8_t Unicode::toDecimal(int32_t code_point) {
  if (isASCII(code_point)) {
    return ASCII::toDecimal(code_point);
  }
  return toDecimalDB(code_point);
}

inline int8_t Unicode::toDigit(int32_t code_point) {
  if (isASCII(code_point)) {
    return ASCII::toDigit(code_point);
  }
  return toDigitDB(code_point);
}

inline FullCasing Unicode::toFolded(int32_t code_point) {
  if (isASCII(code_point)) {
    return {ASCII::toLower(code_point), -1};
  }
  return toFoldedDB(code_point);
}

inline FullCasing Unicode::toLower(int32_t code_point) {
  if (isASCII(code_point)) {
    return {ASCII::toLower(code_point), -1};
  }
  return toLowerDB(code_point);
}

inline double Unicode::toNumeric(int32_t code_point) {
  if (isASCII(code_point)) {
    return ASCII::toNumeric(code_point);
  }
  return toNumericDB(code_point);
}

inline FullCasing Unicode::toTitle(int32_t code_point) {
  if (isASCII(code_point)) {
    return {ASCII::toUpper(code_point), -1};
  }
  return toTitleDB(code_point);
}

inline FullCasing Unicode::toUpper(int32_t code_point) {
  if (isASCII(code_point)) {
    return {ASCII::toUpper(code_point), -1};
  }
  return toUpperDB(code_point);
}

}  // namespace py
