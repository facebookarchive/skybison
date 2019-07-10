#include "str-builtins.h"

#include "frame.h"
#include "globals.h"
#include "int-builtins.h"
#include "list-builtins.h"
#include "objects.h"
#include "runtime.h"
#include "slice-builtins.h"
#include "thread.h"
#include "tuple-builtins.h"

namespace python {

// TODO(T39861344): This can be replaced by a real string codec.
RawObject strEscapeNonASCII(Thread* thread, const Object& str_obj) {
  CHECK(str_obj.isStr(), "strEscapeNonASCII cannot currently handle non-str");
  HandleScope scope(thread);
  Str str(&scope, *str_obj);
  for (word i = 0; i < str.length(); i++) {
    if (str.charAt(i) > kMaxASCII) {
      UNIMPLEMENTED(
          "Character '%d' at index %ld is not yet escapable in strEscape",
          str.charAt(i), i);
    }
  }
  return *str;
}

word strSpan(const Str& src, const Str& str) {
  word length = src.length();
  word str_length = str.length();
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
  word length = src.length();
  word str_length = str.length();
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

RawObject strSplitlines(Thread* thread, const Str& str, bool keepends) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  List result(&scope, runtime->newList());
  // Looping over code points, not bytes, but i is a byte offset
  for (word i = 0, j = 0; i < str.length(); j = i) {
    // Skip newline chars
    word num_bytes;
    while (i < str.length() && !isLineBreak(str.codePointAt(i, &num_bytes))) {
      i += num_bytes;
    }

    word eol_pos = i;
    if (i < str.length()) {
      int32_t cp = str.codePointAt(i, &num_bytes);
      word next = i + num_bytes;
      word next_num_bytes;
      // Check for \r\n specifically
      if (cp == '\r' && next < str.length() &&
          str.codePointAt(next, &next_num_bytes) == '\n') {
        i += next_num_bytes;
      }
      i += num_bytes;
      if (keepends) {
        eol_pos = i;
      }
    }

    // If there are no newlines, the str returned should be identity-equal
    if (j == 0 && eol_pos == str.length() && str.isStr()) {
      runtime->listAdd(thread, result, str);
      return *result;
    }

    Str substr(&scope, runtime->strSubstr(thread, str, j, eol_pos - j));
    runtime->listAdd(thread, result, substr);
  }

  return *result;
}

// TODO(T43723300): Handle unicode
static bool isAsciiSpace(byte ch) {
  return ((ch >= 0x09) && (ch <= 0x0D)) || ((ch >= 0x1C) && (ch <= 0x1F)) ||
         ch == 0x20;
}

RawObject strStripSpace(Thread* thread, const Str& src) {
  word length = src.length();
  if (length == 0) {
    return *src;
  }
  if (length == 1 && isAsciiSpace(src.charAt(0))) {
    return Str::empty();
  }
  word first = 0;
  while (first < length && isAsciiSpace(src.charAt(first))) {
    ++first;
  }
  word last = 0;
  for (word i = length - 1; i >= first && isAsciiSpace(src.charAt(i)); i--) {
    last++;
  }
  return thread->runtime()->strSubstr(thread, src, first,
                                      length - first - last);
}

RawObject strStripSpaceLeft(Thread* thread, const Str& src) {
  word length = src.length();
  if (length == 0) {
    return *src;
  }
  if (length == 1 && isAsciiSpace(src.charAt(0))) {
    return Str::empty();
  }
  word first = 0;
  while (first < length && isAsciiSpace(src.charAt(first))) {
    ++first;
  }
  return thread->runtime()->strSubstr(thread, src, first, length - first);
}

RawObject strStripSpaceRight(Thread* thread, const Str& src) {
  word length = src.length();
  if (length == 0) {
    return *src;
  }
  if (length == 1 && isAsciiSpace(src.charAt(0))) {
    return Str::empty();
  }
  word last = 0;
  for (word i = length - 1; i >= 0 && isAsciiSpace(src.charAt(i)); i--) {
    last++;
  }
  return thread->runtime()->strSubstr(thread, src, 0, length - last);
}

RawObject strStrip(Thread* thread, const Str& src, const Str& str) {
  word length = src.length();
  if (length == 0 || str.length() == 0) {
    return *src;
  }
  word first = strSpan(src, str);
  word last = strRSpan(src, str, first);
  return thread->runtime()->strSubstr(thread, src, first,
                                      length - first - last);
}

RawObject strStripLeft(Thread* thread, const Str& src, const Str& str) {
  word length = src.length();
  if (length == 0 || str.length() == 0) {
    return *src;
  }
  word first = strSpan(src, str);
  return thread->runtime()->strSubstr(thread, src, first, length - first);
}

RawObject strStripRight(Thread* thread, const Str& src, const Str& str) {
  word length = src.length();
  if (length == 0 || str.length() == 0) {
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
  if (byte_offset >= underlying.length()) {
    return Error::noMoreItems();
  }
  word num_bytes = 0;
  word code_point = underlying.codePointAt(byte_offset, &num_bytes);
  iter.setIndex(byte_offset + num_bytes);
  return RawSmallStr::fromCodePoint(code_point);
}

RawObject strUnderlying(Thread* thread, const Object& obj) {
  if (obj.isStr()) return *obj;
  DCHECK(thread->runtime()->isInstanceOfStr(*obj),
         "cannot get a base str value from a non-str");
  HandleScope scope(thread);
  UserStrBase user_str(&scope, *obj);
  return user_str.value();
}

void SmallStrBuiltins::postInitialize(Runtime* runtime, const Type& new_type) {
  new_type.setBuiltinBase(kSuperType);
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
    {SymbolId::kInvalid, UserStrBase::kValueOffset},
    {SymbolId::kSentinelId, 0},
};

const BuiltinMethod StrBuiltins::kBuiltinMethods[] = {
    {SymbolId::kDunderAdd, dunderAdd},
    {SymbolId::kDunderBool, dunderBool},
    {SymbolId::kDunderEq, dunderEq},
    {SymbolId::kDunderGe, dunderGe},
    {SymbolId::kDunderGetitem, dunderGetItem},
    {SymbolId::kDunderGt, dunderGt},
    {SymbolId::kDunderHash, dunderHash},
    {SymbolId::kDunderIter, dunderIter},
    {SymbolId::kDunderLe, dunderLe},
    {SymbolId::kDunderLen, dunderLen},
    {SymbolId::kDunderLt, dunderLt},
    {SymbolId::kDunderMod, dunderMod},
    {SymbolId::kDunderMul, dunderMul},
    {SymbolId::kDunderNe, dunderNe},
    {SymbolId::kDunderRepr, dunderRepr},
    {SymbolId::kLower, lower},
    {SymbolId::kLStrip, lstrip},
    {SymbolId::kRStrip, rstrip},
    {SymbolId::kStrip, strip},
    {SymbolId::kUpper, upper},
    {SymbolId::kSentinelId, nullptr},
};

void StrBuiltins::postInitialize(Runtime*, const Type& new_type) {
  new_type.setBuiltinBase(LayoutId::kStr);
}

RawObject StrBuiltins::dunderAdd(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  Object other_obj(&scope, args.get(1));
  if (!runtime->isInstanceOfStr(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kStr);
  }
  if (!runtime->isInstanceOfStr(*other_obj)) {
    return thread->raiseRequiresType(other_obj, SymbolId::kStr);
  }
  Str self(&scope, strUnderlying(thread, self_obj));
  Str other(&scope, strUnderlying(thread, other_obj));
  return runtime->strConcat(thread, self, other);
}

RawObject StrBuiltins::dunderBool(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfStr(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kStr);
  }
  Str self(&scope, strUnderlying(thread, self_obj));
  return Bool::fromBool(*self != Str::empty());
}

RawObject StrBuiltins::dunderEq(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Object other_obj(&scope, args.get(1));
  if (!thread->runtime()->isInstanceOfStr(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kStr);
  }
  if (!thread->runtime()->isInstanceOfStr(*other_obj)) {
    return NotImplementedType::object();
  }
  Str self(&scope, strUnderlying(thread, self_obj));
  Str other(&scope, strUnderlying(thread, other_obj));
  return Bool::fromBool(self.compare(*other) == 0);
}

RawObject StrBuiltins::dunderGe(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Object other_obj(&scope, args.get(1));
  if (!thread->runtime()->isInstanceOfStr(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kStr);
  }
  if (!thread->runtime()->isInstanceOfStr(*other_obj)) {
    return NotImplementedType::object();
  }
  Str self(&scope, strUnderlying(thread, self_obj));
  Str other(&scope, strUnderlying(thread, other_obj));
  return Bool::fromBool(self.compare(*other) >= 0);
}

RawObject StrBuiltins::dunderGt(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Object other_obj(&scope, args.get(1));
  if (!thread->runtime()->isInstanceOfStr(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kStr);
  }
  if (!thread->runtime()->isInstanceOfStr(*other_obj)) {
    return NotImplementedType::object();
  }
  Str self(&scope, strUnderlying(thread, self_obj));
  Str other(&scope, strUnderlying(thread, other_obj));
  return Bool::fromBool(self.compare(*other) > 0);
}

RawObject StrBuiltins::dunderHash(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfStr(*self)) {
    return thread->raiseRequiresType(self, SymbolId::kStr);
  }
  return runtime->hash(*self);
}

static void adjustIndices(const Str& str, word* startp, word* endp) {
  DCHECK(*startp < 0 || *endp < 0,
         "adjustIndices should not be called unless start or end is < 0");
  word str_len = str.codePointLength();
  word start = *startp;
  word end = *endp;
  if (end < 0) {
    end += str_len;
    if (end < 0) {
      end = 0;
    }
    *endp = end;
  }
  if (start < 0) {
    start += str_len;
    if (start < 0) {
      start = 0;
    }
    *startp = start;
  }
}

void strInternInTuple(Thread* thread, const Object& items) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  DCHECK(runtime->isInstanceOfTuple(*items), "items must be a tuple instance");
  Tuple tuple(&scope, tupleUnderlying(thread, items));
  Object obj(&scope, NoneType::object());
  Object result(&scope, NoneType::object());
  for (word i = 0; i < tuple.length(); i++) {
    obj = tuple.at(i);
    CHECK(obj.isStr(), "non-string found in code slot");
    result = runtime->internStr(thread, obj);
    if (result.isError()) continue;
    if (result != obj) {
      tuple.atPut(i, *result);
    }
  }
}

static bool allNameChars(const Str& str) {
  for (word i = 0; i < str.length(); i++) {
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
  Tuple tuple(&scope, tupleUnderlying(thread, items));
  Object obj(&scope, NoneType::object());
  Object result(&scope, NoneType::object());
  bool modified = false;
  for (word i = 0; i < tuple.length(); i++) {
    obj = tuple.at(i);

    if (obj.isStr()) {
      Str str(&scope, *obj);
      if (allNameChars(str)) {
        // if all name chars, intern in place
        result = runtime->internStr(thread, obj);
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
        seq.atPut(j, Set::Bucket::key(*data, idx));
      }
      if (strInternConstants(thread, seq)) {
        obj = runtime->setUpdate(thread, set, seq);
        if (obj.isError()) continue;
        tuple.atPut(i, *obj);
        modified = true;
      }
    }
  }
  return modified;
}

bool strIsASCII(const Str& str) {
  // TODO(T45148575): Check multiple bytes at once
  for (word i = 0; i < str.length(); i++) {
    if (str.charAt(i) > kMaxASCII) {
      return false;
    }
  }
  return true;
}

RawObject strFind(const Str& haystack, const Str& needle, word start,
                  word end) {
  if (end < 0 || start < 0) {
    adjustIndices(haystack, &start, &end);
  }

  word start_index = haystack.offsetByCodePoints(0, start);
  if (start_index == haystack.length() && needle.length() > 0) {
    // Haystack is too small; fast early return
    return SmallInt::fromWord(-1);
  }
  word end_index = haystack.offsetByCodePoints(start_index, end - start);

  if ((end_index - start_index) < needle.length() || start_index > end_index) {
    // Haystack is too small; fast early return
    return SmallInt::fromWord(-1);
  }

  // Loop is in byte space, not code point space
  word result = start;
  bool has_match = false;
  // TODO(T41400083): Use a different search algorithm
  for (word i = start_index; i <= end_index - needle.length(); result++) {
    has_match = true;
    for (word j = 0; has_match && j < needle.length(); j++) {
      if (haystack.charAt(i + j) != needle.charAt(j)) {
        has_match = false;
      }
    }
    word next = haystack.offsetByCodePoints(i, 1);
    if (i == next) {
      // We've reached a fixpoint; offsetByCodePoints will not advance past the
      // length of the string.
      if (start_index >= i) {
        // The start is greater than the length of the string.
        return SmallInt::fromWord(-1);
      }
      // If the start is within bounds, just return the last found index.
      break;
    }
    if (has_match) {
      return SmallInt::fromWord(result);
    }
    i = next;
  }
  return SmallInt::fromWord(-1);
}

word strFindFirstNonWhitespace(const Str& str) {
  word i = 0;
  while (i < str.length() && isAsciiSpace(str.charAt(i))) {
    i++;
  }
  return i;
}

RawObject strRFind(const Str& haystack, const Str& needle, word start,
                   word end) {
  if (end < 0 || start < 0) {
    adjustIndices(haystack, &start, &end);
  }

  word start_index = haystack.offsetByCodePoints(0, start);
  if (start_index == haystack.length() && needle.length() > 0) {
    // Haystack is too small; fast early return
    return SmallInt::fromWord(-1);
  }
  word end_index = haystack.offsetByCodePoints(start_index, end - start);

  if ((end_index - start_index) < needle.length() || start_index > end_index) {
    // Haystack is too small; fast early return
    return SmallInt::fromWord(-1);
  }

  // Loop is in byte space, not code point space
  word result = start;
  word last_index = -1;
  bool has_match = false;
  // TODO(T41400083): Use a different search algorithm
  for (word i = start_index; i <= end_index - needle.length(); result++) {
    has_match = true;
    for (word j = 0; has_match && j < needle.length(); j++) {
      if (haystack.charAt(i + j) != needle.charAt(j)) {
        has_match = false;
      }
    }
    if (has_match) {
      last_index = result;
    }
    word next = haystack.offsetByCodePoints(i, 1);
    if (i == next) {
      // We've reached a fixpoint; offsetByCodePoints will not advance past the
      // length of the string.
      if (start_index >= i) {
        // The start is greater than the length of the string.
        return SmallInt::fromWord(-1);
      }
      // If the start is within bounds, just return the last found index.
      break;
    }
    i = next;
  }
  return SmallInt::fromWord(last_index);
}

RawObject StrBuiltins::dunderLe(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Object other_obj(&scope, args.get(1));
  if (!thread->runtime()->isInstanceOfStr(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kStr);
  }
  if (!thread->runtime()->isInstanceOfStr(*other_obj)) {
    return NotImplementedType::object();
  }
  Str self(&scope, strUnderlying(thread, self_obj));
  Str other(&scope, strUnderlying(thread, other_obj));
  return Bool::fromBool(self.compare(*other) <= 0);
}

RawObject StrBuiltins::dunderLen(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfStr(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kStr);
  }
  Str self(&scope, strUnderlying(thread, self_obj));
  return SmallInt::fromWord(self.codePointLength());
}

RawObject StrBuiltins::lower(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfStr(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kStr);
  }
  Str self(&scope, strUnderlying(thread, self_obj));
  std::unique_ptr<byte[]> buf(new byte[self.length()]);
  byte* bufp = buf.get();
  for (word i = 0; i < self.length(); i++) {
    byte c = self.charAt(i);
    if (c > kMaxASCII) {
      UNIMPLEMENTED("Lowercase non-ASCII characters");
    }
    if (c >= 'A' && c <= 'Z') {
      bufp[i] = c - 'A' + 'a';
    } else {
      bufp[i] = c;
    }
  }
  Str result(&scope, runtime->newStrWithAll(View<byte>{bufp, self.length()}));
  return *result;
}

RawObject StrBuiltins::upper(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfStr(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kStr);
  }
  Str self(&scope, strUnderlying(thread, self_obj));
  std::unique_ptr<byte[]> buf(new byte[self.length()]);
  byte* bufp = buf.get();
  for (word i = 0; i < self.length(); i++) {
    byte c = self.charAt(i);
    if (c > kMaxASCII) {
      UNIMPLEMENTED("Uppercase non-ASCII characters");
    }
    if (c >= 'a' && c <= 'z') {
      bufp[i] = c - 'a' + 'A';
    } else {
      bufp[i] = c;
    }
  }
  Str result(&scope, runtime->newStrWithAll(View<byte>{bufp, self.length()}));
  return *result;
}

RawObject StrBuiltins::dunderLt(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Object other_obj(&scope, args.get(1));
  if (!thread->runtime()->isInstanceOfStr(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kStr);
  }
  if (!thread->runtime()->isInstanceOfStr(*other_obj)) {
    return NotImplementedType::object();
  }
  Str self(&scope, strUnderlying(thread, self_obj));
  Str other(&scope, strUnderlying(thread, other_obj));
  return Bool::fromBool(self.compare(*other) < 0);
}

RawObject StrBuiltins::strFormatBufferLength(Thread* thread, const Str& fmt,
                                             const Tuple& args) {
  word arg_idx = 0;
  word len = 0;
  for (word fmt_idx = 0; fmt_idx < fmt.length(); fmt_idx++, len++) {
    byte ch = fmt.charAt(fmt_idx);
    if (ch != '%') {
      continue;
    }
    if (++fmt_idx >= fmt.length()) {
      return thread->raiseWithFmt(LayoutId::kValueError, "Incomplete format");
    }
    switch (fmt.charAt(fmt_idx)) {
      case 'd': {
        len--;
        if (!args.at(arg_idx).isInt()) {
          return thread->raiseWithFmt(LayoutId::kTypeError,
                                      "Argument mismatch");
        }
        len +=
            snprintf(nullptr, 0, "%ld", Int::cast(args.at(arg_idx)).asWord());
        arg_idx++;
      } break;
      case 'g': {
        len--;
        if (!args.at(arg_idx).isFloat()) {
          return thread->raiseWithFmt(LayoutId::kTypeError,
                                      "Argument mismatch");
        }
        len +=
            snprintf(nullptr, 0, "%g", Float::cast(args.at(arg_idx)).value());
        arg_idx++;
      } break;
      case 's': {
        len--;
        if (!args.at(arg_idx).isStr()) {
          return thread->raiseWithFmt(LayoutId::kTypeError,
                                      "Argument mismatch");
        }
        len += Str::cast(args.at(arg_idx)).length();
        arg_idx++;
      } break;
      case '%':
        break;
      default:
        UNIMPLEMENTED("Unsupported format specifier");
    }
  }

  if (!SmallInt::isValid(len)) {
    return thread->raiseWithFmt(LayoutId::kOverflowError,
                                "Output of format string is too long");
  }
  return SmallInt::fromWord(len);
}

static void stringFormatToBuffer(const Str& fmt, const Tuple& args, char* dst,
                                 word len) {
  word arg_idx = 0;
  word dst_idx = 0;
  for (word fmt_idx = 0; fmt_idx < fmt.length(); fmt_idx++) {
    byte ch = fmt.charAt(fmt_idx);
    if (ch != '%') {
      dst[dst_idx++] = ch;
      continue;
    }
    switch (fmt.charAt(++fmt_idx)) {
      case 'd': {
        word value = Int::cast(args.at(arg_idx++)).asWord();
        dst_idx += snprintf(&dst[dst_idx], len - dst_idx + 1, "%ld", value);
      } break;
      case 'g': {
        double value = Float::cast(args.at(arg_idx++)).value();
        dst_idx += snprintf(&dst[dst_idx], len - dst_idx + 1, "%g", value);
      } break;
      case 's': {
        RawStr value = Str::cast(args.at(arg_idx));
        value.copyTo(reinterpret_cast<byte*>(&dst[dst_idx]), value.length());
        dst_idx += value.length();
        arg_idx++;
      } break;
      case '%':
        dst[dst_idx++] = '%';
        break;
      default:
        UNIMPLEMENTED("Unsupported format specifier");
    }
  }
  dst[len] = '\0';
}

RawObject StrBuiltins::strFormat(Thread* thread, const Str& fmt,
                                 const Tuple& args) {
  if (fmt.length() == 0) {
    return *fmt;
  }
  RawObject raw_len = strFormatBufferLength(thread, fmt, args);
  if (raw_len.isError()) return raw_len;
  word len = SmallInt::cast(raw_len).value();
  char* dst = static_cast<char*>(std::malloc(len + 1));
  CHECK(dst != nullptr, "Buffer allocation failure");
  stringFormatToBuffer(fmt, args, dst, len);
  HandleScope scope(thread);
  Object result(&scope, thread->runtime()->newStrFromCStr(dst));
  std::free(dst);
  return *result;
}

RawObject StrBuiltins::dunderMod(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  Object other(&scope, args.get(1));
  if (self.isStr()) {
    Str format(&scope, *self);
    Tuple format_args(&scope, runtime->emptyTuple());
    if (other.isTuple()) {
      format_args = *other;
    } else {
      Tuple tuple(&scope, runtime->newTuple(1));
      tuple.atPut(0, *other);
      format_args = *tuple;
    }
    return strFormat(thread, format, format_args);
  }
  // TODO(cshapiro): handle user-defined subtypes of string.
  return NotImplementedType::object();
}

RawObject StrBuiltins::dunderMul(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfStr(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kStr);
  }
  Object count_index(&scope, args.get(1));
  Object count_obj(&scope, intFromIndex(thread, count_index));
  if (count_obj.isError()) return *count_obj;
  Int count_int(&scope, intUnderlying(thread, count_obj));
  word count = count_int.asWordSaturated();
  if (!SmallInt::isValid(count)) {
    return thread->raiseWithFmt(LayoutId::kOverflowError,
                                "cannot fit '%T' into an index-sized integer",
                                &count_index);
  }
  Str self(&scope, *self_obj);
  word length = self.length();
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

RawObject StrBuiltins::dunderNe(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Object other_obj(&scope, args.get(1));
  if (!thread->runtime()->isInstanceOfStr(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kStr);
  }
  if (!thread->runtime()->isInstanceOfStr(*other_obj)) {
    return NotImplementedType::object();
  }
  Str self(&scope, strUnderlying(thread, self_obj));
  Str other(&scope, strUnderlying(thread, other_obj));
  return Bool::fromBool(self.compare(*other) != 0);
}

RawObject StrBuiltins::slice(Thread* thread, const Str& str,
                             const Slice& slice) {
  HandleScope scope(thread);
  word start, stop, step;
  Object err(&scope, sliceUnpack(thread, slice, &start, &stop, &step));
  if (err.isError()) return *err;
  word length = Slice::adjustIndices(str.length(), &start, &stop, step);
  std::unique_ptr<char[]> buf(new char[length + 1]);
  buf[length] = '\0';
  for (word i = 0, index = start; i < length; i++, index += step) {
    buf[i] = str.charAt(index);
  }
  return thread->runtime()->newStrFromCStr(buf.get());
}

RawObject StrBuiltins::dunderGetItem(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfStr(*self)) {
    return thread->raiseRequiresType(self, SymbolId::kStr);
  }
  Str string(&scope, strUnderlying(thread, self));
  Object index_obj(&scope, args.get(1));
  if (runtime->isInstanceOfInt(*index_obj)) {
    Int index(&scope, intUnderlying(thread, index_obj));
    if (!index.isSmallInt()) {
      return thread->raiseWithFmt(
          LayoutId::kIndexError,
          "cannot fit index into an index-sized integer");
    }
    word idx = index.asWord();
    if (idx < 0) {
      idx += string.length();
    }
    if (idx < 0 || idx >= string.length()) {
      return thread->raiseWithFmt(LayoutId::kIndexError,
                                  "string index out of range");
    }
    byte c = string.charAt(idx);
    return SmallStr::fromBytes(View<byte>(&c, 1));
  }
  if (index_obj.isSlice()) {
    Slice str_slice(&scope, *index_obj);
    return slice(thread, string, str_slice);
  }
  // TODO(T27897506): use __index__ to get index
  return thread->raiseWithFmt(LayoutId::kTypeError,
                              "string indices must be integers or slices");
}

RawObject StrBuiltins::dunderIter(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfStr(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kStr);
  }
  Str self(&scope, strUnderlying(thread, self_obj));
  return runtime->newStrIterator(self);
}

// Convert a byte to its hex digits, and write them out to buf.
// Increments buf to point after the written characters.
void StrBuiltins::byteToHex(byte** buf, byte convert) {
  static byte hexdigits[] = {'0', '1', '2', '3', '4', '5', '6', '7',
                             '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
  // Since convert is unsigned, the right shift will not propagate the sign
  // bit, and the upper bits will be zero.
  *(*buf)++ = hexdigits[convert >> 4];
  *(*buf)++ = hexdigits[convert & 0x0f];
}

RawObject StrBuiltins::dunderRepr(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfStr(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kStr);
  }
  Str self(&scope, strUnderlying(thread, self_obj));
  const word self_len = self.length();
  word output_size = 0;
  word squote = 0;
  word dquote = 0;
  // Precompute the size so that only one string allocation is necessary.
  for (word i = 0; i < self_len; ++i) {
    word incr = 1;
    byte ch = self.charAt(i);
    switch (ch) {
      case '\'':
        squote++;
        break;
      case '"':
        dquote++;
        break;
      case '\\':
        // FALLTHROUGH
      case '\t':
        // FALLTHROUGH
      case '\r':
        // FALLTHROUGH
      case '\n':
        incr = 2;
        break;
      default:
        if (ch < ' ' || ch == 0x7f) {
          incr = 4;  // \xHH
        }
        break;
    }
    output_size += incr;
  }

  byte quote = '\'';
  bool unchanged = (output_size == self_len);
  if (squote > 0) {
    unchanged = false;
    // If there are both single quotes and double quotes, the outer quote will
    // be singles, and all internal quotes will need to be escaped.
    if (dquote > 0) {
      // Add the size of the escape backslashes on the single quotes.
      output_size += squote;
    } else {
      quote = '"';
    }
  }
  output_size += 2;  // quotes

  std::unique_ptr<byte[]> buf(new byte[output_size]);
  // Write in the quotes.
  buf[0] = quote;
  buf[output_size - 1] = quote;
  if (unchanged) {
    // Rest of the characters were all unmodified, copy them directly into the
    // buffer.
    self.copyTo(buf.get() + 1, self_len);
  } else {
    byte* curr = buf.get() + 1;
    for (word i = 0; i < self_len; ++i) {
      byte ch = self.charAt(i);
      // quote can't be handled in the switch case because it's not a constant.
      if (ch == quote) {
        *curr++ = '\\';
        *curr++ = ch;
        continue;
      }
      switch (ch) {
        case '\\':
          *curr++ = '\\';
          *curr++ = ch;
          break;
        case '\t':
          *curr++ = '\\';
          *curr++ = 't';
          break;
        case '\r':
          *curr++ = '\\';
          *curr++ = 'r';
          break;
        case '\n':
          *curr++ = '\\';
          *curr++ = 'n';
          break;
        default:
          if (ch >= 32 && ch < 127) {
            *curr++ = ch;
          } else {
            // Map non-printable ASCII to '\xhh'.
            *curr++ = '\\';
            *curr++ = 'x';
            byteToHex(&curr, ch);
          }
          break;
      }
    }
    DCHECK(curr == buf.get() + output_size - 1,
           "Didn't write the correct number of characters out");
  }
  return runtime->newStrWithAll(View<byte>{buf.get(), output_size});
}

RawObject StrBuiltins::lstrip(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfStr(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kStr);
  }
  Str str(&scope, strUnderlying(thread, self_obj));
  Object other_obj(&scope, args.get(1));
  if (other_obj.isNoneType()) {
    return strStripSpaceLeft(thread, str);
  }
  if (!runtime->isInstanceOfStr(*other_obj)) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "str.lstrip() arg must be None or str");
  }
  Str chars(&scope, strUnderlying(thread, other_obj));
  return strStripLeft(thread, str, chars);
}

RawObject StrBuiltins::rstrip(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfStr(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kStr);
  }
  Str str(&scope, strUnderlying(thread, self_obj));
  Object other_obj(&scope, args.get(1));
  if (other_obj.isNoneType()) {
    return strStripSpaceRight(thread, str);
  }
  if (!runtime->isInstanceOfStr(*other_obj)) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "str.rstrip() arg must be None or str");
  }
  Str chars(&scope, strUnderlying(thread, other_obj));
  return strStripRight(thread, str, chars);
}

RawObject StrBuiltins::strip(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfStr(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kStr);
  }
  Str str(&scope, strUnderlying(thread, self_obj));
  Object other_obj(&scope, args.get(1));
  if (other_obj.isNoneType()) {
    return strStripSpace(thread, str);
  }
  if (!runtime->isInstanceOfStr(*other_obj)) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "str.strip() arg must be None or str");
  }
  Str chars(&scope, strUnderlying(thread, other_obj));
  return strStrip(thread, str, chars);
}

const BuiltinMethod StrIteratorBuiltins::kBuiltinMethods[] = {
    {SymbolId::kDunderIter, dunderIter},
    {SymbolId::kDunderLengthHint, dunderLengthHint},
    {SymbolId::kDunderNext, dunderNext},
    {SymbolId::kSentinelId, nullptr},
};

RawObject StrIteratorBuiltins::dunderIter(Thread* thread, Frame* frame,
                                          word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isStrIterator()) {
    return thread->raiseWithFmt(
        LayoutId::kTypeError,
        "__iter__() must be called with a str iterator instance as the first "
        "argument");
  }
  return *self;
}

// TODO(T35578204) Implement this for UTF-8. This probably means keeping extra
// state and logic so that __next__() will advance to the next codepoint.

RawObject StrIteratorBuiltins::dunderNext(Thread* thread, Frame* frame,
                                          word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isStrIterator()) {
    return thread->raiseWithFmt(
        LayoutId::kTypeError,
        "__next__() must be called with a str iterator instance as the first "
        "argument");
  }
  StrIterator iter(&scope, *self);
  Object value(&scope, strIteratorNext(thread, iter));
  if (value.isError()) {
    return thread->raise(LayoutId::kStopIteration, NoneType::object());
  }
  return *value;
}

RawObject StrIteratorBuiltins::dunderLengthHint(Thread* thread, Frame* frame,
                                                word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isStrIterator()) {
    return thread->raiseWithFmt(
        LayoutId::kTypeError,
        "__length_hint__() must be called with a str iterator instance as the "
        "first argument");
  }
  StrIterator str_iterator(&scope, *self);
  Str str(&scope, str_iterator.iterable());
  return SmallInt::fromWord(str.length() - str_iterator.index());
}

}  // namespace python
