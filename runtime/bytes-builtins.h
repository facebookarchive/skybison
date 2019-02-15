#pragma once

#include "objects.h"
#include "runtime.h"

namespace python {

// Converts a non-bytes obj to bytes, using `__bytes__` if possible.
// If the object does not have `__bytes__`, then calls bytesFromIterable.
RawObject asBytes(Thread* thread, const Object& obj);

// Converts obj, which should be list, tuple, or iterable, into bytes.
// Assumes that obj is not bytes. Fails if obj is an instance of str.
// Shared between PyObject_Bytes and PyBytes_FromObject.
RawObject bytesFromIterable(Thread* thread, const Object& obj);

// Creates a new Bytes from the first size elements of items.
RawObject bytesFromTuple(Thread* thread, const Tuple& items, word size);

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

 private:
  static const BuiltinMethod kBuiltinMethods[];

  DISALLOW_IMPLICIT_CONSTRUCTORS(BytesBuiltins);
};

}  // namespace python
