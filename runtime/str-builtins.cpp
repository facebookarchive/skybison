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
#include "unicode.h"

namespace python {

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

  // TODO(T41400083): Use a different search algorithm
  return SmallInt::fromWord(strCountSubStrFromTo(
      haystack, needle, start_index, end_index, haystack.charLength()));
}

word strCountSubStrFromTo(const Str& haystack, const Str& needle, word start,
                          word end, word max_count) {
  DCHECK(max_count > 0, "max_count must be positive");
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

// TODO(T43723300): Handle unicode
RawObject strStripSpace(Thread* thread, const Str& src) {
  word length = src.charLength();
  if (length == 0) {
    return *src;
  }
  if (length == 1 && isSpaceASCII(src.charAt(0))) {
    return Str::empty();
  }
  word first = 0;
  while (first < length && isSpaceASCII(src.charAt(first))) {
    ++first;
  }
  word last = 0;
  for (word i = length - 1; i >= first && isSpaceASCII(src.charAt(i)); i--) {
    last++;
  }
  return thread->runtime()->strSubstr(thread, src, first,
                                      length - first - last);
}

RawObject strStripSpaceLeft(Thread* thread, const Str& src) {
  word length = src.charLength();
  if (length == 0) {
    return *src;
  }
  if (length == 1 && isSpaceASCII(src.charAt(0))) {
    return Str::empty();
  }
  word first = 0;
  while (first < length && isSpaceASCII(src.charAt(first))) {
    ++first;
  }
  return thread->runtime()->strSubstr(thread, src, first, length - first);
}

RawObject strStripSpaceRight(Thread* thread, const Str& src) {
  word length = src.charLength();
  if (length == 0) {
    return *src;
  }
  if (length == 1 && isSpaceASCII(src.charAt(0))) {
    return Str::empty();
  }
  word last = 0;
  for (word i = length - 1; i >= 0 && isSpaceASCII(src.charAt(i)); i--) {
    last++;
  }
  return thread->runtime()->strSubstr(thread, src, 0, length - last);
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

RawObject strUnderlying(Thread* thread, const Object& obj) {
  if (obj.isStr()) return *obj;
  DCHECK(thread->runtime()->isInstanceOfStr(*obj),
         "cannot get a base str value from a non-str");
  HandleScope scope(thread);
  UserStrBase user_str(&scope, *obj);
  return user_str.value();
}

struct FormatSpec {
  char alignment;
  char positive_sign;
  char thousands_separator;
  bool alternate;
  int32_t fill_char;
  int32_t type;
  word width;
  word precision;
};

static bool isAlignmentSpec(int32_t c) {
  switch (c) {
    case '<':
    case '>':
    case '=':
    case '^':
      return true;
    default:
      return false;
  }
}

static int32_t nextCodePoint(const Str& spec, word length, word* index) {
  if (*index >= length) {
    return 0;
  }
  word cp_length;
  int32_t cp = spec.codePointAt(*index, &cp_length);
  *index += cp_length;
  return cp;
}

static RawObject parseFormatSpec(Thread* thread, const Str& spec,
                                 int32_t default_type, char default_align,
                                 FormatSpec* result) {
  result->alignment = default_align;
  result->positive_sign = 0;
  result->thousands_separator = 0;
  result->type = default_type;
  result->alternate = false;
  result->fill_char = ' ';
  result->width = -1;
  result->precision = -1;

  word index = 0;
  word length = spec.charLength();
  int32_t cp = nextCodePoint(spec, length, &index);

  bool fill_char_specified = false;
  bool alignment_specified = false;
  word old_index = index;
  int32_t c_next = nextCodePoint(spec, length, &index);
  if (isAlignmentSpec(c_next)) {
    result->alignment = static_cast<char>(c_next);
    result->fill_char = cp;
    fill_char_specified = true;
    alignment_specified = true;

    cp = nextCodePoint(spec, length, &index);
  } else if (!alignment_specified && isAlignmentSpec(cp)) {
    result->alignment = static_cast<char>(cp);
    alignment_specified = true;
    cp = c_next;
  } else {
    index = old_index;
  }

  switch (cp) {
    case '+':
    case ' ':
      result->positive_sign = static_cast<char>(cp);
      cp = nextCodePoint(spec, length, &index);
      break;
    case '-':
      cp = nextCodePoint(spec, length, &index);
      break;
  }

  if (!fill_char_specified && cp == '0') {
    result->fill_char = '0';
    if (!alignment_specified) {
      result->alignment = '=';
    }
  }

  if (cp == '#') {
    result->alternate = true;
    cp = nextCodePoint(spec, length, &index);
  }

  if ('0' <= cp && cp <= '9') {
    word width = 0;
    for (;;) {
      width += cp - '0';
      cp = nextCodePoint(spec, length, &index);
      if ('0' > cp || cp > '9') break;
      if (__builtin_mul_overflow(width, 10, &width)) {
        return thread->raiseWithFmt(LayoutId::kValueError,
                                    "Too many decimal digits in format string");
      }
    }
    result->width = width;
  }

  if (cp == ',') {
    result->thousands_separator = ',';
    cp = nextCodePoint(spec, length, &index);
  }
  if (cp == '_') {
    if (result->thousands_separator != 0) {
      return thread->raiseWithFmt(LayoutId::kValueError,
                                  "Cannot specify both ',' and '_'.");
    }
    result->thousands_separator = '_';
    cp = nextCodePoint(spec, length, &index);
  }
  if (cp == ',') {
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "Cannot specify both ',' and '_'.");
  }

  if (cp == '.') {
    cp = nextCodePoint(spec, length, &index);
    if ('0' > cp || cp > '9') {
      return thread->raiseWithFmt(LayoutId::kValueError,
                                  "Format specifier missing precision");
    }

    word precision = 0;
    for (;;) {
      precision += cp - '0';
      cp = nextCodePoint(spec, length, &index);
      if ('0' > cp || cp > '9') break;
      if (__builtin_mul_overflow(precision, 10, &precision)) {
        return thread->raiseWithFmt(LayoutId::kValueError,
                                    "Too many decimal digits in format string");
      }
    }
    result->precision = precision;
  }

  if (cp != 0) {
    result->type = cp;
    // This was the last step: No need to call `nextCodePoint()` here.
  }
  if (index < length) {
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "Invalid format specifier");
  }

  if (result->thousands_separator) {
    switch (result->type) {
      case 'd':
      case 'e':
      case 'f':
      case 'g':
      case 'E':
      case 'G':
      case '%':
      case 'F':
      case '\0':
        // These are allowed. See PEP 378.
        break;
      case 'b':
      case 'o':
      case 'x':
      case 'X':
        // Underscores are allowed in bin/oct/hex. See PEP 515.
        if (result->thousands_separator == '_') {
          break;
        }
        /* fall through */
      default:
        if (32 < result->type && result->type <= kMaxASCII) {
          return thread->raiseWithFmt(
              LayoutId::kValueError, "Cannot specify '%c' with '%c'.",
              result->thousands_separator, static_cast<char>(result->type));
        }
        return thread->raiseWithFmt(
            LayoutId::kValueError, "Cannot specify '%c' with '\\x%x'.",
            result->thousands_separator, static_cast<unsigned>(result->type));
    }
  }
  return NoneType::object();
}

static RawObject strFormat(Thread* thread, const Str& str, FormatSpec* format) {
  if (format->positive_sign != '\0') {
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "Sign not allowed in string format specifier");
  }
  if (format->alternate) {
    return thread->raiseWithFmt(
        LayoutId::kValueError,
        "Alternate form (#) not allowed in string format specifier");
  }
  if (format->alignment == '=') {
    return thread->raiseWithFmt(
        LayoutId::kValueError,
        "'=' alignment not allowed in string format specifier");
  }
  word width = format->width;
  word precision = format->precision;
  if (width == -1 && precision == -1) {
    return *str;
  }

  word char_length = str.charLength();
  word codepoint_length;
  word str_end_index;
  if (precision >= 0) {
    str_end_index = str.offsetByCodePoints(0, precision);
    if (str_end_index < char_length) {
      codepoint_length = precision;
    } else {
      codepoint_length = str.codePointLength();
    }
  } else {
    str_end_index = char_length;
    codepoint_length = str.codePointLength();
  }

  Runtime* runtime = thread->runtime();
  word padding = width - codepoint_length;
  if (padding <= 0) {
    return runtime->strSubstr(thread, str, 0, str_end_index);
  }

  // Construct result.
  HandleScope scope(thread);
  Str fill_char(&scope, SmallStr::fromCodePoint(format->fill_char));
  word fill_char_length = fill_char.charLength();
  word padding_char_length = padding * fill_char_length;
  word result_char_length = str_end_index + padding_char_length;
  MutableBytes result(
      &scope, runtime->newMutableBytesUninitialized(result_char_length));
  word index = 0;
  word early_padding;
  if (format->alignment == '>') {
    early_padding = padding;
    padding = 0;
  } else if (format->alignment == '^') {
    word half = padding / 2;
    early_padding = half;
    padding = half + (padding % 2);
  } else {
    early_padding = 0;
    DCHECK(format->alignment == '<', "remaining assignment must be '<'");
  }
  for (word i = 0; i < early_padding; i++) {
    result.replaceFromWithStr(index, *fill_char, fill_char_length);
    index += fill_char_length;
  }
  result.replaceFromWithStr(index, *str, str_end_index);
  index += str_end_index;
  if (padding > 0) {
    DCHECK(format->alignment == '<' || format->alignment == '^',
           "unexpected alignment");
    for (word i = 0; i < padding; i++) {
      result.replaceFromWithStr(index, *fill_char, fill_char_length);
      index += fill_char_length;
    }
  }
  DCHECK(index == result_char_length, "overflow or underflow in result");
  return result.becomeStr();
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
    {SymbolId::kDunderFormat, dunderFormat},
    {SymbolId::kDunderGe, dunderGe},
    {SymbolId::kDunderGetitem, dunderGetItem},
    {SymbolId::kDunderGt, dunderGt},
    {SymbolId::kDunderHash, dunderHash},
    {SymbolId::kDunderIter, dunderIter},
    {SymbolId::kDunderLe, dunderLe},
    {SymbolId::kDunderLen, dunderLen},
    {SymbolId::kDunderLt, dunderLt},
    {SymbolId::kDunderMul, dunderMul},
    {SymbolId::kDunderNe, dunderNe},
    {SymbolId::kDunderRepr, dunderRepr},
    {SymbolId::kIsalnum, isalnum},
    {SymbolId::kIsalpha, isalpha},
    {SymbolId::kIsdecimal, isdecimal},
    {SymbolId::kIsdigit, isdigit},
    {SymbolId::kIsidentifier, isidentifier},
    {SymbolId::kIslower, islower},
    {SymbolId::kIsnumeric, isnumeric},
    {SymbolId::kIsprintable, isprintable},
    {SymbolId::kIsspace, isspace},
    {SymbolId::kIstitle, istitle},
    {SymbolId::kIsupper, isupper},
    {SymbolId::kLStrip, lstrip},
    {SymbolId::kLower, lower},
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

RawObject StrBuiltins::dunderFormat(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfStr(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kStr);
  }
  Str self(&scope, strUnderlying(thread, self_obj));

  Object spec_obj(&scope, args.get(1));
  if (!runtime->isInstanceOfStr(*spec_obj)) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "__format__() argument 1 must be str, not %T",
                                &spec_obj);
  }
  Str spec(&scope, strUnderlying(thread, spec_obj));

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
    if (32 < format.type && format.type < kMaxASCII) {
      return thread->raiseWithFmt(
          LayoutId::kValueError,
          "Unknown format code '%c' for object of type '%T'",
          static_cast<char>(format.type), &self_obj);
    }
    return thread->raiseWithFmt(
        LayoutId::kValueError,
        "Unknown format code '\\x%x' for object of type '%T'",
        static_cast<unsigned>(format.type), &self_obj);
  }

  return strFormat(thread, self, &format);
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
  for (word i = 0; i < str.charLength(); i++) {
    if (str.charAt(i) > kMaxASCII) {
      return false;
    }
  }
  return true;
}

RawObject strFind(const Str& haystack, const Str& needle, word start,
                  word end) {
  if (end < 0 || start < 0) {
    Slice::adjustSearchIndices(&start, &end, haystack.codePointLength());
  }

  word start_index = haystack.offsetByCodePoints(0, start);
  if (start_index == haystack.charLength() && needle.charLength() > 0) {
    // Haystack is too small; fast early return
    return SmallInt::fromWord(-1);
  }
  word end_index = haystack.offsetByCodePoints(start_index, end - start);

  if ((end_index - start_index) < needle.charLength() ||
      start_index > end_index) {
    // Haystack is too small; fast early return
    return SmallInt::fromWord(-1);
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
  while (i < str.charLength() && isSpaceASCII(str.charAt(i))) {
    i++;
  }
  return i;
}

bool strHasPrefix(const Str& str, const Str& prefix, word start) {
  word str_len = str.charLength();
  word prefix_len = prefix.charLength();
  if (str_len - start + 1 < prefix_len) {
    return false;
  }
  for (word i = 0; i < prefix_len; i++) {
    if (str.charAt(start + i) != prefix.charAt(i)) {
      return false;
    }
  }
  return true;
}

RawObject strRFind(const Str& haystack, const Str& needle, word start,
                   word end) {
  if (end < 0 || start < 0) {
    Slice::adjustSearchIndices(&start, &end, haystack.codePointLength());
  }

  word start_index = haystack.offsetByCodePoints(0, start);
  if (start_index == haystack.charLength() && needle.charLength() > 0) {
    // Haystack is too small; fast early return
    return SmallInt::fromWord(-1);
  }
  word end_index = haystack.offsetByCodePoints(start_index, end - start);

  if ((end_index - start_index) < needle.charLength() ||
      start_index > end_index) {
    // Haystack is too small; fast early return
    return SmallInt::fromWord(-1);
  }

  // Loop is in byte space, not code point space
  word result = start;
  word last_index = -1;
  // TODO(T41400083): Use a different search algorithm
  for (word i = start_index; i <= end_index - needle.charLength(); result++) {
    if (strHasPrefix(haystack, needle, i)) {
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
  std::unique_ptr<byte[]> buf(new byte[self.charLength()]);
  byte* bufp = buf.get();
  for (word i = 0; i < self.charLength(); i++) {
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
  Str result(&scope,
             runtime->newStrWithAll(View<byte>{bufp, self.charLength()}));
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
  std::unique_ptr<byte[]> buf(new byte[self.charLength()]);
  byte* bufp = buf.get();
  for (word i = 0; i < self.charLength(); i++) {
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
  Str result(&scope,
             runtime->newStrWithAll(View<byte>{bufp, self.charLength()}));
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
  word length = Slice::adjustIndices(str.charLength(), &start, &stop, step);
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
      idx += string.charLength();
    }
    if (idx < 0 || idx >= string.charLength()) {
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
  const word self_len = self.charLength();
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

RawObject StrBuiltins::isalnum(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfStr(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kStr);
  }
  Str self(&scope, strUnderlying(thread, self_obj));
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
    if (!isAlnumASCII(b)) {
      return Bool::falseObj();
    }
  } while (i < char_length);
  return Bool::trueObj();
}

RawObject StrBuiltins::isalpha(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfStr(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kStr);
  }
  Str self(&scope, strUnderlying(thread, self_obj));
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
    if (!isAlphaASCII(b)) {
      return Bool::falseObj();
    }
  } while (i < char_length);
  return Bool::trueObj();
}

RawObject StrBuiltins::isdecimal(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfStr(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kStr);
  }
  Str self(&scope, strUnderlying(thread, self_obj));
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
    if (!isDecimalASCII(b)) {
      return Bool::falseObj();
    }
  } while (i < char_length);
  return Bool::trueObj();
}

RawObject StrBuiltins::isdigit(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfStr(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kStr);
  }
  Str self(&scope, strUnderlying(thread, self_obj));
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
    if (!isDigitASCII(b)) {
      return Bool::falseObj();
    }
  } while (i < char_length);
  return Bool::trueObj();
}

RawObject StrBuiltins::isidentifier(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfStr(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kStr);
  }
  Str self(&scope, strUnderlying(thread, self_obj));
  word char_length = self.charLength();
  if (char_length == 0) {
    return Bool::falseObj();
  }
  byte b0 = self.charAt(0);
  if (b0 > kMaxASCII) {
    UNIMPLEMENTED("non-ASCII character");
  }
  if (!isIdStartASCII(b0)) {
    return Bool::falseObj();
  }
  for (word i = 1; i < char_length; i++) {
    byte b = self.charAt(i);
    if (b > kMaxASCII) {
      UNIMPLEMENTED("non-ASCII character");
    }
    if (!isIdContinueASCII(b)) {
      return Bool::falseObj();
    }
  }
  return Bool::trueObj();
}

RawObject StrBuiltins::islower(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfStr(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kStr);
  }
  Str self(&scope, strUnderlying(thread, self_obj));
  word char_length = self.charLength();
  bool cased = false;
  for (word i = 0; i < char_length; i++) {
    byte b = self.charAt(i);
    if (b > kMaxASCII) {
      UNIMPLEMENTED("non-ASCII character");
    }
    if (isUpperASCII(b)) {
      return Bool::falseObj();
    }
    if (!cased && isLowerASCII(b)) {
      cased = true;
    }
  }
  return Bool::fromBool(cased);
}

RawObject StrBuiltins::isnumeric(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfStr(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kStr);
  }
  Str self(&scope, strUnderlying(thread, self_obj));
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
    if (!isNumericASCII(b)) {
      return Bool::falseObj();
    }
  } while (i < char_length);
  return Bool::trueObj();
}

RawObject StrBuiltins::isprintable(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfStr(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kStr);
  }
  Str self(&scope, strUnderlying(thread, self_obj));
  for (word i = 0, char_length = self.charLength(); i < char_length; i++) {
    byte b = self.charAt(i);
    if (b > kMaxASCII) {
      UNIMPLEMENTED("non-ASCII character");
    }
    if (!isPrintableASCII(b)) {
      return Bool::falseObj();
    }
  }
  return Bool::trueObj();
}

RawObject StrBuiltins::isspace(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfStr(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kStr);
  }
  Str self(&scope, strUnderlying(thread, self_obj));
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
    if (!isSpaceASCII(b)) {
      return Bool::falseObj();
    }
  } while (i < char_length);
  return Bool::trueObj();
}

RawObject StrBuiltins::istitle(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfStr(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kStr);
  }
  Str self(&scope, strUnderlying(thread, self_obj));
  bool cased = false;
  bool previous_is_cased = false;
  for (word i = 0, char_length = self.charLength(); i < char_length; i++) {
    byte b = self.charAt(i);
    if (isUpperASCII(b)) {
      if (previous_is_cased) return Bool::falseObj();
      previous_is_cased = true;
      cased = true;
    } else if (isLowerASCII(b)) {
      if (!previous_is_cased) return Bool::falseObj();
      previous_is_cased = true;
      cased = true;
    } else {
      previous_is_cased = false;
    }
  }
  return Bool::fromBool(cased);
}

RawObject StrBuiltins::isupper(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfStr(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kStr);
  }
  Str self(&scope, strUnderlying(thread, self_obj));
  word char_length = self.charLength();
  bool cased = false;
  for (word i = 0; i < char_length; i++) {
    byte b = self.charAt(i);
    if (b > kMaxASCII) {
      UNIMPLEMENTED("non-ASCII character");
    }
    if (isLowerASCII(b)) {
      return Bool::falseObj();
    }
    if (!cased && isUpperASCII(b)) {
      cased = true;
    }
  }
  return Bool::fromBool(cased);
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
  return SmallInt::fromWord(str.charLength() - str_iterator.index());
}

}  // namespace python
