#pragma once

#include "globals.h"

#if defined(NDEBUG) && !defined(DCHECK_ALWAYS_ON)
#define DCHECK_IS_ON() 0
#else
#define DCHECK_IS_ON() 1
#endif

#define CHECK(expr, ...)                                                       \
  do {                                                                         \
    if (UNLIKELY(!(expr))) {                                                   \
      checkFailed(__FILE__, __LINE__, __func__, "check '" #expr "' failed",    \
                  __VA_ARGS__);                                                \
    }                                                                          \
  } while (0)

#define CHECK_INDEX(index, high)                                               \
  do {                                                                         \
    if (UNLIKELY(!((index >= 0) && (index < high)))) {                         \
      checkIndexFailed(__FILE__, __LINE__, __func__, static_cast<word>(index), \
                       static_cast<word>(high));                               \
    }                                                                          \
  } while (0)

#define CHECK_BOUND(val, high)                                                 \
  do {                                                                         \
    if (UNLIKELY(!((val >= 0) && (val <= high)))) {                            \
      checkBoundFailed(__FILE__, __LINE__, __func__, static_cast<word>(val),   \
                       static_cast<word>(high));                               \
    }                                                                          \
  } while (0)

#if DCHECK_IS_ON()
#define DCHECK(...) CHECK(__VA_ARGS__)
#define DCHECK_BOUND(val, high) CHECK_BOUND(val, high)
#define DCHECK_INDEX(index, high) CHECK_INDEX(index, high)
#else
#define DCHECK(...)                                                            \
  if (false) {                                                                 \
    CHECK(__VA_ARGS__);                                                        \
  }
#define DCHECK_BOUND(val, high)                                                \
  if (false) {                                                                 \
    CHECK_BOUND(val, high);                                                    \
  }
#define DCHECK_INDEX(index, high)                                              \
  if (false) {                                                                 \
    CHECK_INDEX(index, high);                                                  \
  }
#endif

#define UNIMPLEMENTED(...)                                                     \
  checkFailed(__FILE__, __LINE__, __func__, "unimplemented", __VA_ARGS__)

#define UNREACHABLE(...)                                                       \
  checkFailed(__FILE__, __LINE__, __func__, "unreachable", __VA_ARGS__)

[[noreturn]] void checkFailed(const char* file, int line, const char* func,
                              const char* expr, ...);
[[noreturn]] void checkIndexFailed(const char* file, int line, const char* func,
                                   word index, word high);
[[noreturn]] void checkBoundFailed(const char* file, int line, const char* func,
                                   word value, word high);
