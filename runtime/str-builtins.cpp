#include "str-builtins.h"

#include <cwchar>

#include "builtins.h"
#include "formatter.h"
#include "frame.h"
#include "globals.h"
#include "int-builtins.h"
#include "objects.h"
#include "runtime.h"
#include "set-builtins.h"
#include "slice-builtins.h"
#include "thread.h"
#include "tuple-builtins.h"
#include "unicode.h"

namespace py {

RawObject newStrFromWideChar(Thread* thread, const wchar_t* wc_str) {
  return newStrFromWideCharWithLength(thread, wc_str, std::wcslen(wc_str));
}

RawObject newStrFromWideCharWithLength(Thread* thread, const wchar_t* wc_str,
                                       word length) {
  static_assert(sizeof(*wc_str) * kBitsPerByte == 32,
                "only 32bit wchar_t supported.");

  for (word i = 0; i < length; ++i) {
    if (wc_str[i] < 0 || wc_str[i] > kMaxUnicode) {
      return thread->raiseWithFmt(LayoutId::kValueError,
                                  "character is not in range");
    }
  }

  return thread->runtime()->newStrFromUTF32(
      View<int32_t>(reinterpret_cast<const int32_t*>(wc_str), length));
}

static word strCountCharFromTo(const Str& haystack, byte needle, word start,
                               word end) {
  word result = 0;
  if (haystack.isSmallStr()) {
    RawSmallStr small = SmallStr::cast(*haystack);
    for (word i = start; i < end; i++) {
      if (small.charAt(i) == needle) result++;
    }
    return result;
  }
  byte* ptr = reinterpret_cast<byte*>(LargeStr::cast(*haystack).address());
  for (word i = start; i < end; i++) {
    if (ptr[i] == needle) result++;
  }
  return result;
}

RawObject strCount(const Str& haystack, const Str& needle, word start,
                   word end) {
  if (end < 0 || start < 0) {
    // N.B.: If end is negative we may be able to cheaply walk backward. We
    // should avoid calling adjustSearchIndices here since the underlying
    // container is not O(1) and replace it with something that preserves some
    // of the signals that would be useful to lower the cost of the O(n)
    // traversal.
    // TODO(T41400083): Use a different search algorithm
    Slice::adjustSearchIndices(&start, &end, haystack.codePointLength());
  }

  word start_index = start == 0 ? 0 : haystack.offsetByCodePoints(0, start);
  if (start_index == haystack.charLength() && needle.charLength() > 0) {
    // Haystack is too small; fast early return
    return SmallInt::fromWord(0);
  }

  word end_index = end == kMaxWord
                       ? haystack.charLength()
                       : haystack.offsetByCodePoints(start_index, end - start);
  if ((end_index - start_index) < needle.charLength() ||
      start_index > end_index) {
    // Haystack is too small; fast early return
    return SmallInt::fromWord(0);
  }

  if (needle.charLength() == 1) {
    return SmallInt::fromWord(strCountCharFromTo(
        haystack, SmallStr::cast(*needle).charAt(0), start_index, end_index));
  }

  // TODO(T41400083): Use a different search algorithm
  return SmallInt::fromWord(strCountSubStrFromTo(
      haystack, needle, start_index, end_index, haystack.charLength()));
}

word strCountSubStrFromTo(const Str& haystack, const Str& needle, word start,
                          word end, word max_count) {
  DCHECK(max_count >= 0, "max_count must be non-negative");
  word needle_len = needle.charLength();
  word num_match = 0;
  // Loop is in byte space, not code point space
  for (word i = start; i <= end - needle_len && num_match < max_count;) {
    if (strHasPrefix(haystack, needle, i)) {
      i += needle_len;
      num_match++;
      continue;
    }
    i++;
  }
  return num_match;
}

word strCountSubStr(const Str& haystack, const Str& needle, word max_count) {
  return strCountSubStrFromTo(haystack, needle, 0, haystack.charLength(),
                              max_count);
}

RawObject strEncodeASCII(Thread* thread, const Str& str) {
  HandleScope scope(thread);
  if (!str.isASCII()) {
    return Unbound::object();
  }
  if (str.isSmallStr()) {
    return SmallStr::cast(*str).becomeBytes();
  }
  word str_len = str.charLength();
  MutableBytes bytes(&scope,
                     thread->runtime()->newMutableBytesUninitialized(str_len));
  bytes.replaceFromWithStr(0, *str, str_len);
  return bytes.becomeImmutable();
}

// TODO(T39861344): This can be replaced by a real string codec.
RawObject strEscapeNonASCII(Thread* thread, const Object& str_obj) {
  CHECK(str_obj.isStr(), "strEscapeNonASCII cannot currently handle non-str");
  HandleScope scope(thread);
  Str str(&scope, *str_obj);
  for (word i = 0; i < str.charLength(); i++) {
    if (str.charAt(i) > kMaxASCII) {
      UNIMPLEMENTED(
          "Character '%d' at index %ld is not yet escapable in strEscape",
          str.charAt(i), i);
    }
  }
  return *str;
}

word strSpan(const Str& src, const Str& str) {
  word length = src.charLength();
  word str_length = str.charLength();
  word first = 0;
  for (; first < length; first++) {
    bool has_match = false;
    byte ch = src.charAt(first);
    for (word j = 0; j < str_length; j++) {
      if (ch == str.charAt(j)) {
        has_match = true;
        break;
      }
    }
    if (!has_match) {
      break;
    }
  }
  return first;
}

word strRSpan(const Str& src, const Str& str, word rend) {
  DCHECK(rend >= 0, "string index underflow");
  word length = src.charLength();
  word str_length = str.charLength();
  word result = 0;
  for (word i = length - 1; i >= rend; i--, result++) {
    byte ch = src.charAt(i);
    bool has_match = false;
    for (word j = 0; j < str_length; j++) {
      if (ch == str.charAt(j)) {
        has_match = true;
        break;
      }
    }
    if (!has_match) {
      break;
    }
  }
  return result;
}

static bool isLineBreak(int32_t c) {
  switch (c) {
    // Common cases
    case '\n':
    case '\r':
    // Less common cases
    case '\f':
    case '\v':
    case 0x1c:
    case 0x1d:
    case 0x1e:
    case 0x85:
    case 0x2028:
    case 0x2029:
      return true;
    default:
      return false;
  }
}

RawObject strSplit(Thread* thread, const Str& str, const Str& sep,
                   word maxsplit) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  word num_splits = strCountSubStr(str, sep, maxsplit);
  word result_len = num_splits + 1;
  MutableTuple result_items(&scope, runtime->newMutableTuple(result_len));
  word last_idx = 0;
  word sep_len = sep.charLength();
  for (word i = 0, result_idx = 0; result_idx < num_splits;) {
    if (strHasPrefix(str, sep, i)) {
      result_items.atPut(result_idx++, runtime->strSubstr(thread, str, last_idx,
                                                          i - last_idx));
      i += sep_len;
      last_idx = i;
    } else {
      i = str.offsetByCodePoints(i, 1);
    }
  }
  result_items.atPut(
      num_splits,
      runtime->strSubstr(thread, str, last_idx, str.charLength() - last_idx));
  List result(&scope, runtime->newList());
  result.setItems(*result_items);
  result.setNumItems(result_len);
  return *result;
}

RawObject strSplitlines(Thread* thread, const Str& str, bool keepends) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  List result(&scope, runtime->newList());
  // Looping over code points, not bytes, but i is a byte offset
  for (word i = 0, j = 0; i < str.charLength(); j = i) {
    // Skip newline chars
    word num_bytes;
    while (i < str.charLength() &&
           !isLineBreak(str.codePointAt(i, &num_bytes))) {
      i += num_bytes;
    }

    word eol_pos = i;
    if (i < str.charLength()) {
      int32_t cp = str.codePointAt(i, &num_bytes);
      word next = i + num_bytes;
      word next_num_bytes;
      // Check for \r\n specifically
      if (cp == '\r' && next < str.charLength() &&
          str.codePointAt(next, &next_num_bytes) == '\n') {
        i += next_num_bytes;
      }
      i += num_bytes;
      if (keepends) {
        eol_pos = i;
      }
    }

    // If there are no newlines, the str returned should be identity-equal
    if (j == 0 && eol_pos == str.charLength() && str.isStr()) {
      runtime->listAdd(thread, result, str);
      return *result;
    }

    Str substr(&scope, runtime->strSubstr(thread, str, j, eol_pos - j));
    runtime->listAdd(thread, result, substr);
  }

  return *result;
}

RawObject strStripSpace(Thread* thread, const Str& src) {
  word length = src.charLength();
  if (length == 0) {
    return *src;
  }
  if (length == 1 && ASCII::isSpace(src.charAt(0))) {
    return Str::empty();
  }
  word first = 0;
  while (first < length) {
    word num_bytes;
    int32_t ch = src.codePointAt(first, &num_bytes);
    if (!Unicode::isSpace(ch)) {
      break;
    }
    first += num_bytes;
  }
  word last = length;
  while (last > first) {
    last = src.offsetByCodePoints(last, -1);
    word num_bytes;
    int32_t ch = src.codePointAt(last, &num_bytes);
    if (!Unicode::isSpace(ch)) {
      last += num_bytes;
      break;
    }
  }
  return thread->runtime()->strSubstr(thread, src, first, last - first);
}

RawObject strStripSpaceLeft(Thread* thread, const Str& src) {
  word length = src.charLength();
  if (length == 0) {
    return *src;
  }
  if (length == 1 && ASCII::isSpace(src.charAt(0))) {
    return Str::empty();
  }
  word first = 0;
  while (first < length) {
    word num_bytes;
    int32_t ch = src.codePointAt(first, &num_bytes);
    if (!Unicode::isSpace(ch)) {
      break;
    }
    first += num_bytes;
  }
  return thread->runtime()->strSubstr(thread, src, first, length - first);
}

RawObject strStripSpaceRight(Thread* thread, const Str& src) {
  word length = src.charLength();
  if (length == 0) {
    return *src;
  }
  if (length == 1 && ASCII::isSpace(src.charAt(0))) {
    return Str::empty();
  }
  word last = length;
  while (last > 0) {
    last = src.offsetByCodePoints(last, -1);
    word num_bytes;
    int32_t ch = src.codePointAt(last, &num_bytes);
    if (!Unicode::isSpace(ch)) {
      last += num_bytes;
      break;
    }
  }
  return thread->runtime()->strSubstr(thread, src, 0, last);
}

RawObject strStrip(Thread* thread, const Str& src, const Str& str) {
  word length = src.charLength();
  if (length == 0 || str.charLength() == 0) {
    return *src;
  }
  word first = strSpan(src, str);
  word last = strRSpan(src, str, first);
  return thread->runtime()->strSubstr(thread, src, first,
                                      length - first - last);
}

RawObject strStripLeft(Thread* thread, const Str& src, const Str& str) {
  word length = src.charLength();
  if (length == 0 || str.charLength() == 0) {
    return *src;
  }
  word first = strSpan(src, str);
  return thread->runtime()->strSubstr(thread, src, first, length - first);
}

RawObject strStripRight(Thread* thread, const Str& src, const Str& str) {
  word length = src.charLength();
  if (length == 0 || str.charLength() == 0) {
    return *src;
  }
  word first = 0;
  word last = strRSpan(src, str, first);
  return thread->runtime()->strSubstr(thread, src, 0, length - last);
}

RawObject strIteratorNext(Thread* thread, const StrIterator& iter) {
  HandleScope scope(thread);
  word byte_offset = iter.index();
  Str underlying(&scope, iter.iterable());
  if (byte_offset >= underlying.charLength()) {
    return Error::noMoreItems();
  }
  word num_bytes = 0;
  word code_point = underlying.codePointAt(byte_offset, &num_bytes);
  iter.setIndex(byte_offset + num_bytes);
  return RawSmallStr::fromCodePoint(code_point);
}

void SmallStrBuiltins::postInitialize(Runtime* runtime, const Type& new_type) {
  runtime->setSmallStrType(new_type);
  Layout::cast(new_type.instanceLayout())
      .setDescribedType(runtime->typeAt(kSuperType));
}

void LargeStrBuiltins::postInitialize(Runtime* runtime, const Type& new_type) {
  new_type.setBuiltinBase(kSuperType);
  runtime->setLargeStrType(new_type);
  Layout::cast(new_type.instanceLayout())
      .setDescribedType(runtime->typeAt(kSuperType));
}

const BuiltinAttribute StrBuiltins::kAttributes[] = {
    {ID(_UserStr__value), UserStrBase::kValueOffset, AttributeFlags::kHidden},
    {SymbolId::kSentinelId, 0},
};

void StrBuiltins::postInitialize(Runtime*, const Type& new_type) {
  new_type.setBuiltinBase(LayoutId::kStr);
}

RawObject METH(str, __add__)(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  Object other_obj(&scope, args.get(1));
  if (!runtime->isInstanceOfStr(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(str));
  }
  if (!runtime->isInstanceOfStr(*other_obj)) {
    return thread->raiseRequiresType(other_obj, ID(str));
  }
  Str self(&scope, strUnderlying(*self_obj));
  Str other(&scope, strUnderlying(*other_obj));
  return runtime->strConcat(thread, self, other);
}

RawObject METH(str, __bool__)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfStr(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(str));
  }
  Str self(&scope, strUnderlying(*self_obj));
  return Bool::fromBool(*self != Str::empty());
}

RawObject METH(str, __contains__)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  RawObject self = args.get(0);
  RawObject other = args.get(1);
  Runtime* runtime = thread->runtime();
  bool is_self_invalid = true;
  if (runtime->isInstanceOfStr(self)) {
    is_self_invalid = false;
    if (runtime->isInstanceOfStr(other)) {
      return Bool::fromBool(strUnderlying(self).includes(strUnderlying(other)));
    }
  }
  HandleScope scope(thread);
  Object invalid(&scope, is_self_invalid ? self : other);
  return thread->raiseRequiresType(invalid, ID(str));
}

RawObject METH(str, __eq__)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Object other_obj(&scope, args.get(1));
  if (!thread->runtime()->isInstanceOfStr(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(str));
  }
  if (!thread->runtime()->isInstanceOfStr(*other_obj)) {
    return NotImplementedType::object();
  }
  Str self(&scope, strUnderlying(*self_obj));
  Str other(&scope, strUnderlying(*other_obj));
  return Bool::fromBool(self.equals(*other));
}

RawObject METH(str, __format__)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfStr(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(str));
  }
  Str self(&scope, strUnderlying(*self_obj));

  Object spec_obj(&scope, args.get(1));
  if (!runtime->isInstanceOfStr(*spec_obj)) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "__format__() argument 1 must be str, not %T",
                                &spec_obj);
  }
  Str spec(&scope, strUnderlying(*spec_obj));

  if (spec.charLength() == 0) {
    return *self;
  }

  FormatSpec format;
  Object possible_error(&scope,
                        parseFormatSpec(thread, spec,
                                        /*default_type=*/'s',
                                        /*default_align=*/'<', &format));
  if (!possible_error.isNoneType()) {
    DCHECK(possible_error.isErrorException(), "expected exception");
    return *possible_error;
  }
  if (format.type != 's') {
    return raiseUnknownFormatError(thread, format.type, self_obj);
  }
  if (format.positive_sign != '\0') {
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "Sign not allowed in string format specifier");
  }
  if (format.alternate) {
    return thread->raiseWithFmt(
        LayoutId::kValueError,
        "Alternate form (#) not allowed in string format specifier");
  }
  if (format.alignment == '=') {
    return thread->raiseWithFmt(
        LayoutId::kValueError,
        "'=' alignment not allowed in string format specifier");
  }

  return formatStr(thread, self, &format);
}

RawObject METH(str, __ge__)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Object other_obj(&scope, args.get(1));
  if (!thread->runtime()->isInstanceOfStr(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(str));
  }
  if (!thread->runtime()->isInstanceOfStr(*other_obj)) {
    return NotImplementedType::object();
  }
  Str self(&scope, strUnderlying(*self_obj));
  Str other(&scope, strUnderlying(*other_obj));
  return Bool::fromBool(self.compare(*other) >= 0);
}

RawObject METH(str, __gt__)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Object other_obj(&scope, args.get(1));
  if (!thread->runtime()->isInstanceOfStr(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(str));
  }
  if (!thread->runtime()->isInstanceOfStr(*other_obj)) {
    return NotImplementedType::object();
  }
  Str self(&scope, strUnderlying(*self_obj));
  Str other(&scope, strUnderlying(*other_obj));
  return Bool::fromBool(self.compare(*other) > 0);
}

RawObject METH(str, __hash__)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfStr(*self)) {
    return thread->raiseRequiresType(self, ID(str));
  }
  Str self_str(&scope, strUnderlying(*self));
  return SmallInt::fromWord(strHash(thread, *self_str));
}

void strInternInTuple(Thread* thread, const Object& items) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  DCHECK(runtime->isInstanceOfTuple(*items), "items must be a tuple instance");
  Tuple tuple(&scope, tupleUnderlying(*items));
  Str str(&scope, Str::empty());
  Object result(&scope, NoneType::object());
  for (word i = 0; i < tuple.length(); i++) {
    str = tuple.at(i);
    result = Runtime::internStr(thread, str);
    if (result.isError()) continue;
    if (result != str) {
      tuple.atPut(i, *result);
    }
  }
}

static bool allNameChars(const Str& str) {
  for (word i = 0; i < str.charLength(); i++) {
    if (!isalnum(str.charAt(i))) {
      return false;
    }
  }
  return true;
}

bool strInternConstants(Thread* thread, const Object& items) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  DCHECK(runtime->isInstanceOfTuple(*items), "items must be a tuple instance");
  Tuple tuple(&scope, tupleUnderlying(*items));
  Object obj(&scope, NoneType::object());
  Object result(&scope, NoneType::object());
  bool modified = false;
  for (word i = 0; i < tuple.length(); i++) {
    obj = tuple.at(i);

    if (obj.isStr()) {
      Str str(&scope, *obj);
      if (allNameChars(str)) {
        // if all name chars, intern in place
        result = Runtime::internStr(thread, str);
        if (result.isError()) continue;
        if (result != obj) {
          tuple.atPut(i, *result);
          modified = true;
        }
      }
    } else if (obj.isTuple()) {
      strInternConstants(thread, obj);
    } else if (obj.isFrozenSet()) {
      FrozenSet set(&scope, *obj);
      Tuple data(&scope, set.data());
      Tuple seq(&scope, runtime->newTuple(set.numItems()));
      for (word j = 0, idx = Set::Bucket::kFirst;
           Set::Bucket::nextItem(*data, &idx); j++) {
        seq.atPut(j, Set::Bucket::value(*data, idx));
      }
      if (strInternConstants(thread, seq)) {
        obj = setUpdate(thread, set, seq);
        if (obj.isError()) continue;
        tuple.atPut(i, *obj);
        modified = true;
      }
    }
  }
  return modified;
}

word strFind(const Str& haystack, const Str& needle) {
  word haystack_len = haystack.charLength();
  word needle_len = needle.charLength();
  if (needle_len > haystack_len) {
    return -1;
  }
  if (needle_len == 0) {
    return 0;
  }
  if (needle_len == 1 && haystack.isASCII()) {
    return strFindAsciiChar(haystack, needle.charAt(0));
  }
  // Loop is in byte space, not code point space
  word result = 0;
  // TODO(T41400083): Use a different search algorithm
  for (word i = 0; i <= haystack_len - needle_len; result++) {
    if (strHasPrefix(haystack, needle, i)) {
      return result;
    }
    i = haystack.offsetByCodePoints(i, 1);
  }
  return -1;
}

word strFindWithRange(const Str& haystack, const Str& needle, word start,
                      word end) {
  if (end < 0 || start < 0) {
    Slice::adjustSearchIndices(&start, &end, haystack.codePointLength());
  }

  word start_index = haystack.offsetByCodePoints(0, start);
  if (start_index == haystack.charLength() && needle.charLength() > 0) {
    // Haystack is too small; fast early return
    return -1;
  }
  word end_index = haystack.offsetByCodePoints(start_index, end - start);

  if ((end_index - start_index) < needle.charLength() ||
      start_index > end_index) {
    // Haystack is too small; fast early return
    return -1;
  }

  // Loop is in byte space, not code point space
  word result = start;
  // TODO(T41400083): Use a different search algorithm
  for (word i = start_index; i <= end_index - needle.charLength(); result++) {
    bool has_match = strHasPrefix(haystack, needle, i);
    word next = haystack.offsetByCodePoints(i, 1);
    if (i == next) {
      // We've reached a fixpoint; offsetByCodePoints will not advance past the
      // length of the string.
      if (start_index >= i) {
        // The start is greater than the length of the string.
        return -1;
      }
      // If the start is within bounds, just return the last found index.
      break;
    }
    if (has_match) {
      return result;
    }
    i = next;
  }
  return -1;
}

word strFindAsciiChar(const Str& haystack, byte needle) {
  DCHECK(needle <= kMaxASCII, "must only be called for ASCII `needle`");
  for (word i = 0, length = haystack.charLength(); i < length; i++) {
    if (haystack.charAt(i) == needle) {
      return i;
    }
  }
  return -1;
}

word strFindFirstNonWhitespace(const Str& str) {
  word i = 0;
  for (word codepoint_len, length = str.charLength(); i < length;
       i += codepoint_len) {
    if (!Unicode::isSpace(str.codePointAt(i, &codepoint_len))) return i;
  }
  return i;
}

bool strHasPrefix(const Str& str, const Str& prefix, word start) {
  word str_len = str.charLength();
  word prefix_len = prefix.charLength();
  if (str_len - start < prefix_len) {
    return false;
  }
  for (word i = 0; i < prefix_len; i++) {
    if (str.charAt(start + i) != prefix.charAt(i)) {
      return false;
    }
  }
  return true;
}

word strRFind(const Str& haystack, const Str& needle, word start, word end) {
  // Haystack slice is empty; fast early return
  if (start > end) return -1;
  // Needle is empty
  if (needle == Str::empty()) return end;
  word start_index = haystack.offsetByCodePoints(0, start);
  if (start_index == haystack.charLength()) {
    // Haystack is too small; fast early return
    return -1;
  }
  word end_index = haystack.offsetByCodePoints(start_index, end - start);
  if ((end_index - start_index) < needle.charLength() ||
      start_index > end_index) {
    // Haystack is too small; fast early return
    return -1;
  }
  // Loop is in byte space, not code point space
  // Invariant: cp_offset and byte_offset describe the same offset into the
  // string, but one is in code point space and the other is in byte space
  // TODO(T41400083): Use a different search algorithm
  for (word cp_offset = end - 1,
            byte_offset = haystack.offsetByCodePoints(end_index, -1);
       byte_offset >= 0; cp_offset--,
            byte_offset = haystack.offsetByCodePoints(byte_offset, -1)) {
    if (strHasPrefix(haystack, needle, byte_offset)) {
      return cp_offset;
    }
  }
  return -1;
}

RawObject METH(str, __le__)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Object other_obj(&scope, args.get(1));
  if (!thread->runtime()->isInstanceOfStr(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(str));
  }
  if (!thread->runtime()->isInstanceOfStr(*other_obj)) {
    return NotImplementedType::object();
  }
  Str self(&scope, strUnderlying(*self_obj));
  Str other(&scope, strUnderlying(*other_obj));
  return Bool::fromBool(self.compare(*other) <= 0);
}

RawObject METH(str, __len__)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfStr(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(str));
  }
  Str self(&scope, strUnderlying(*self_obj));
  return SmallInt::fromWord(self.codePointLength());
}

RawObject METH(str, lower)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfStr(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(str));
  }
  Str self(&scope, strUnderlying(*self_obj));
  word length = self.charLength();

  // Search for the first uppercase character.
  word i = 0;
  for (; i < length; i++) {
    byte c = self.charAt(i);
    if (c > kMaxASCII) {
      // TODO(T57791326) support non-ASCII
      UNIMPLEMENTED("non-ASCII characters");
    }
    if (c >= 'A' && c <= 'Z') {
      break;
    }
  }
  if (i >= length && self_obj.isStr()) {
    return *self_obj;
  }

  // Create new string object and translate remaining characters.
  MutableBytes result(&scope, runtime->newMutableBytesUninitialized(length));
  result.replaceFromWithStr(0, *self, i);
  for (; i < length; i++) {
    byte c = self.charAt(i);
    if (c > kMaxASCII) {
      // TODO(T57791326) support non-ASCII
      UNIMPLEMENTED("non-ASCII characters");
    }
    if (c >= 'A' && c <= 'Z') {
      c -= 'A' - 'a';
    }
    result.byteAtPut(i, c);
  }
  return result.becomeStr();
}

RawObject METH(str, title)(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfStr(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(str));
  }
  Str self(&scope, strUnderlying(*self_obj));
  word length = self.charLength();
  bool is_previous_mapped = false;
  MutableBytes result(&scope, runtime->newMutableBytesUninitialized(length));
  for (word i = 0; i < length; i++) {
    byte ch = self.charAt(i);
    byte mapped;
    if (is_previous_mapped) {
      mapped = Unicode::toLower(ch);
    } else {
      mapped = Unicode::toTitle(ch);
    }
    // TODO(...): use the Unicode database case mapping
    is_previous_mapped = Unicode::isAlpha(ch);
    result.byteAtPut(i, mapped);
  }
  return result.becomeStr();
}

RawObject METH(str, upper)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfStr(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(str));
  }
  Str self(&scope, strUnderlying(*self_obj));
  word length = self.charLength();

  // Search for the first lowercase character.
  word i = 0;
  for (; i < length; i++) {
    byte c = self.charAt(i);
    if (c > kMaxASCII) {
      // TODO(T57791406) support non-ASCII
      UNIMPLEMENTED("non-ASCII characters");
    }
    if (c >= 'a' && c <= 'z') {
      break;
    }
  }
  if (i >= length && self_obj.isStr()) {
    return *self_obj;
  }

  // Create new string object and translate remaining characters.
  MutableBytes result(&scope, runtime->newMutableBytesUninitialized(length));
  result.replaceFromWithStr(0, *self, i);
  for (; i < length; i++) {
    byte c = self.charAt(i);
    if (c > kMaxASCII) {
      // TODO(T57791406) support non-ASCII
      UNIMPLEMENTED("non-ASCII characters");
    }
    if (c >= 'a' && c <= 'z') {
      c -= 'a' - 'A';
    }
    result.byteAtPut(i, c);
  }
  return result.becomeStr();
}

RawObject METH(str, __lt__)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Object other_obj(&scope, args.get(1));
  if (!thread->runtime()->isInstanceOfStr(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(str));
  }
  if (!thread->runtime()->isInstanceOfStr(*other_obj)) {
    return NotImplementedType::object();
  }
  Str self(&scope, strUnderlying(*self_obj));
  Str other(&scope, strUnderlying(*other_obj));
  return Bool::fromBool(self.compare(*other) < 0);
}

RawObject METH(str, __mul__)(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfStr(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(str));
  }
  Object count_index(&scope, args.get(1));
  Object count_obj(&scope, intFromIndex(thread, count_index));
  if (count_obj.isError()) return *count_obj;
  word count = intUnderlying(*count_obj).asWordSaturated();
  if (!SmallInt::isValid(count)) {
    return thread->raiseWithFmt(LayoutId::kOverflowError,
                                "cannot fit '%T' into an index-sized integer",
                                &count_obj);
  }
  Str self(&scope, *self_obj);
  word length = self.charLength();
  if (count <= 0 || length == 0) {
    return Str::empty();
  }
  word new_length;
  if (__builtin_mul_overflow(length, count, &new_length) ||
      !SmallInt::isValid(new_length)) {
    return thread->raiseWithFmt(LayoutId::kOverflowError,
                                "repeated string is too long");
  }
  return runtime->strRepeat(thread, self, count);
}

RawObject METH(str, __ne__)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Object other_obj(&scope, args.get(1));
  if (!thread->runtime()->isInstanceOfStr(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(str));
  }
  if (!thread->runtime()->isInstanceOfStr(*other_obj)) {
    return NotImplementedType::object();
  }
  Str self(&scope, strUnderlying(*self_obj));
  Str other(&scope, strUnderlying(*other_obj));
  return Bool::fromBool(!self.equals(*other));
}

RawObject METH(str, __iter__)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfStr(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(str));
  }
  Str self(&scope, strUnderlying(*self_obj));
  return runtime->newStrIterator(self);
}

// Convert a byte to its hex digits, and write them out to buf.
// Increments buf to point after the written characters.
static void byteToHex(const MutableBytes& buf, word index, byte convert) {
  static byte hexdigits[] = {'0', '1', '2', '3', '4', '5', '6', '7',
                             '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
  // Since convert is unsigned, the right shift will not propagate the sign
  // bit, and the upper bits will be zero.
  buf.byteAtPut(index, hexdigits[convert >> 4]);
  buf.byteAtPut(index + 1, hexdigits[convert & 0x0f]);
}

RawObject METH(str, __repr__)(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfStr(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(str));
  }
  Str self(&scope, strUnderlying(*self_obj));
  const word self_len = self.charLength();
  word result_len = 0;
  word squote = 0;
  word dquote = 0;
  // Precompute the size so that only one allocation is necessary.
  for (word i = 0, char_len; i < self_len; i += char_len) {
    int32_t code_point = self.codePointAt(i, &char_len);
    if (code_point == '\'') {
      squote++;
      result_len += 1;
    } else if (code_point == '"') {
      dquote++;
      result_len += 1;
    } else if (code_point == '\\' || code_point == '\t' || code_point == '\r' ||
               code_point == '\n') {
      result_len += 2;
    } else if (Unicode::isPrintable(code_point)) {
      result_len += char_len;
    } else if (code_point < 0x100) {
      result_len += 4;
    } else if (code_point < 0x10000) {
      result_len += 6;
    } else {
      result_len += 10;
    }
  }

  byte quote = '\'';
  bool unchanged = (result_len == self_len);
  if (squote > 0) {
    unchanged = false;
    // If there are both single quotes and double quotes, the outer quote will
    // be singles, and all internal quotes will need to be escaped.
    if (dquote > 0) {
      // Add the size of the escape backslashes on the single quotes.
      result_len += squote;
    } else {
      quote = '"';
    }
  }
  result_len += 2;  // quotes

  MutableBytes buf(&scope, runtime->newMutableBytesUninitialized(result_len));
  buf.byteAtPut(0, quote);
  buf.byteAtPut(result_len - 1, quote);
  if (unchanged) {
    // Remaining characters were unmodified, so copy them directly.
    buf.replaceFromWithStr(1, *self, self_len);
    return buf.becomeStr();
  }
  word out = 1;
  for (word in = 0, char_len; in < self_len; in += char_len) {
    int32_t code_point = self.codePointAt(in, &char_len);
    if (code_point == quote) {
      buf.byteAtPut(out++, '\\');
      buf.byteAtPut(out++, quote);
    } else if (code_point == '\\') {
      buf.byteAtPut(out++, '\\');
      buf.byteAtPut(out++, '\\');
    } else if (code_point == '\t') {
      buf.byteAtPut(out++, '\\');
      buf.byteAtPut(out++, 't');
    } else if (code_point == '\r') {
      buf.byteAtPut(out++, '\\');
      buf.byteAtPut(out++, 'r');
    } else if (code_point == '\n') {
      buf.byteAtPut(out++, '\\');
      buf.byteAtPut(out++, 'n');
    } else if (' ' <= code_point && code_point < kMaxASCII) {
      buf.byteAtPut(out++, code_point);
    } else if (code_point <= kMaxASCII) {
      buf.byteAtPut(out++, '\\');
      buf.byteAtPut(out++, 'x');
      byteToHex(buf, out, code_point);
      out += 2;
    } else if (Unicode::isPrintable(code_point)) {
      for (word i = 0; i < char_len; i++) {
        buf.byteAtPut(out + i, self.charAt(in + i));
      }
      out += char_len;
    } else if (code_point <= 0xff) {
      buf.byteAtPut(out++, '\\');
      buf.byteAtPut(out++, 'x');
      byteToHex(buf, out, code_point);
      out += 2;
    } else if (code_point <= 0xffff) {
      buf.byteAtPut(out++, '\\');
      buf.byteAtPut(out++, 'u');
      byteToHex(buf, out, code_point);
      byteToHex(buf, out + 2, code_point >> kBitsPerByte);
      out += 4;
    } else {
      buf.byteAtPut(out++, '\\');
      buf.byteAtPut(out++, 'U');
      for (word i = 0; i < 4; i++, out += 2) {
        byteToHex(buf, out, code_point >> (i * kBitsPerByte));
      }
    }
  }
  DCHECK(out == result_len - 1, "wrote %ld characters, expected %ld", out - 1,
         result_len - 2);
  return buf.becomeStr();
}

RawObject METH(str, isalnum)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfStr(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(str));
  }
  Str self(&scope, strUnderlying(*self_obj));
  word char_length = self.charLength();
  if (char_length == 0) {
    return Bool::falseObj();
  }
  word i = 0;
  do {
    byte b = self.charAt(i++);
    if (b > kMaxASCII) {
      UNIMPLEMENTED("non-ASCII character");
    }
    if (!ASCII::isAlnum(b)) {
      return Bool::falseObj();
    }
  } while (i < char_length);
  return Bool::trueObj();
}

RawObject METH(str, isalpha)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfStr(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(str));
  }
  Str self(&scope, strUnderlying(*self_obj));
  word char_length = self.charLength();
  if (char_length == 0) {
    return Bool::falseObj();
  }
  word i = 0;
  do {
    byte b = self.charAt(i++);
    if (b > kMaxASCII) {
      UNIMPLEMENTED("non-ASCII character");
    }
    if (!ASCII::isAlpha(b)) {
      return Bool::falseObj();
    }
  } while (i < char_length);
  return Bool::trueObj();
}

RawObject METH(str, isascii)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfStr(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(str));
  }
  Str self(&scope, strUnderlying(*self_obj));
  return Bool::fromBool(self.isASCII());
}

RawObject METH(str, isdecimal)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfStr(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(str));
  }
  Str self(&scope, strUnderlying(*self_obj));
  word char_length = self.charLength();
  if (char_length == 0) {
    return Bool::falseObj();
  }
  word i = 0;
  do {
    byte b = self.charAt(i++);
    if (b > kMaxASCII) {
      UNIMPLEMENTED("non-ASCII character");
    }
    if (!ASCII::isDecimal(b)) {
      return Bool::falseObj();
    }
  } while (i < char_length);
  return Bool::trueObj();
}

RawObject METH(str, isdigit)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfStr(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(str));
  }
  Str self(&scope, strUnderlying(*self_obj));
  word char_length = self.charLength();
  if (char_length == 0) {
    return Bool::falseObj();
  }
  word i = 0;
  do {
    byte b = self.charAt(i++);
    if (b > kMaxASCII) {
      UNIMPLEMENTED("non-ASCII character");
    }
    if (!ASCII::isDigit(b)) {
      return Bool::falseObj();
    }
  } while (i < char_length);
  return Bool::trueObj();
}

RawObject METH(str, isidentifier)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfStr(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(str));
  }
  Str self(&scope, strUnderlying(*self_obj));
  word char_length = self.charLength();
  if (char_length == 0) {
    return Bool::falseObj();
  }
  word len;
  int32_t first = self.codePointAt(0, &len);
  if (!Unicode::isXidStart(first) && first != '_') {
    return Bool::falseObj();
  }
  for (word i = len; i < char_length; i += len) {
    int32_t code_point = self.codePointAt(i, &len);
    if (!Unicode::isXidContinue(code_point)) {
      return Bool::falseObj();
    }
  }
  return Bool::trueObj();
}

RawObject METH(str, islower)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfStr(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(str));
  }
  Str self(&scope, strUnderlying(*self_obj));
  word char_length = self.charLength();
  bool cased = false;
  for (word i = 0, len; i < char_length; i += len) {
    int32_t code_point = self.codePointAt(i, &len);
    if (Unicode::isUpper(code_point) || Unicode::isTitle(code_point)) {
      return Bool::falseObj();
    }
    if (!cased && Unicode::isLower(code_point)) {
      cased = true;
    }
  }
  return Bool::fromBool(cased);
}

RawObject METH(str, isnumeric)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfStr(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(str));
  }
  Str self(&scope, strUnderlying(*self_obj));
  word char_length = self.charLength();
  if (char_length == 0) {
    return Bool::falseObj();
  }
  word i = 0;
  do {
    byte b = self.charAt(i++);
    if (b > kMaxASCII) {
      UNIMPLEMENTED("non-ASCII character");
    }
    if (!ASCII::isNumeric(b)) {
      return Bool::falseObj();
    }
  } while (i < char_length);
  return Bool::trueObj();
}

RawObject METH(str, isprintable)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfStr(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(str));
  }
  Str self(&scope, strUnderlying(*self_obj));
  for (word i = 0, char_length = self.charLength(); i < char_length; i++) {
    byte b = self.charAt(i);
    if (b > kMaxASCII) {
      UNIMPLEMENTED("non-ASCII character");
    }
    if (!ASCII::isPrintable(b)) {
      return Bool::falseObj();
    }
  }
  return Bool::trueObj();
}

RawObject METH(str, isspace)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfStr(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(str));
  }
  Str self(&scope, strUnderlying(*self_obj));
  word char_length = self.charLength();
  if (char_length == 0) {
    return Bool::falseObj();
  }
  if (char_length == 1) {
    return ASCII::isSpace(self.charAt(0)) ? Bool::trueObj() : Bool::falseObj();
  }
  word byte_index = 0;
  do {
    word num_bytes;
    int32_t codepoint = self.codePointAt(byte_index, &num_bytes);
    if (!Unicode::isSpace(codepoint)) {
      return Bool::falseObj();
    }
    byte_index += num_bytes;
  } while (byte_index < char_length);
  return Bool::trueObj();
}

RawObject METH(str, istitle)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfStr(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(str));
  }
  Str self(&scope, strUnderlying(*self_obj));
  bool cased = false;
  bool previous_is_cased = false;
  for (word i = 0, char_length = self.charLength(); i < char_length; i++) {
    byte b = self.charAt(i);
    if (ASCII::isUpper(b)) {
      if (previous_is_cased) return Bool::falseObj();
      previous_is_cased = true;
      cased = true;
    } else if (ASCII::isLower(b)) {
      if (!previous_is_cased) return Bool::falseObj();
      previous_is_cased = true;
      cased = true;
    } else {
      previous_is_cased = false;
    }
  }
  return Bool::fromBool(cased);
}

RawObject METH(str, isupper)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfStr(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(str));
  }
  Str self(&scope, strUnderlying(*self_obj));
  word char_length = self.charLength();
  bool cased = false;
  for (word i = 0; i < char_length; i++) {
    byte b = self.charAt(i);
    if (b > kMaxASCII) {
      UNIMPLEMENTED("non-ASCII character");
    }
    if (ASCII::isLower(b)) {
      return Bool::falseObj();
    }
    if (!cased && ASCII::isUpper(b)) {
      cased = true;
    }
  }
  return Bool::fromBool(cased);
}

RawObject METH(str, lstrip)(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfStr(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(str));
  }
  Str str(&scope, strUnderlying(*self_obj));
  Object other_obj(&scope, args.get(1));
  if (other_obj.isNoneType()) {
    return strStripSpaceLeft(thread, str);
  }
  if (!runtime->isInstanceOfStr(*other_obj)) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "str.lstrip() arg must be None or str");
  }
  Str chars(&scope, strUnderlying(*other_obj));
  return strStripLeft(thread, str, chars);
}

RawObject METH(str, rstrip)(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfStr(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(str));
  }
  Str str(&scope, strUnderlying(*self_obj));
  Object other_obj(&scope, args.get(1));
  if (other_obj.isNoneType()) {
    return strStripSpaceRight(thread, str);
  }
  if (!runtime->isInstanceOfStr(*other_obj)) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "str.rstrip() arg must be None or str");
  }
  Str chars(&scope, strUnderlying(*other_obj));
  return strStripRight(thread, str, chars);
}

RawObject METH(str, strip)(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfStr(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(str));
  }
  Str str(&scope, strUnderlying(*self_obj));
  Object other_obj(&scope, args.get(1));
  if (other_obj.isNoneType()) {
    return strStripSpace(thread, str);
  }
  if (!runtime->isInstanceOfStr(*other_obj)) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "str.strip() arg must be None or str");
  }
  Str chars(&scope, strUnderlying(*other_obj));
  return strStrip(thread, str, chars);
}

RawObject METH(str_iterator, __iter__)(Thread* thread, Frame* frame,
                                       word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isStrIterator()) {
    return thread->raiseRequiresType(self, ID(str_iterator));
  }
  return *self;
}

// TODO(T35578204) Implement this for UTF-8. This probably means keeping extra
// state and logic so that __next__() will advance to the next codepoint.

RawObject METH(str_iterator, __next__)(Thread* thread, Frame* frame,
                                       word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isStrIterator()) {
    return thread->raiseRequiresType(self, ID(str_iterator));
  }
  StrIterator iter(&scope, *self);
  Object value(&scope, strIteratorNext(thread, iter));
  if (value.isError()) {
    return thread->raise(LayoutId::kStopIteration, NoneType::object());
  }
  return *value;
}

RawObject METH(str_iterator, __length_hint__)(Thread* thread, Frame* frame,
                                              word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isStrIterator()) {
    return thread->raiseRequiresType(self, ID(str_iterator));
  }
  StrIterator str_iterator(&scope, *self);
  Str str(&scope, str_iterator.iterable());
  return SmallInt::fromWord(str.charLength() - str_iterator.index());
}

}  // namespace py
