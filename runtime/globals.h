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

const int kByteSize = sizeof(byte);
const int kDoubleSize = sizeof(double);
const int kFloatSize = sizeof(float);
const int kIntSize = sizeof(int);
const int kLongSize = sizeof(long);
const int kLongLongSize = sizeof(long long);
const int kPointerSize = sizeof(void*);
const int kShortSize = sizeof(short);
const int kWcharSize = sizeof(wchar_t);
const int kWordSize = sizeof(word);

const int kWordSizeLog2 = 3;

const int kUwordDigits10 = 19;
const uword kUwordDigits10Pow = 10000000000000000000ul;
const int kBitsPerHexDigit = 4;
const int kBitsPerOctDigit = 3;

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
const uint32_t kMaxUint32 = UINT32_MAX;

const byte kMaxByte = 0xFF;

const word kMinWord = INTPTR_MIN;
const word kMaxWord = INTPTR_MAX;
const uword kMaxUword = UINTPTR_MAX;

const int kMaxASCII = 127;
const int kMaxUnicode = 0x10ffff;
const int32_t kReplacementCharacter = 0xFFFD;

const int kKiB = 1024;
const int kMiB = kKiB * kKiB;
const int kGiB = kKiB * kKiB * kKiB;

const int kMillisecondsPerSecond = 1000;
const int kMicrosecondsPerMillisecond = 1000;
const int kMicrosecondsPerSecond =
    kMillisecondsPerSecond * kMicrosecondsPerMillisecond;
const int kNanosecondsPerMicrosecond = 1000;
const int kNanosecondsPerSecond =
    kMicrosecondsPerSecond * kNanosecondsPerMicrosecond;

// Equivalent to _PyHash_BITS. This is NOT the maximum size of a hash value,
// that would is either RawHeader::kHashCodeBits or SmallInt::kMaxValue
// depending on whether the hash value is cached in the object header.
const word kArithmeticHashBits = 61;
// Equivalent to _PyHASH_MODULUS. Should be a mersenne prime.
const word kArithmeticHashModulus = ((word{1} << kArithmeticHashBits) - 1);
const word kHashInf = 314159;
const word kHashNan = 0;
const word kHashImag = 1000003;

#ifndef __has_builtin
#define __has_builtin(x) 0
#endif
#ifndef __has_cpp_atttribute
#define __has_cpp_atttribute(x) 0
#endif

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

#define PY_EXPORT extern "C" __attribute__((visibility("default")))

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

// Old-school version of offsetof() that works with non-standard layout classes.
#define OFFSETOF(ty, memb)                                                     \
  (reinterpret_cast<char*>(&reinterpret_cast<ty*>(16)->memb) -                 \
   reinterpret_cast<char*>(16))

#if __has_cpp_atttribute(nodiscard) ||                                         \
    (__GNUC__ >= 5 || __GNUC__ == 4 && __GNUC_MINOR__ >= 8)
#define NODISCARD [[nodiscard]]
#else
#define NODISCARD __attribute__((warn_unused_result))
#endif

#define ALIGN_16 __attribute__((aligned(16)))

#define ALWAYS_INLINE inline __attribute__((always_inline))

#if defined(__clang__)
#define FALLTHROUGH [[clang::fallthrough]]
#elif defined(__GNUC__)
#define FALLTHROUGH __attribute__((fallthrough))
#else
#define FALLTHROUGH                                                            \
  do {                                                                         \
  } while (0)
#endif

#define NEVER_INLINE __attribute__((noinline))

#define USED __attribute__((used))

#define WARN_UNUSED __attribute__((warn_unused))

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
