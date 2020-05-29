#include "unicode.h"

#include <cstdint>

#include "unicode-db.h"

namespace py {

bool Unicode::isCasedDB(int32_t code_point) {
  return (typeRecord(code_point)->flags & kCasedMask) != 0;
}

bool Unicode::isLinebreakDB(int32_t code_point) {
  return unicodeIsLinebreak(code_point);
}

bool Unicode::isLowerDB(int32_t code_point) {
  return (typeRecord(code_point)->flags & kLowerMask) != 0;
}

bool Unicode::isSpaceDB(int32_t code_point) {
  return unicodeIsWhitespace(code_point);
}

bool Unicode::isTitleDB(int32_t code_point) {
  return (typeRecord(code_point)->flags & kTitleMask) != 0;
}

bool Unicode::isUnfoldedDB(int32_t code_point) {
  const UnicodeTypeRecord* record = typeRecord(code_point);
  return (record->flags & kExtendedCaseMask) != 0 &&
         ((record->lower >> 20) & 7) != 0;
}

bool Unicode::isUpperDB(int32_t code_point) {
  return (typeRecord(code_point)->flags & kUpperMask) != 0;
}

bool Unicode::isXidContinueDB(int32_t code_point) {
  return (typeRecord(code_point)->flags & kXidContinueMask) != 0;
}

bool Unicode::isXidStartDB(int32_t code_point) {
  return (typeRecord(code_point)->flags & kXidStartMask) != 0;
}

FullCasing Unicode::toFoldedDB(int32_t code_point) {
  const UnicodeTypeRecord* record = typeRecord(code_point);

  if (record->flags & kExtendedCaseMask && (record->lower >> 20) & 7) {
    FullCasing result = {-1, -1, -1};
    int32_t index = (record->lower & 0xFFFF) + (record->lower >> 24);
    switch ((record->lower >> 20) & 7) {
      default:
        UNREACHABLE("Case mappings are limited to [1..3] code points");
      case 3:
        result.code_points[2] = extendedCaseMapping(index + 2);
        FALLTHROUGH;
      case 2:
        result.code_points[1] = extendedCaseMapping(index + 1);
        FALLTHROUGH;
      case 1:
        result.code_points[0] = extendedCaseMapping(index);
    }
    return result;
  }
  return toLowerDB(code_point);
}

FullCasing Unicode::toLowerDB(int32_t code_point) {
  const UnicodeTypeRecord* record = typeRecord(code_point);
  if ((record->flags & kExtendedCaseMask) == 0) {
    return {code_point + record->lower, -1};
  }
  FullCasing result = {-1, -1, -1};
  int32_t index = record->lower & 0xFFFF;
  switch (record->lower >> 24) {
    default:
      UNREACHABLE("Case mappings are limited to [1..3] code points");
    case 3:
      result.code_points[2] = extendedCaseMapping(index + 2);
      FALLTHROUGH;
    case 2:
      result.code_points[1] = extendedCaseMapping(index + 1);
      FALLTHROUGH;
    case 1:
      result.code_points[0] = extendedCaseMapping(index);
  }
  return result;
}

FullCasing Unicode::toTitleDB(int32_t code_point) {
  const UnicodeTypeRecord* record = typeRecord(code_point);
  if ((record->flags & kExtendedCaseMask) == 0) {
    return {code_point + record->title, -1};
  }
  FullCasing result = {-1, -1, -1};
  int32_t index = record->title & 0xFFFF;
  switch (record->title >> 24) {
    default:
      UNREACHABLE("Case mappings are limited to [1..3] code points");
    case 3:
      result.code_points[2] = extendedCaseMapping(index + 2);
      FALLTHROUGH;
    case 2:
      result.code_points[1] = extendedCaseMapping(index + 1);
      FALLTHROUGH;
    case 1:
      result.code_points[0] = extendedCaseMapping(index);
  }
  return result;
}

FullCasing Unicode::toUpperDB(int32_t code_point) {
  const UnicodeTypeRecord* record = typeRecord(code_point);
  if ((record->flags & kExtendedCaseMask) == 0) {
    return {code_point + record->upper, -1};
  }
  FullCasing result = {-1, -1, -1};
  int32_t index = record->upper & 0xFFFF;
  switch (record->upper >> 24) {
    default:
      UNREACHABLE("Case mappings are limited to [1..3] code points");
    case 3:
      result.code_points[2] = extendedCaseMapping(index + 2);
      FALLTHROUGH;
    case 2:
      result.code_points[1] = extendedCaseMapping(index + 1);
      FALLTHROUGH;
    case 1:
      result.code_points[0] = extendedCaseMapping(index);
  }
  return result;
}

}  // namespace py
