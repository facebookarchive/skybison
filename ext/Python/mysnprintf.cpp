#include <climits>
#include <cstdarg>
#include <cstdio>

#include "cpython-func.h"

#include "globals.h"
#include "utils.h"

namespace py {

PY_EXPORT int PyOS_snprintf(char* str, size_t size, const char* format, ...) {
  va_list va;
  va_start(va, format);
  int result = PyOS_vsnprintf(str, size, format, va);
  va_end(va);
  return result;
}

PY_EXPORT int PyOS_vsnprintf(char* str, size_t size, const char* format,
                             va_list va) {
  DCHECK(str != nullptr, "string to write to cannot be null");
  DCHECK(size > 0, "number of characters to write must be positive");
  DCHECK(format != nullptr, "format string cannot be null");
  if (size > (INT_MAX - 1)) {
    str[size - 1] = '\0';
    return -666;
  }
  return std::vsnprintf(str, size, format, va);
}

}  // namespace py
