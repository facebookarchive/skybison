#pragma once

#include "frame.h"
#include "handles.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

class StrBuiltins {
 public:
  static void initialize(Runtime* runtime);

  static RawObject dunderAdd(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderEq(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderGe(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderGetItem(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderGt(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderIter(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderLe(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderLen(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderLt(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderMod(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderNe(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderNew(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderRepr(Thread* thread, Frame* frame, word nargs);
  static RawObject lower(Thread* thread, Frame* frame, word nargs);
  static RawObject lstrip(Thread* thread, Frame* frame, word nargs);
  static RawObject join(Thread* thread, Frame* frame, word nargs);
  static RawObject rstrip(Thread* thread, Frame* frame, word nargs);
  static RawObject strip(Thread* thread, Frame* frame, word nargs);

 private:
  static word strFormatBufferLength(const Str& fmt, const ObjectArray& args);
  static RawObject strFormat(Thread* thread, const Str& fmt,
                             const ObjectArray& args);
  static void byteToHex(byte** buf, byte convert);
  static const BuiltinMethod kMethods[];

  DISALLOW_IMPLICIT_CONSTRUCTORS(StrBuiltins);
};

class StrIteratorBuiltins {
 public:
  static void initialize(Runtime* runtime);

  static RawObject dunderIter(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderLengthHint(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderNext(Thread* thread, Frame* frame, word nargs);

 private:
  static const BuiltinMethod kMethods[];

  DISALLOW_IMPLICIT_CONSTRUCTORS(StrIteratorBuiltins);
};

}  // namespace python
