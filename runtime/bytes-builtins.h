#pragma once

#include "objects.h"
#include "runtime.h"

namespace python {

// Looks for needle in haystack in the range [start, end). Returns the first
// starting index found in that range, or -1 if the needle was not found.
word bytesFind(const Bytes& haystack, word haystack_len, const Bytes& needle,
               word needle_len, word start, word end);

// Converts the bytes into a string, mapping each byte to two hex characters.
RawObject bytesHex(Thread* thread, const Bytes& bytes, word length);

// Like `bytesFind`, but returns the last starting index in [start, end) or -1.
word bytesRFind(const Bytes& haystack, word haystack_len, const Bytes& needle,
                word needle_len, word start, word end);

// Converts self into a string representation with single quote delimiters.
RawObject bytesReprSingleQuotes(Thread* thread, const Bytes& self);

// Converts self into a string representation.
// Scans self to select an appropriate delimiter (single or double quotes).
RawObject bytesReprSmartQuotes(Thread* thread, const Bytes& self);

// Returns the internal bytes value of an instance of bytes.
RawObject bytesUnderlying(Thread* thread, const Object& obj);

class SmallBytesBuiltins
    : public Builtins<SmallBytesBuiltins, SymbolId::kSmallBytes,
                      LayoutId::kSmallBytes, LayoutId::kBytes> {
 public:
  static void postInitialize(Runtime* runtime, const Type& new_type);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(SmallBytesBuiltins);
};

class LargeBytesBuiltins
    : public Builtins<LargeBytesBuiltins, SymbolId::kLargeBytes,
                      LayoutId::kLargeBytes, LayoutId::kBytes> {
 public:
  static void postInitialize(Runtime* runtime, const Type& new_type);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(LargeBytesBuiltins);
};

class BytesBuiltins
    : public Builtins<BytesBuiltins, SymbolId::kBytes, LayoutId::kBytes> {
 public:
  static void postInitialize(Runtime*, const Type& new_type);

  static RawObject dunderAdd(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderEq(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderGe(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderGetItem(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderGt(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderHash(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderIter(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderLe(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderLen(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderLt(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderMul(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderNe(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderRepr(Thread* thread, Frame* frame, word nargs);

  static RawObject hex(Thread* thread, Frame* frame, word nargs);
  static RawObject translate(Thread* thread, Frame* frame, word nargs);

  static const BuiltinAttribute kAttributes[];
  static const BuiltinMethod kBuiltinMethods[];
  static const word kTranslationTableLength = 1 << kBitsPerByte;

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(BytesBuiltins);
};

class BytesIteratorBuiltins
    : public Builtins<BytesIteratorBuiltins, SymbolId::kBytesIterator,
                      LayoutId::kBytesIterator> {
 public:
  static RawObject dunderIter(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderLengthHint(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderNext(Thread* thread, Frame* frame, word nargs);

  static const BuiltinMethod kBuiltinMethods[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(BytesIteratorBuiltins);
};

}  // namespace python
