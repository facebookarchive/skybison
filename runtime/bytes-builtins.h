#pragma once

#include "objects.h"
#include "runtime.h"

namespace py {

// Counts distinct occurrences of needle in haystack in the range [start, end).
word bytesCount(const Bytes& haystack, word haystack_len, const Bytes& needle,
                word needle_len, word start, word end);

// Returns a Str object if each byte in bytes is ascii, else Unbound
RawObject bytesDecodeASCII(Thread* thread, const Bytes& bytes);

// Looks for needle in haystack in the range [start, end). Returns the first
// starting index found in that range, or -1 if the needle was not found.
word bytesFind(const Bytes& haystack, word haystack_len, const Bytes& needle,
               word needle_len, word start, word end);

word bytesHash(Thread* thread, RawObject object);

// Converts the bytes into a string, mapping each byte to two hex characters.
RawObject bytesHex(Thread* thread, const Bytes& bytes, word length);

// Like `bytesFind`, but returns the last starting index in [start, end) or -1.
word bytesRFind(const Bytes& haystack, word haystack_len, const Bytes& needle,
                word needle_len, word start, word end);

// Converts bytes into a string representation with single quote delimiters.
RawObject bytesReprSingleQuotes(Thread* thread, const Bytes& bytes);

// Converts bytes into a string representation.
// Scans bytes to select an appropriate delimiter (single or double quotes).
RawObject bytesReprSmartQuotes(Thread* thread, const Bytes& bytes);

// Strips the given characters from the end(s) of the given bytes. For left and
// right variants, strips only the specified side. For space variants, strips
// all ASCII whitespace from the specified side(s).
RawObject bytesStrip(Thread* thread, const Bytes& bytes, word bytes_len,
                     const Bytes& chars, word chars_len);
RawObject bytesStripLeft(Thread* thread, const Bytes& bytes, word bytes_len,
                         const Bytes& chars, word chars_len);
RawObject bytesStripRight(Thread* thread, const Bytes& bytes, word bytes_len,
                          const Bytes& chars, word chars_len);
RawObject bytesStripSpace(Thread* thread, const Bytes& bytes, word len);
RawObject bytesStripSpaceLeft(Thread* thread, const Bytes& bytes, word len);
RawObject bytesStripSpaceRight(Thread* thread, const Bytes& bytes, word len);

bool bytesIsValidUTF8(RawBytes bytes);

// Test whether bytes are valid UTF-8 except that it also allows codepoints
// from the surrogate range which is technically not valid UTF-8 but allowed
// in strings, because python supports things like UTF-8B (aka surrogateescape).
bool bytesIsValidStr(RawBytes bytes);

RawObject bytesUnderlying(Thread* thread, const Object& obj);

class SmallBytesBuiltins
    : public ImmediateBuiltins<SmallBytesBuiltins, ID(smallbytes),
                               LayoutId::kSmallBytes, LayoutId::kBytes> {
 public:
  static void postInitialize(Runtime* runtime, const Type& new_type);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(SmallBytesBuiltins);
};

class LargeBytesBuiltins
    : public Builtins<LargeBytesBuiltins, ID(largebytes), LayoutId::kLargeBytes,
                      LayoutId::kBytes> {
 public:
  static void postInitialize(Runtime* runtime, const Type& new_type);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(LargeBytesBuiltins);
};

class BytesBuiltins
    : public Builtins<BytesBuiltins, ID(bytes), LayoutId::kBytes> {
 public:
  static void postInitialize(Runtime*, const Type& new_type);

  static const BuiltinAttribute kAttributes[];
  static const word kTranslationTableLength = 1 << kBitsPerByte;

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(BytesBuiltins);
};

class BytesIteratorBuiltins
    : public Builtins<BytesIteratorBuiltins, ID(bytes_iterator),
                      LayoutId::kBytesIterator> {
 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(BytesIteratorBuiltins);
};

inline word bytesHash(Thread* thread, RawObject object) {
  if (object.isSmallBytes()) {
    return SmallBytes::cast(object).hash();
  }
  DCHECK(object.isLargeBytes(), "expected bytes object");
  return thread->runtime()->valueHash(object);
}

}  // namespace py
