#pragma once

#include "frame.h"
#include "handles.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

RawObject strEscapeNonASCII(Thread* thread, const Object& str_obj);
// Look for needle in haystack in the range [start, end]. Return the first
// index found in that range, or -1 if needle was not found.
RawObject strFind(const Str& haystack, const Str& needle, word start, word end);
// Look for needle in haystack in the range [start, end]. Return the last
// index found in that range, or -1 if needle was not found.
RawObject strRFind(const Str& haystack, const Str& needle, word start,
                   word end);

RawObject strStrip(Thread* thread, const Str& src, const Str& str);
RawObject strStripLeft(Thread* thread, const Str& src, const Str& str);
RawObject strStripRight(Thread* thread, const Str& src, const Str& str);

RawObject strStripSpace(Thread* thread, const Str& src);
RawObject strStripSpaceLeft(Thread* thread, const Str& src);
RawObject strStripSpaceRight(Thread* thread, const Str& src);

// Split the string into logical lines using \r, \n, and other end-of-line
// markers. keepends == true keeps the newline characters, keepends == false
// does not.
//
// Returns a list of component strings, either with or without endlines.
RawObject strSplitlines(Thread* thread, const Str& str, bool keepends);

// Returns the length of the maximum initial span of src composed of code
// points found in str. Analogous to the C string API function strspn().
word strSpan(const Str& src, const Str& str);

// Returns the length of the maximum final span of src composed
// of code points found in str. Right handed version of strSpan(). The rend
// argument is the index at which to stop scanning left, and could be set to 0
// to scan the whole string.
word strRSpan(const Str& src, const Str& str, word rend);

// Return the next item from the iterator, or Error if there are no items left.
RawObject strIteratorNext(Thread* thread, const StrIterator& iter);

class SmallStrBuiltins : public Builtins<SmallStrBuiltins, SymbolId::kSmallStr,
                                         LayoutId::kSmallStr, LayoutId::kStr> {
 public:
  static void postInitialize(Runtime*, const Type& new_type) {
    new_type.setBuiltinBase(kSuperType);
  }

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(SmallStrBuiltins);
};

class LargeStrBuiltins : public Builtins<LargeStrBuiltins, SymbolId::kLargeStr,
                                         LayoutId::kLargeStr, LayoutId::kStr> {
 public:
  static void postInitialize(Runtime*, const Type& new_type) {
    new_type.setBuiltinBase(kSuperType);
  }

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(LargeStrBuiltins);
};

class StrBuiltins
    : public Builtins<StrBuiltins, SymbolId::kStr, LayoutId::kStr> {
 public:
  static void postInitialize(Runtime*, const Type& new_type);

  static RawObject dunderAdd(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderBool(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderEq(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderGe(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderGetItem(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderGt(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderHash(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderIter(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderLe(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderLen(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderLt(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderMod(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderMul(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderNe(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderNew(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderRepr(Thread* thread, Frame* frame, word nargs);
  static RawObject lower(Thread* thread, Frame* frame, word nargs);
  static RawObject lstrip(Thread* thread, Frame* frame, word nargs);
  static RawObject join(Thread* thread, Frame* frame, word nargs);
  static RawObject rstrip(Thread* thread, Frame* frame, word nargs);
  static RawObject strip(Thread* thread, Frame* frame, word nargs);
  static RawObject upper(Thread* thread, Frame* frame, word nargs);

  static const BuiltinMethod kBuiltinMethods[];

 private:
  static RawObject slice(Thread* thread, const Str& str, const Slice& slice);
  static RawObject strFormatBufferLength(Thread* thread, const Str& fmt,
                                         const Tuple& args);
  static RawObject strFormat(Thread* thread, const Str& fmt, const Tuple& args);
  static void byteToHex(byte** buf, byte convert);

  DISALLOW_IMPLICIT_CONSTRUCTORS(StrBuiltins);
};

class StrIteratorBuiltins
    : public Builtins<StrIteratorBuiltins, SymbolId::kStrIterator,
                      LayoutId::kStrIterator> {
 public:
  static RawObject dunderIter(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderLengthHint(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderNext(Thread* thread, Frame* frame, word nargs);

  static const BuiltinMethod kBuiltinMethods[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(StrIteratorBuiltins);
};

}  // namespace python
