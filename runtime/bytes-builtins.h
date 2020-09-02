#pragma once

#include "objects.h"
#include "runtime.h"

namespace py {

const word kByteTranslationTableLength = kMaxByte + 1;

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

// Split bytes into logical lines using \r, \n, or \r\n markers.
// keepends == true keeps the newline characters, keepends == false does not.
// Returns a list with a bytes objects for each line.
RawObject bytesSplitLines(Thread* thread, const Bytes& bytes, word length,
                          bool keepends);

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

// Returns a new Bytes containing the Bytes or MutableBytes subsequence of
// bytes with the given start index and length.
RawObject bytesSubseq(Thread* thread, const Bytes& bytes, word start,
                      word length);

bool bytesIsValidUTF8(RawBytes bytes);

// Test whether bytes are valid UTF-8 except that it also allows codepoints
// from the surrogate range which is technically not valid UTF-8 but allowed
// in strings, because python supports things like UTF-8B (aka surrogateescape).
bool bytesIsValidStr(RawBytes bytes);

void initializeBytesTypes(Thread* thread);

inline word bytesHash(Thread* thread, RawObject object) {
  if (object.isSmallBytes()) {
    return SmallBytes::cast(object).hash();
  }
  DCHECK(object.isLargeBytes(), "expected bytes object");
  return thread->runtime()->valueHash(object);
}

}  // namespace py
