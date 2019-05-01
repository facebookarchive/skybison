#pragma once

#include "objects.h"
#include "runtime.h"

namespace python {

// Converts the bytes into a string, mapping each byte to two hex characters.
RawObject bytesHex(Thread* thread, const Bytes& bytes, word length);

// Converts self into a string representation with single quote delimiters.
RawObject bytesReprSingleQuotes(Thread* thread, const Bytes& self);

// Converts self into a string representation.
// Scans self to select an appropriate delimiter (single or double quotes).
RawObject bytesReprSmartQuotes(Thread* thread, const Bytes& self);

class SmallBytesBuiltins
    : public Builtins<SmallBytesBuiltins, SymbolId::kBytes,
                      LayoutId::kSmallBytes, LayoutId::kBytes> {
 public:
  static void postInitialize(Runtime*, const Type& new_type) {
    new_type.setBuiltinBase(kSuperType);
  }

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(SmallBytesBuiltins);
};

class LargeBytesBuiltins
    : public Builtins<LargeBytesBuiltins, SymbolId::kBytes,
                      LayoutId::kLargeBytes, LayoutId::kBytes> {
 public:
  static void postInitialize(Runtime*, const Type& new_type) {
    new_type.setBuiltinBase(kSuperType);
  }

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
  static RawObject dunderLe(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderLen(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderLt(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderMul(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderNe(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderRepr(Thread* thread, Frame* frame, word nargs);

  static RawObject hex(Thread* thread, Frame* frame, word nargs);
  static RawObject join(Thread* thread, Frame* frame, word nargs);
  static RawObject translate(Thread* thread, Frame* frame, word nargs);

  static const BuiltinMethod kBuiltinMethods[];
  static const word kTranslationTableLength = 1 << kBitsPerByte;

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(BytesBuiltins);
};

}  // namespace python
