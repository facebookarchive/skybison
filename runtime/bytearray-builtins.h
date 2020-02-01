#pragma once

#include "runtime.h"

namespace py {

// Appends a byte to the end of the array.
inline void byteArrayAdd(Thread* thread, Runtime* runtime,
                         const ByteArray& array, byte value) {
  runtime->byteArrayExtend(thread, array, View<byte>(&value, 1));
}

// Returns a new RawBytes containing the bytes in the array.
RawObject byteArrayAsBytes(Thread* thread, Runtime* runtime,
                           const ByteArray& array);

// Returns a new Str containing the repr of self. On error, raise & return
// OverflowError.
RawObject byteArrayRepr(Thread* thread, const ByteArray& self);

// Writes the value to the array as two hex digits.
void writeByteAsHexDigits(Thread* thread, const ByteArray& array, byte value);

RawObject METH(bytearray, __add__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(bytearray, __eq__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(bytearray, __ge__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(bytearray, __gt__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(bytearray, __iadd__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(bytearray, __imul__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(bytearray, __iter__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(bytearray, __le__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(bytearray, __len__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(bytearray, __lt__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(bytearray, __mul__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(bytearray, __ne__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(bytearray, __new__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(bytearray, __repr__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(bytearray, hex)(Thread* thread, Frame* frame, word nargs);
RawObject METH(bytearray, lstrip)(Thread* thread, Frame* frame, word nargs);
RawObject METH(bytearray, rstrip)(Thread* thread, Frame* frame, word nargs);
RawObject METH(bytearray, strip)(Thread* thread, Frame* frame, word nargs);
RawObject METH(bytearray, translate)(Thread* thread, Frame* frame, word nargs);

class ByteArrayBuiltins
    : public Builtins<ByteArrayBuiltins, ID(bytearray), LayoutId::kByteArray> {
 public:
  static const BuiltinAttribute kAttributes[];
  static const BuiltinMethod kBuiltinMethods[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(ByteArrayBuiltins);
};

RawObject METH(bytearray_iterator, __iter__)(Thread* thread, Frame* frame,
                                             word nargs);
RawObject METH(bytearray_iterator, __length_hint__)(Thread* thread,
                                                    Frame* frame, word nargs);
RawObject METH(bytearray_iterator, __next__)(Thread* thread, Frame* frame,
                                             word nargs);

class ByteArrayIteratorBuiltins
    : public Builtins<ByteArrayIteratorBuiltins, ID(bytearray_iterator),
                      LayoutId::kByteArrayIterator> {
 public:
  static const BuiltinMethod kBuiltinMethods[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(ByteArrayIteratorBuiltins);
};

}  // namespace py
