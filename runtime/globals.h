#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>

typedef unsigned char byte;
typedef intptr_t word;
typedef uintptr_t uword;

static_assert(sizeof(word) == sizeof(size_t),
              "word must be the same size as size_t");

const int kWordSize = sizeof(word);
const int kPointerSize = sizeof(void*);
const int kDoubleSize = sizeof(double);

const int kWordSizeLog2 = 3;

const int kUwordDigits10 = 19;
const uword kUwordDigits10Pow = 10000000000000000000ul;

const int kBitsPerByte = 8;
const int kBitsPerPointer = kBitsPerByte * kWordSize;
const int kBitsPerWord = kBitsPerByte * kWordSize;
const int kBitsPerDouble = kBitsPerByte * kDoubleSize;

const int kDoubleMantissaBits = 52;

const int16_t kMaxInt16 = INT16_MAX;
const int16_t kMinInt16 = INT16_MIN;
const int32_t kMaxInt32 = INT32_MAX;
const int32_t kMinInt32 = INT32_MIN;
const int64_t kMaxInt64 = INT64_MAX;
const int64_t kMinInt64 = INT64_MIN;
const uint64_t kMaxUint64 = UINT64_MAX;

const byte kMaxByte = 0xFF;

const word kMinWord = INTPTR_MIN;
const word kMaxWord = INTPTR_MAX;
const uword kMaxUword = UINTPTR_MAX;

const int kMaxUnicode = 0x10ffff;
const int kMaxASCII = 127;

const int kKiB = 1024;
const int kMiB = kKiB * kKiB;
const int kGiB = kKiB * kKiB * kKiB;

const int kMillisecondsPerSecond = 1000;
const int kMillsecondsPerMicrosecond = 1000;
const int kMicrosecondsPerSecond =
    kMillisecondsPerSecond * kMillsecondsPerMicrosecond;
const int kNanosecondsPerMicrosecond = 1000;
const int kNanosecondsPerSecond =
    kMicrosecondsPerSecond * kNanosecondsPerMicrosecond;

#if __GNUG__ && __GNUC__ < 5
#define IS_TRIVIALLY_COPYABLE(T) __has_trivial_copy(T)
#else
#define IS_TRIVIALLY_COPYABLE(T) std::is_trivially_copyable<T>::value
#endif

template <typename D, typename S>
inline D bit_cast(const S& src) {
  static_assert(sizeof(S) == sizeof(D), "src and dst must be the same size");
  static_assert(IS_TRIVIALLY_COPYABLE(S), "src must be trivially copyable");
  static_assert(std::is_trivial<D>::value, "dst must be trivial");
  D dst;
  std::memcpy(&dst, &src, sizeof(dst));
  return dst;
}

#define ARRAYSIZE(x) (sizeof(x) / sizeof((x)[0]))

#define DISALLOW_COPY_AND_ASSIGN(TypeName)                                     \
  TypeName(const TypeName&) = delete;                                          \
  void operator=(const TypeName&) = delete

#define DISALLOW_HEAP_ALLOCATION()                                             \
  void* operator new(size_t size) = delete;                                    \
  void operator delete(void* p) = delete

#define DISALLOW_IMPLICIT_CONSTRUCTORS(TypeName)                               \
  TypeName() = delete;                                                         \
  DISALLOW_COPY_AND_ASSIGN(TypeName)

#define PY_EXPORT extern "C"

// FORMAT_ATTRIBUTE allows typechecking by the compiler.
// string_index: The function argument index where the format index is.
//               this has the implicit index 1.
// varargs_index: The function argument index where varargs start.
// The archetype chosen is printf since this is being used for output strings.
#define FORMAT_ATTRIBUTE(string_index, first_to_check)                         \
  __attribute__((format(printf, string_index, first_to_check)))

// Branch prediction hints for the compiler.  Use in performance critial code
// which almost always branches one way.
#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

// Placeholders for [[nodiscard]] until we move to C++17
#define NODISCARD __attribute__((warn_unused_result))
#define WARN_UNUSED __attribute__((warn_unused))

#define ALIGN_16 __attribute__((aligned(16)))

#ifndef __has_builtin
#define __has_builtin(x) 0
#endif

// Endian enum (as proposed in the C++20 draft).
enum class endian {
// Implementation for gcc + clang compilers.
#if defined(__ORDER_LITTLE_ENDIAN__) && defined(__ORDER_BIG_ENDIAN__) &&       \
    defined(__BYTE_ORDER__)
  little = __ORDER_LITTLE_ENDIAN__,
  big = __ORDER_BIG_ENDIAN__,
  native = __BYTE_ORDER__
#else
#error "endian class not implemented for this compiler"
#endif
};
