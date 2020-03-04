#include "unicode.h"

#include <cstdint>

#include "unicode-db.h"

namespace py {

bool Unicode::isLowerDB(int32_t code_point) {
  return (typeRecord(code_point)->flags & kLowerMask) != 0;
}

bool Unicode::isTitleDB(int32_t code_point) {
  return (typeRecord(code_point)->flags & kTitleMask) != 0;
}

bool Unicode::isUpperDB(int32_t code_point) {
  return (typeRecord(code_point)->flags & kUpperMask) != 0;
}

}  // namespace py
