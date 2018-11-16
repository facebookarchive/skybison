#pragma once

#include <cassert>
#include <cstdio>
#include <cstdlib>

#include "globals.h"

#define CHECK(expr, ...)                                                       \
  do {                                                                         \
    if (!(expr)) {                                                             \
      fprintf(                                                                 \
          stderr, "%s:%d assertion '%s' failed: ", __FILE__, __LINE__, #expr); \
      fprintf(stderr, __VA_ARGS__);                                            \
      std::abort();                                                            \
    }                                                                          \
  } while (0)

namespace python {

class Utils {
 public:
  template <typename T>
  static inline bool isAligned(T x, int n) {
    assert(isPowerOfTwo(n));
    return (x & (n - 1)) == 0;
  }

  template <typename T>
  static inline bool isPowerOfTwo(T x) {
    return (x & (x - 1)) == 0;
  }

  template <typename T>
  static inline T roundDown(T x, int n) {
    assert(isPowerOfTwo(n));
    return (x & -n);
  }

  template <typename T>
  static inline T roundUp(T x, int n) {
    return roundDown(x + n - 1, n);
  }

  template <typename T>
  static inline T rotateLeft(T x, int n) {
    return (x << n) | (x >> (-n & (sizeof(T) * kBitsPerByte - 1)));
  }

  template <typename T>
  static inline T maximum(T x, T y) {
    return x > y ? x : y;
  }

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(Utils);
};

} // namespace python
