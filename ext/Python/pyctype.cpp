#include "cpython-func.h"

#include "globals.h"
#include "unicode.h"

namespace py {

PY_EXPORT int Py_ISALNUM_Func(unsigned char c) { return Byte::isAlnum(c); }

PY_EXPORT int Py_ISALPHA_Func(unsigned char c) { return Byte::isAlpha(c); }

PY_EXPORT int Py_ISDIGIT_Func(unsigned char c) { return Byte::isDigit(c); }

PY_EXPORT int Py_ISLOWER_Func(unsigned char c) { return Byte::isLower(c); }

PY_EXPORT int Py_ISSPACE_Func(unsigned char c) { return Byte::isSpace(c); }

PY_EXPORT int Py_ISUPPER_Func(unsigned char c) { return Byte::isUpper(c); }

PY_EXPORT int Py_ISXDIGIT_Func(unsigned char c) { return Byte::isHexDigit(c); }

PY_EXPORT unsigned char Py_TOLOWER_Func(unsigned char c) {
  return Byte::toLower(c);
}

PY_EXPORT unsigned char Py_TOUPPER_Func(unsigned char c) {
  return Byte::toUpper(c);
}

}  // namespace py
