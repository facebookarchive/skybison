#pragma once

#include "frame.h"
#include "handles.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

// TODO(T38930404): Make this part of the interface instead of an ugly enum
enum class StrStripDirection { Left, Right, Both };

RawObject strConcat(Thread* thread, const Str& left, const Str& right);
RawObject strJoin(Thread* thread, const Str& sep, const Tuple& items,
                  word allocated);
RawObject strStripSpace(Thread* thread, const Str& src,
                        const StrStripDirection direction);
RawObject strStrip(Thread* thread, const Str& src, const Str& str,
                   StrStripDirection direction);
RawObject strSubstr(Thread* thread, const Str& str, word start, word length);

// Returns the length of the maximum initial span of src composed of code
// points found in str. Analogous to the C string API function strspn().
word strSpan(const Str& src, const Str& str);

// Returns the length of the maximum final span of src composed
// of code points found in str. Right handed version of strSpan(). The rend
// argument is the index at which to stop scanning left, and could be set to 0
// to scan the whole string.
word strRSpan(const Str& src, const Str& str, word rend);

RawObject strIteratorNext(Thread* thread, StrIterator& iter);

class SmallStrBuiltins {
 public:
  static void initialize(Runtime* runtime);
};

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
  static RawObject slice(Thread* thread, const Str& str, const Slice& slice);
  static RawObject strFormatBufferLength(Thread* thread, const Str& fmt,
                                         const Tuple& args);
  static RawObject strFormat(Thread* thread, const Str& fmt, const Tuple& args);
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
