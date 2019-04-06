#include "str-builtins.h"

#include "frame.h"
#include "globals.h"
#include "int-builtins.h"
#include "list-builtins.h"
#include "objects.h"
#include "runtime.h"
#include "slice-builtins.h"
#include "thread.h"
#include "trampolines-inl.h"

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

static bool isAsciiSpace(byte ch) {
  return ((ch >= 0x09) && (ch <= 0x0D)) || ((ch >= 0x1C) && (ch <= 0x1F)) ||
         ch == 0x20;
}

RawObject strStripSpace(Thread* thread, const Str& src,
                        const StrStripDirection direction) {
  word length = src.length();
  if (length == 0) {
    return *src;
  }
  if (length == 1 && isAsciiSpace(src.charAt(0))) {
    return Str::empty();
  }

  word first = 0;
  if (direction == StrStripDirection::Left ||
      direction == StrStripDirection::Both) {
    while (first < length && isAsciiSpace(src.charAt(first))) {
      ++first;
    }
  }

  word last = 0;
  if (direction == StrStripDirection::Right ||
      direction == StrStripDirection::Both) {
    for (word i = length - 1; i >= first && isAsciiSpace(src.charAt(i)); i--) {
      last++;
    }
  }
  return thread->runtime()->strSubstr(thread, src, first,
                                      length - first - last);
}

RawObject strStrip(Thread* thread, const Str& src, const Str& str,
                   StrStripDirection direction) {
  word length = src.length();
  if (length == 0 || str.length() == 0) {
    return *src;
  }
  word first = 0;
  word last = 0;
  // TODO(jeethu): Use set lookup if chars is a LargeStr
  if (direction == StrStripDirection::Left ||
      direction == StrStripDirection::Both) {
    first = strSpan(src, str);
  }

  if (direction == StrStripDirection::Right ||
      direction == StrStripDirection::Both) {
    last = strRSpan(src, str, first);
  }
  return thread->runtime()->strSubstr(thread, src, first,
                                      length - first - last);
}

RawObject strIteratorNext(Thread* thread, const StrIterator& iter) {
  HandleScope scope(thread);
  word byte_offset = iter.index();
  Str underlying(&scope, iter.iterable());
  if (byte_offset >= underlying.length()) {
    return Error::object();
  }
  word num_bytes = 0;
  word code_point = underlying.codePointAt(byte_offset, &num_bytes);
  iter.setIndex(byte_offset + num_bytes);
  return RawSmallStr::fromCodePoint(code_point);
}

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
    {SymbolId::kJoin, join},
    {SymbolId::kLower, lower},
    {SymbolId::kLStrip, lstrip},
    {SymbolId::kRStrip, rstrip},
    {SymbolId::kStrip, strip},
    {SymbolId::kUpper, upper},
    {SymbolId::kSentinelId, nullptr},
};

RawObject StrBuiltins::dunderAdd(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  Object other(&scope, args.get(1));
  if (!runtime->isInstanceOfStr(*self)) {
    return thread->raiseRequiresType(self, SymbolId::kStr);
  }
  if (!runtime->isInstanceOfStr(*other)) {
    return thread->raiseRequiresType(other, SymbolId::kStr);
  }
  if (!self.isStr()) {
    UNIMPLEMENTED("Strict subclass of string");
  }
  if (!other.isStr()) {
    UNIMPLEMENTED("Strict subclass of string");
  }
  Str self_str(&scope, *self);
  Str other_str(&scope, *other);
  return runtime->strConcat(thread, self_str, other_str);
}

RawObject StrBuiltins::dunderBool(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (self_obj.isStr()) {
    return Bool::fromBool(*self_obj != Str::empty());
  }
  if (thread->runtime()->isInstanceOfStr(*self_obj)) {
    UNIMPLEMENTED("Strict subclass of string");
  }
  return thread->raiseRequiresType(self_obj, SymbolId::kStr);
}

RawObject StrBuiltins::dunderEq(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Object other(&scope, args.get(1));
  if (self.isStr() && other.isStr()) {
    return Bool::fromBool(RawStr::cast(*self).compare(*other) == 0);
  }
  // TODO(cshapiro): handle user-defined subtypes of string.
  return NotImplementedType::object();
}

RawObject StrBuiltins::dunderGe(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Object other(&scope, args.get(1));
  if (self.isStr() && other.isStr()) {
    return Bool::fromBool(RawStr::cast(*self).compare(*other) >= 0);
  }
  // TODO(cshapiro): handle user-defined subtypes of string.
  return NotImplementedType::object();
}

RawObject StrBuiltins::dunderGt(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Object other(&scope, args.get(1));
  if (self.isStr() && other.isStr()) {
    return Bool::fromBool(RawStr::cast(*self).compare(*other) > 0);
  }
  // TODO(cshapiro): handle user-defined subtypes of string.
  return NotImplementedType::object();
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

RawObject StrBuiltins::join(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object sep_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfStr(*sep_obj)) {
    return thread->raiseRequiresType(sep_obj, SymbolId::kStr);
  }
  Str sep(&scope, *sep_obj);
  Object iterable(&scope, args.get(1));
  // Tuples of strings
  if (iterable.isTuple()) {
    Tuple tuple(&scope, *iterable);
    return runtime->strJoin(thread, sep, tuple, tuple.length());
  }
  // Lists of strings
  if (iterable.isList()) {
    List list(&scope, *iterable);
    Tuple tuple(&scope, list.items());
    return runtime->strJoin(thread, sep, tuple, list.numItems());
  }
  // Iterators of strings
  List list(&scope, runtime->newList());
  listExtend(thread, list, iterable);
  Tuple tuple(&scope, list.items());
  return runtime->strJoin(thread, sep, tuple, list.numItems());
}

RawObject StrBuiltins::dunderLe(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Object other(&scope, args.get(1));
  if (self.isStr() && other.isStr()) {
    return Bool::fromBool(RawStr::cast(*self).compare(*other) <= 0);
  }
  // TODO(cshapiro): handle user-defined subtypes of string.
  return NotImplementedType::object();
}

RawObject StrBuiltins::dunderLen(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfStr(*self)) {
    return thread->raiseRequiresType(self, SymbolId::kStr);
  }
  Str str(&scope, *self);
  return SmallInt::fromWord(str.codePointLength());
}

RawObject StrBuiltins::lower(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfStr(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kStr);
  }
  if (!self_obj.isStr()) {
    UNIMPLEMENTED("Strict subclass of string");
  }
  Str self(&scope, *self_obj);
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
  if (!self_obj.isStr()) {
    UNIMPLEMENTED("Strict subclass of string");
  }
  Str self(&scope, *self_obj);
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
  Object self(&scope, args.get(0));
  Object other(&scope, args.get(1));
  if (self.isStr() && other.isStr()) {
    return Bool::fromBool(RawStr::cast(*self).compare(*other) < 0);
  }
  // TODO(cshapiro): handle user-defined subtypes of string.
  return NotImplementedType::object();
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
      return thread->raiseValueErrorWithCStr("Incomplete format");
    }
    switch (fmt.charAt(fmt_idx)) {
      case 'd': {
        len--;
        if (!args.at(arg_idx).isInt()) {
          return thread->raiseTypeErrorWithCStr("Argument mismatch");
        }
        len += snprintf(nullptr, 0, "%ld",
                        RawInt::cast(args.at(arg_idx)).asWord());
        arg_idx++;
      } break;
      case 'g': {
        len--;
        if (!args.at(arg_idx).isFloat()) {
          return thread->raiseTypeErrorWithCStr("Argument mismatch");
        }
        len += snprintf(nullptr, 0, "%g",
                        RawFloat::cast(args.at(arg_idx)).value());
        arg_idx++;
      } break;
      case 's': {
        len--;
        if (!args.at(arg_idx).isStr()) {
          return thread->raiseTypeErrorWithCStr("Argument mismatch");
        }
        len += RawStr::cast(args.at(arg_idx)).length();
        arg_idx++;
      } break;
      case '%':
        break;
      default:
        UNIMPLEMENTED("Unsupported format specifier");
    }
  }

  if (!SmallInt::isValid(len)) {
    return thread->raiseOverflowErrorWithCStr(
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
        word value = RawInt::cast(args.at(arg_idx++)).asWord();
        dst_idx += snprintf(&dst[dst_idx], len - dst_idx + 1, "%ld", value);
      } break;
      case 'g': {
        double value = RawFloat::cast(args.at(arg_idx++)).value();
        dst_idx += snprintf(&dst[dst_idx], len - dst_idx + 1, "%g", value);
      } break;
      case 's': {
        RawStr value = RawStr::cast(args.at(arg_idx));
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
  word len = RawSmallInt::cast(raw_len).value();
  char* dst = static_cast<char*>(std::malloc(len + 1));
  CHECK(dst != nullptr, "Buffer allocation failure");
  stringFormatToBuffer(fmt, args, dst, len);
  HandleScope scope(thread);
  Object result(&scope, thread->runtime()->newStrFromCStr(dst));
  std::free(dst);
  return *result;
}

RawObject StrBuiltins::dunderMod(Thread* thread, Frame* caller, word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(caller, nargs);
  Object self(&scope, args.get(0));
  Object other(&scope, args.get(1));
  if (self.isStr()) {
    Str format(&scope, *self);
    Tuple format_args(&scope, runtime->newTuple(0));
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
  Object count_obj(&scope, args.get(1));
  count_obj = intFromIndex(thread, count_obj);
  if (count_obj.isError()) return *count_obj;
  Int count_int(&scope, *count_obj);
  word count = count_int.asWordSaturated();
  if (!SmallInt::isValid(count)) {
    return thread->raiseOverflowErrorWithCStr(
        "cannot fit count into an index-sized integer");
  }
  Str self(&scope, *self_obj);
  word length = self.length();
  if (count <= 0 || length == 0) {
    return Str::empty();
  }
  word new_length;
  if (__builtin_mul_overflow(length, count, &new_length) ||
      !SmallInt::isValid(new_length)) {
    return thread->raiseOverflowErrorWithCStr("repeated string is too long");
  }
  return runtime->strRepeat(thread, self, count);
}

RawObject StrBuiltins::dunderNe(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Object other(&scope, args.get(1));
  if (self.isStr() && other.isStr()) {
    return Bool::fromBool(RawStr::cast(*self).compare(*other) != 0);
  }
  // TODO(cshapiro): handle user-defined subtypes of string.
  return NotImplementedType::object();
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

  // TODO(T36619828): strict subclass of str
  Str string(&scope, *self);
  Object index(&scope, args.get(1));
  if (runtime->isInstanceOfInt(*index)) {
    // TODO(T38780562): strict subclass of int
    if (!index.isSmallInt()) {
      return thread->raiseIndexErrorWithCStr(
          "cannot fit index into an index-sized integer");
    }
    word idx = RawSmallInt::cast(*index).value();
    if (idx < 0) {
      idx += string.length();
    }
    if (idx < 0 || idx >= string.length()) {
      return thread->raiseIndexErrorWithCStr("string index out of range");
    }
    byte c = string.charAt(idx);
    return SmallStr::fromBytes(View<byte>(&c, 1));
  }
  if (index.isSlice()) {
    Slice str_slice(&scope, *index);
    return slice(thread, string, str_slice);
  }
  // TODO(T27897506): use __index__ to get index
  return thread->raiseTypeErrorWithCStr(
      "string indices must be integers or slices");
}

RawObject StrBuiltins::dunderIter(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfStr(*self)) {
    return thread->raiseRequiresType(self, SymbolId::kStr);
  }
  if (!self.isStr()) {
    UNIMPLEMENTED("str.__iter__(<subtype of str>)");
  }
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
  if (!self_obj.isStr()) {
    UNIMPLEMENTED("Strict subclass of string");
  }
  Str self(&scope, *self_obj);
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
  Object self(&scope, args.get(0));
  if (!runtime->isInstanceOfStr(*self)) {
    return thread->raiseRequiresType(self, SymbolId::kStr);
  }
  if (!self.isStr()) {
    UNIMPLEMENTED("Strict subclass of string");
  }
  Str str(&scope, *self);
  Object other(&scope, args.get(1));
  if (other.isNoneType()) {
    return strStripSpace(thread, str, StrStripDirection::Left);
  }
  if (!runtime->isInstanceOfStr(*other)) {
    return thread->raiseTypeErrorWithCStr(
        "str.lstrip() arg must be None or str");
  }
  if (!other.isStr()) {
    UNIMPLEMENTED("Strict subclass of string");
  }
  Str chars(&scope, *other);
  return strStrip(thread, str, chars, StrStripDirection::Left);
}

RawObject StrBuiltins::rstrip(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  if (!runtime->isInstanceOfStr(*self)) {
    return thread->raiseRequiresType(self, SymbolId::kStr);
  }
  if (!self.isStr()) {
    UNIMPLEMENTED("Strict subclass of string");
  }
  Str str(&scope, *self);
  Object other(&scope, args.get(1));
  if (other.isNoneType()) {
    return strStripSpace(thread, str, StrStripDirection::Right);
  }
  if (!runtime->isInstanceOfStr(*other)) {
    return thread->raiseTypeErrorWithCStr(
        "str.rstrip() arg must be None or str");
  }
  if (!other.isStr()) {
    UNIMPLEMENTED("Strict subclass of string");
  }
  Str chars(&scope, *other);
  return strStrip(thread, str, chars, StrStripDirection::Right);
}

RawObject StrBuiltins::strip(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  if (!runtime->isInstanceOfStr(*self)) {
    return thread->raiseRequiresType(self, SymbolId::kStr);
  }
  if (!self.isStr()) {
    UNIMPLEMENTED("Strict subclass of string");
  }
  Str str(&scope, *self);
  Object other(&scope, args.get(1));
  if (other.isNoneType()) {
    return strStripSpace(thread, str, StrStripDirection::Both);
  }
  if (!runtime->isInstanceOfStr(*other)) {
    return thread->raiseTypeErrorWithCStr(
        "str.strip() arg must be None or str");
  }
  if (!other.isStr()) {
    UNIMPLEMENTED("Strict subclass of string");
  }
  Str chars(&scope, *other);
  return strStrip(thread, str, chars, StrStripDirection::Both);
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
    return thread->raiseTypeErrorWithCStr(
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
    return thread->raiseTypeErrorWithCStr(
        "__next__() must be called with a str iterator instance as the first "
        "argument");
  }
  StrIterator iter(&scope, *self);
  Object value(&scope, strIteratorNext(thread, iter));
  if (value.isError()) {
    return thread->raiseStopIteration(NoneType::object());
  }
  return *value;
}

RawObject StrIteratorBuiltins::dunderLengthHint(Thread* thread, Frame* frame,
                                                word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isStrIterator()) {
    return thread->raiseTypeErrorWithCStr(
        "__length_hint__() must be called with a str iterator instance as the "
        "first argument");
  }
  StrIterator str_iterator(&scope, *self);
  Str str(&scope, str_iterator.iterable());
  return SmallInt::fromWord(str.length() - str_iterator.index());
}

}  // namespace python
