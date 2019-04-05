#pragma once

#include "objects.h"
#include "runtime.h"

namespace python {

// Converts a non-bytes object to bytes with `__bytes__` if it exists.
// If the object does not have `__bytes__`, then returns None.
RawObject callDunderBytes(Thread* thread, const Object& obj);

// Converts obj, which should be list, tuple, or iterable, into bytes.
// Assumes that obj is not bytes. Fails if obj is an instance of str.
// Shared between PyObject_Bytes and PyBytes_FromObject.
RawObject bytesFromIterable(Thread* thread, const Object& obj);

// Creates a new Bytes from the first size elements of items.
RawObject bytesFromTuple(Thread* thread, const Tuple& items, word size);

// Converts the bytes into a string, mapping each byte to two hex characters.
RawObject bytesHex(Thread* thread, const Bytes& bytes, word length);

// Converts self into a string representation with single quote delimiters.
RawObject bytesReprSingleQuotes(Thread* thread, const Bytes& self);

// Converts self into a string representation.
// Scans self to select an appropriate delimiter (single or double quotes).
RawObject bytesReprSmartQuotes(Thread* thread, const Bytes& self);

RawObject underBytesNew(Thread* thread, Frame* frame, word nargs);

class BytesBuiltins
    : public Builtins<BytesBuiltins, SymbolId::kBytes, LayoutId::kBytes> {
 public:
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

  static const BuiltinMethod kBuiltinMethods[];
  static const word kTranslationTableLength = 1 << kBitsPerByte;

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(BytesBuiltins);
};

}  // namespace python
