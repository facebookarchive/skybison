#pragma once

#include <climits>
#include <cstdio>
#include <cstdlib>
#include <iosfwd>
#include <memory>
#include <type_traits>

#include "globals.h"

#define CHECK(expr, ...)                                                       \
  do {                                                                         \
    if (!(expr)) {                                                             \
      fprintf(stderr, "%s:%d assertion '%s' failed: ", __FILE__, __LINE__,     \
              #expr);                                                          \
      fprintf(stderr, __VA_ARGS__);                                            \
      fputc('\n', stderr);                                                     \
      python::Utils::printTraceback();                                         \
      std::abort();                                                            \
    }                                                                          \
  } while (0)

#define CHECK_INDEX(index, high)                                               \
  do {                                                                         \
    if (!((index >= 0) && (index < high))) {                                   \
      fprintf(                                                                 \
          stderr, "%s:%d index out of range, %ld not in 0..%ld : ", __FILE__,  \
          __LINE__, static_cast<word>(index), static_cast<word>(high) - 1);    \
      fputc('\n', stderr);                                                     \
      python::Utils::printTraceback();                                         \
      std::abort();                                                            \
    }                                                                          \
  } while (0)

#define CHECK_BOUND(val, high)                                                 \
  do {                                                                         \
    if (!((val >= 0) && (val <= high))) {                                      \
      fprintf(stderr,                                                          \
              "%s:%d bounds violation, %ld not in 0..%ld : ", __FILE__,        \
              __LINE__, static_cast<word>(val), static_cast<word>(high));      \
      fputc('\n', stderr);                                                     \
      python::Utils::printTraceback();                                         \
      std::abort();                                                            \
    }                                                                          \
  } while (0)

#ifdef NDEBUG
#define DCHECK(...)
#define DCHECK_BOUND(val, high)
#define DCHECK_INDEX(index, high)
#else
#define DCHECK(...) CHECK(__VA_ARGS__)
#define DCHECK_BOUND(val, high) CHECK_BOUND(val, high)
#define DCHECK_INDEX(index, high) CHECK_INDEX(index, high)
#endif

#define UNIMPLEMENTED(...)                                                     \
  do {                                                                         \
    fprintf(stderr, "%s:%d unimplemented: ", __FILE__, __LINE__);              \
    fprintf(stderr, __VA_ARGS__);                                              \
    fputc('\n', stderr);                                                       \
    python::Utils::printTraceback();                                           \
    std::abort();                                                              \
  } while (0)

#define UNREACHABLE(...)                                                       \
  do {                                                                         \
    fprintf(stderr, "%s:%d unreachable: ", __FILE__, __LINE__);                \
    fprintf(stderr, __VA_ARGS__);                                              \
    fputc('\n', stderr);                                                       \
    python::Utils::printTraceback();                                           \
    std::abort();                                                              \
  } while (0)

namespace python {

class Utils {
 public:
  template <typename T>
  static bool isAligned(T x, int n) {
    DCHECK(isPowerOfTwo(n), "must be power of 2");
    return (x & (n - 1)) == 0;
  }

  template <typename T>
  static bool isPowerOfTwo(T x) {
    return (x & (x - 1)) == 0;
  }

  template <typename T>
  static T roundDown(T x, int n) {
    DCHECK(isPowerOfTwo(n), "must be power of 2");
    return (x & -n);
  }

  template <typename T>
  static T roundUp(T x, int n) {
    return roundDown(x + n - 1, n);
  }

  template <typename T>
  static T nextPowerOfTwo(T x) {
    // Turn off all but msb.
    while ((x & (x - 1u)) != 0) {
      x &= x - 1u;
    }
    return x << 1u;
  }

  template <typename T>
  static T rotateLeft(T x, int n) {
    return (x << n) | (x >> (-n & (sizeof(T) * kBitsPerByte - 1)));
  }

  template <typename T>
  static T maximum(T x, T y) {
    return x > y ? x : y;
  }

  template <typename T>
  static T minimum(T x, T y) {
    return x < y ? x : y;
  }

  static int highestBit(word x) {
    return x == 0 ? 0 : kBitsPerWord - __builtin_clzl(x);
  }

  // Returns the number of leading redundant sign bits.
  // This is equivalent to gccs __builtin_clrsbl but works for any compiler.
  static int numRedundantSignBits(word x) {
#if __has_builtin(__builtin_clrsbl) ||                                         \
    (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 7))
    static_assert(sizeof(x) == sizeof(long),
                  "Need to choose matching clrsbX builtin");
    return __builtin_clrsbl(x);
#else
    if (x < 0) x = ~x;
    if (x == 0) return sizeof(x) * kBitsPerByte - 1;
    return __builtin_clzl(x) - 1;
#endif
  }

  // Prints a python level stack trace to std::cerr or the ostream of your
  // choice.
  static void printTraceback();
  static void printTraceback(std::ostream* os);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(Utils);
};

// Convenience wrappers to enable templates based on signed or unsigned integral
// types.
template <typename T, typename R>
using if_signed_t = typename std::enable_if<std::is_signed<T>::value, R>::type;
template <typename T, typename R>
using if_unsigned_t =
    typename std::enable_if<std::is_unsigned<T>::value, R>::type;

// std::unique_ptr for objects created with std::malloc() rather than new.
struct FreeDeleter {
  void operator()(void* ptr) const { std::free(ptr); }
};
template <typename T>
using unique_c_ptr = std::unique_ptr<T, FreeDeleter>;

}  // namespace python
