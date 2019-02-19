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

// Converts self into a string representation with single quote delimiters.
RawObject bytesReprSingleQuotes(Thread* thread, const Bytes& self);

// Converts self into a string representation.
// Scans self to select an appropriate delimiter (single or double quotes).
RawObject bytesReprSmartQuotes(Thread* thread, const Bytes& self);

class BytesBuiltins {
 public:
  static void initialize(Runtime* runtime);

  static RawObject dunderAdd(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderEq(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderGe(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderGetItem(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderGt(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderLe(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderLen(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderLt(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderNe(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderNew(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderRepr(Thread* thread, Frame* frame, word nargs);

 private:
  static const BuiltinMethod kBuiltinMethods[];

  DISALLOW_IMPLICIT_CONSTRUCTORS(BytesBuiltins);
};

}  // namespace python
