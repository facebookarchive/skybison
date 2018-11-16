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
      python::Utils::printTraceback();                                         \
      std::abort();                                                            \
    }                                                                          \
  } while (0)

#define UNIMPLEMENTED(...)                                        \
  do {                                                            \
    fprintf(stderr, "%s:%d unimplemented: ", __FILE__, __LINE__); \
    fprintf(stderr, __VA_ARGS__);                                 \
    python::Utils::printTraceback();                              \
    std::abort();                                                 \
  } while (0)

#define UNREACHABLE(...)                                        \
  do {                                                          \
    fprintf(stderr, "%s:%d unreachable: ", __FILE__, __LINE__); \
    fprintf(stderr, __VA_ARGS__);                               \
    python::Utils::printTraceback();                            \
    std::abort();                                               \
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
  static inline T nextPowerOfTwo(T x) {
    // Turn off all but msb.
    while ((x & (x - 1u)) != 0) {
      x &= x - 1u;
    }
    return x << 1u;
  }

  template <typename T>
  static inline T rotateLeft(T x, int n) {
    return (x << n) | (x >> (-n & (sizeof(T) * kBitsPerByte - 1)));
  }

  template <typename T>
  static inline T maximum(T x, T y) {
    return x > y ? x : y;
  }

  template <typename T>
  static inline T minimum(T x, T y) {
    return x < y ? x : y;
  }

  // Prints a python level stack trace to stderr
  static void printTraceback();

  // You can't portably cast function pointers to void*, but we don't need
  // to support platforms where code pointers aren't convertible to data
  // pointers.
  template <typename T>
  static void* castFnPtrToVoid(T* fn) {
    return *reinterpret_cast<void**>(&fn);
  }

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(Utils);
};

} // namespace python
