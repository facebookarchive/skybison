#pragma once

#include <cstddef>
#include <cstdint>

typedef unsigned char byte;
typedef signed char sbyte;
typedef short int int16;
typedef int int32;
typedef long long int64;
typedef unsigned long long uint64;
typedef intptr_t word;
typedef uintptr_t uword;

const int kWordSize = sizeof(word);
const int kPointerSize = sizeof(void*);
const int kDoubleSize = sizeof(double);

const int kBitsPerByte = 8;
const int kBitsPerPointer = kBitsPerByte * kWordSize;
const int kBitsPerWord = kBitsPerByte * kWordSize;

const int16 kMaxInt16 = 0x7FFF;
const int16 kMinInt16 = -kMaxInt16 - 1;
const int32 kMaxInt32 = 0x7FFFFFFF;
const int32 kMinInt32 = -kMaxInt32 - 1;
const int64 kMaxInt64 = 0x7FFFFFFFFFFFFFFFLL;
const int64 kMinInt64 = -kMaxInt64 - 1;
const uint64 kMaxUint64 = 0xFFFFFFFFFFFFFFFFULL;

const word kMinWord = INTPTR_MIN;
const word kMaxWord = INTPTR_MAX;
const uword kMaxUword = UINTPTR_MAX;

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

#define OFFSET_OF(type, field)                                                 \
  (reinterpret_cast<intptr_t>(&(reinterpret_cast<type*>(16)->field)) - 16)

#if __GNUG__ && __GNUC__ < 5
#define IS_TRIVIALLY_COPYABLE(T) __has_trivial_copy(T)
#else
#include <type_traits>
#define IS_TRIVIALLY_COPYABLE(T) std::is_trivially_copyable<T>::value
#endif

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
