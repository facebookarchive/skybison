#pragma once

#include <cstdint>

typedef unsigned char byte;
typedef short int int16;
typedef int int32;
typedef intptr_t word;
typedef uintptr_t uword;

const int kBitsPerByte = 8;

const int kWordSize = sizeof(word);
const int kPointerSize = sizeof(void*);

const int KiB = 1024;
const int MiB = KiB * KiB;
const int GiB = KiB * KiB * KiB;

#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&) = delete;      \
  void operator=(const TypeName&) = delete

#define DISALLOW_HEAP_ALLOCATION()          \
  void* operator new(size_t size) = delete; \
  void operator delete(void* p) = delete;

#define DISALLOW_IMPLICIT_CONSTRUCTORS(TypeName) \
  TypeName() = delete;                           \
  DISALLOW_COPY_AND_ASSIGN(TypeName)

#define OFFSET_OF(type, field) \
  (reinterpret_cast<intptr_t>(&(reinterpret_cast<type*>(16)->field)) - 16)
