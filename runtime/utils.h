#pragma once

#include <climits>
#include <cstdio>
#include <cstdlib>
#include <iosfwd>
#include <memory>
#include <type_traits>

#include "globals.h"

#if defined(NDEBUG) && !defined(DCHECK_ALWAYS_ON)
#define DCHECK_IS_ON() 0
#else
#define DCHECK_IS_ON() 1
#endif

#define CHECK(expr, ...)                                                       \
  do {                                                                         \
    if (!(expr)) {                                                             \
      fprintf(stderr, "%s:%d %s: assertion '%s' failed: ", __FILE__, __LINE__, \
              __func__, #expr);                                                \
      fprintf(stderr, __VA_ARGS__);                                            \
      fputc('\n', stderr);                                                     \
      py::Utils::printDebugInfoAndAbort();                                     \
    }                                                                          \
  } while (0)

#define CHECK_INDEX(index, high)                                               \
  do {                                                                         \
    if (!((index >= 0) && (index < high))) {                                   \
      fprintf(stderr,                                                          \
              "%s:%d %s: index out of range, %ld not in 0..%ld : ", __FILE__,  \
              __LINE__, __func__, static_cast<word>(index),                    \
              static_cast<word>(high) - 1);                                    \
      fputc('\n', stderr);                                                     \
      py::Utils::printDebugInfoAndAbort();                                     \
    }                                                                          \
  } while (0)

#define CHECK_BOUND(val, high)                                                 \
  do {                                                                         \
    if (!((val >= 0) && (val <= high))) {                                      \
      fprintf(stderr,                                                          \
              "%s:%d %s: bounds violation, %ld not in 0..%ld : ", __FILE__,    \
              __LINE__, __func__, static_cast<word>(val),                      \
              static_cast<word>(high));                                        \
      fputc('\n', stderr);                                                     \
      py::Utils::printDebugInfoAndAbort();                                     \
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
  do {                                                                         \
    fprintf(stderr, "%s:%d %s: unimplemented: ", __FILE__, __LINE__,           \
            __func__);                                                         \
    fprintf(stderr, __VA_ARGS__);                                              \
    fputc('\n', stderr);                                                       \
    py::Utils::printDebugInfoAndAbort();                                       \
  } while (0)

#define UNREACHABLE(...)                                                       \
  do {                                                                         \
    fprintf(stderr, "%s:%d %s: unreachable: ", __FILE__, __LINE__, __func__);  \
    fprintf(stderr, __VA_ARGS__);                                              \
    fputc('\n', stderr);                                                       \
    py::Utils::printDebugInfoAndAbort();                                       \
  } while (0)

namespace py {

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
  static bool fits(word value) {
    return static_cast<word>(static_cast<T>(value)) == value;
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
  static T roundUpDiv(T denominator, int divisor) {
    if (isPowerOfTwo(divisor)) {
      return roundUp(denominator, divisor) >> (highestBit(divisor) - 1);
    }
    return (denominator + (divisor - 1)) / divisor;
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

  // Search forwards through `haystack` looking for `needle`. Return the byte
  // offset, or -1 if not found.
  static word memoryFind(byte* haystack, word haystack_len, byte* needle,
                         word needle_len);

  // Search forwards through `haystack` looking for `needle`. Return the byte
  // offset, or -1 if not found.
  static word memoryFindChar(byte* haystack, byte needle, word length);

  // Search backwards through `haystack` looking for `needle`. Return the byte
  // offset, or -1 if not found.
  static word memoryFindCharReverse(byte* haystack, byte needle, word length);

  // Search backwards through `haystack` looking for `needle`. Return the byte
  // offset, or -1 if not found.
  static word memoryFindReverse(byte* haystack, word haystack_len, byte* needle,
                                word needle_len);

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

  template <typename T>
  static T readBytes(void* addr) {
    T dest;
    std::memcpy(&dest, addr, sizeof(dest));
    return dest;
  }

  // Prints a python level stack trace to stderr or the stream of your choice.
  static void printTraceback(std::ostream* os);
  static void printTracebackToStderr();

  // Print the current traceback, information about the pending exception, if
  // one is set, and call std::abort().
  [[noreturn]] static void printDebugInfoAndAbort();

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

}  // namespace py
