#pragma once

#include "frame.h"
#include "handles.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace py {

RawObject newStrFromWideChar(Thread* thread, const wchar_t* wc_str);
RawObject newStrFromWideCharWithLength(Thread* thread, const wchar_t* wc_str,
                                       word length);

// Look for needle in haystack in the range [start, end]. Return the number of
// occurrences found in that range. Note that start and end are code point
// offsets, not byte offsets.
RawObject strCount(const Str& haystack, const Str& needle, word start,
                   word end);

// Return the number of occurences of needle in haystack up to max_count.
word strCountSubStr(const Str& haystack, const Str& needle, word max_count);

// Return the number of occurences of needle in haystack[start:end] up to
// max_count. Note that start and end are byte offsets, not code point offsets.
word strCountSubStrFromTo(const Str& haystack, const Str& needle, word start,
                          word end, word max_count);

RawObject strEncodeASCII(Thread* thread, const Str& str);

RawObject strEscapeNonASCII(Thread* thread, const Object& str_obj);

// Look for needle in haystack. Return the first index found in that range, or
// -1 if needle was not found.
word strFind(const Str& haystack, const Str& needle);

// Look for needle in haystack in the range [start, end]. Return the first
// index found in that range, or -1 if needle was not found. Note that start
// and end are code point offsets, not byte offsets.
word strFindWithRange(const Str& haystack, const Str& needle, word start,
                      word end);

word strFindAsciiChar(const Str& haystack, byte needle);

// Find the index of the first non-whitespace character in the string. If there
// are no non-whitespace characters, return the length of the string.
word strFindFirstNonWhitespace(const Str& str);

// Check if str[start:] has the given prefix. Note that start is a byte offset,
// not a code point offset.
bool strHasPrefix(const Str& str, const Str& prefix, word start);

word strHash(Thread* thread, RawObject object);

// Intern strings in place in a tuple of strings
void strInternInTuple(Thread* thread, const Object& items);
// Intern strings in place in a tuple of nested constant structures (str,
// tuple, frozenset)
bool strInternConstants(Thread* thread, const Object& items);

// Look for needle in haystack in the range [start, end). Return the last
// index found in that range, or -1 if needle was not found. Note that start
// and end are code point offsets, not byte offsets.
word strRFind(const Str& haystack, const Str& needle, word start, word end);

word strRFindAsciiChar(const Str& haystack, byte needle);

RawObject strStrip(Thread* thread, const Str& src, const Str& str);
RawObject strStripLeft(Thread* thread, const Str& src, const Str& str);
RawObject strStripRight(Thread* thread, const Str& src, const Str& str);

RawObject strStripSpace(Thread* thread, const Str& src);
RawObject strStripSpaceLeft(Thread* thread, const Str& src);
RawObject strStripSpaceRight(Thread* thread, const Str& src);

// Splits the string `str` into substrings delimited by the non empty string
// `sep`. `maxsplit` limits the number of substrings that will be generated.
//
// Returns a list of strings
RawObject strSplit(Thread* thread, const Str& str, const Str& sep,
                   word maxsplit);

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

// Translates src according to table. Returns a new, translated Str if table
// maps only ASCII code points. Returns Unbound if table maps or contains a
// non-ASCII code point. The src can contain any Unicode.
RawObject strTranslateASCII(Thread* thread, const Str& src, const Str& table);

// Return the next item from the iterator, or Error if there are no items left.
RawObject strIteratorNext(Thread* thread, const StrIterator& iter);

class SmallStrBuiltins
    : public ImmediateBuiltins<SmallStrBuiltins, ID(smallstr),
                               LayoutId::kSmallStr, LayoutId::kStr> {
 public:
  static void postInitialize(Runtime* runtime, const Type& new_type);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(SmallStrBuiltins);
};

class LargeStrBuiltins : public Builtins<LargeStrBuiltins, ID(largestr),
                                         LayoutId::kLargeStr, LayoutId::kStr> {
 public:
  static void postInitialize(Runtime* runtime, const Type& new_type);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(LargeStrBuiltins);
};

class StrBuiltins : public Builtins<StrBuiltins, ID(str), LayoutId::kStr> {
 public:
  static void postInitialize(Runtime*, const Type& new_type);

  static const BuiltinAttribute kAttributes[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(StrBuiltins);
};

class StrIteratorBuiltins
    : public Builtins<StrIteratorBuiltins, ID(str_iterator),
                      LayoutId::kStrIterator> {
 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(StrIteratorBuiltins);
};

inline word strHash(Thread* thread, RawObject object) {
  if (object.isSmallStr()) {
    return SmallStr::cast(object).hash();
  }
  DCHECK(object.isLargeStr(), "expected str object");
  return thread->runtime()->valueHash(object);
}

}  // namespace py
