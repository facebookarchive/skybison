#include "codecs-module.h"

#include "bytearray-builtins.h"
#include "bytes-builtins.h"
#include "frame.h"
#include "frozen-modules.h"
#include "int-builtins.h"
#include "runtime.h"
#include "str-builtins.h"

extern "C" unsigned char _PyLong_DigitValue[];  // from Include/longobject.h
extern "C" unsigned int _Py_ctype_table[];      // from Include/pyctype.h

namespace python {

const int32_t kLowSurrogateStart = 0xDC00;
const int32_t kHighSurrogateStart = 0xD800;
const char kASCIIReplacement = '?';

static SymbolId lookupSymbolForErrorHandler(const Str& error) {
  if (error.equalsCStr("strict")) {
    return SymbolId::kStrict;
  }
  if (error.equalsCStr("ignore")) {
    return SymbolId::kIgnore;
  }
  if (error.equalsCStr("replace")) {
    return SymbolId::kReplace;
  }
  if (error.equalsCStr("surrogateescape")) {
    return SymbolId::kSurrogateescape;
  }
  return SymbolId::kInvalid;
}

static int asciiDecode(Thread* thread, const StrArray& dst, const Bytes& src,
                       int index) {
  // TODO(T41032331): Implement a fastpass to read longs instead of chars
  Runtime* runtime = thread->runtime();
  for (; index < src.length(); index++) {
    byte byte = src.byteAt(index);
    if (byte & 0x80) {
      break;
    }
    runtime->strArrayAddASCII(thread, dst, byte);
  }
  return index;
}

const BuiltinMethod UnderCodecsModule::kBuiltinMethods[] = {
    {SymbolId::kUnderAsciiDecode, underAsciiDecode},
    {SymbolId::kUnderAsciiEncode, underAsciiEncode},
    {SymbolId::kUnderByteArrayStringAppend, underByteArrayStringAppend},
    {SymbolId::kUnderEscapeDecode, underEscapeDecode},
    {SymbolId::kUnderLatin1Decode, underLatin1Decode},
    {SymbolId::kUnderLatin1Encode, underLatin1Encode},
    {SymbolId::kUnderUnicodeEscapeDecode, underUnicodeEscapeDecode},
    {SymbolId::kUnderUtf16Encode, underUtf16Encode},
    {SymbolId::kUnderUtf32Encode, underUtf32Encode},
    {SymbolId::kUnderUtf8Encode, underUtf8Encode},
    {SymbolId::kSentinelId, nullptr},
};

const char* const UnderCodecsModule::kFrozenData = kUnderCodecsModuleData;

RawObject UnderCodecsModule::underAsciiDecode(Thread* thread, Frame* frame,
                                              word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object bytes_obj(&scope, args.get(0));
  Object errors_obj(&scope, args.get(1));
  Object index_obj(&scope, args.get(2));
  Object output_obj(&scope, args.get(3));
  DCHECK(runtime->isInstanceOfBytes(*bytes_obj),
         "First arg to _ascii_decode must be str");
  DCHECK(runtime->isInstanceOfStr(*errors_obj),
         "Second arg to _ascii_decode must be str");
  DCHECK(runtime->isInstanceOfInt(*index_obj),
         "Third arg to _ascii_decode must be int");
  DCHECK(output_obj.isStrArray(),
         "Fourth arg to _ascii_decode must be _strarray");
  Bytes bytes(&scope, bytesUnderlying(thread, bytes_obj));
  Str errors(&scope, strUnderlying(thread, errors_obj));
  Int index(&scope, intUnderlying(thread, index_obj));
  StrArray dst(&scope, *output_obj);

  Tuple result(&scope, runtime->newTuple(2));
  word length = bytes.length();
  runtime->strArrayEnsureCapacity(thread, dst, length);
  word outpos = asciiDecode(thread, dst, bytes, index.asWord());
  if (outpos == length) {
    result.atPut(0, runtime->strFromStrArray(dst));
    result.atPut(1, runtime->newInt(length));
    return *result;
  }

  SymbolId error_id = lookupSymbolForErrorHandler(errors);
  while (outpos < length) {
    byte c = bytes.byteAt(outpos);
    if (c < 128) {
      runtime->strArrayAddASCII(thread, dst, c);
      ++outpos;
      continue;
    }
    switch (error_id) {
      case SymbolId::kReplace: {
        Str temp(&scope, SmallStr::fromCodePoint(0xFFFD));
        runtime->strArrayAddStr(thread, dst, temp);
        ++outpos;
        break;
      }
      case SymbolId::kSurrogateescape: {
        Str temp(&scope, SmallStr::fromCodePoint(kLowSurrogateStart + c));
        runtime->strArrayAddStr(thread, dst, temp);
        ++outpos;
        break;
      }
      case SymbolId::kIgnore:
        ++outpos;
        break;
      default:
        result.atPut(0, runtime->newIntFromUnsigned(outpos));
        result.atPut(1, runtime->newIntFromUnsigned(outpos + 1));
        return *result;
    }
  }
  result.atPut(0, runtime->strFromStrArray(dst));
  result.atPut(1, runtime->newIntFromUnsigned(length));
  return *result;
}

static bool isSurrogate(int32_t codepoint) {
  return kHighSurrogateStart <= codepoint && codepoint <= 0xDFFF;
}

// CPython encodes latin1 codepoints into the low-surrogate range, and is able
// to recover the original codepoints from those decodable surrogate points.
static bool isEscapedLatin1Surrogate(int32_t codepoint) {
  return (kLowSurrogateStart + kMaxASCII) < codepoint &&
         codepoint <= (kLowSurrogateStart + kMaxByte);
}

RawObject UnderCodecsModule::underAsciiEncode(Thread* thread, Frame* frame,
                                              word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object data_obj(&scope, args.get(0));
  Object errors_obj(&scope, args.get(1));
  Object index_obj(&scope, args.get(2));
  Object output_obj(&scope, args.get(3));
  DCHECK(runtime->isInstanceOfStr(*data_obj),
         "First arg to _ascii_encode must be str");
  DCHECK(runtime->isInstanceOfStr(*errors_obj),
         "Second arg to _ascii_encode must be str");
  DCHECK(runtime->isInstanceOfInt(*index_obj),
         "Third arg to _ascii_encode must be int");
  DCHECK(runtime->isInstanceOfByteArray(*output_obj),
         "Fourth arg to _ascii_encode must be bytearray");
  Str data(&scope, strUnderlying(thread, data_obj));
  Str errors(&scope, strUnderlying(thread, errors_obj));
  Int index_int(&scope, intUnderlying(thread, index_obj));
  ByteArray output(&scope, *output_obj);

  Tuple result(&scope, runtime->newTuple(2));
  SymbolId error_symbol = lookupSymbolForErrorHandler(errors);
  word i = index_int.asWord();
  // TODO(T43252439): Optimize this by first checking whether the entire string
  // is ASCII, and just memcpy into a string if so
  for (word byte_offset = data.offsetByCodePoints(0, i);
       byte_offset < data.length(); i++) {
    word num_bytes;
    int32_t codepoint = data.codePointAt(byte_offset, &num_bytes);
    byte_offset += num_bytes;
    if (codepoint <= kMaxASCII) {
      byteArrayAdd(thread, runtime, output, codepoint);
    } else {
      switch (error_symbol) {
        case SymbolId::kIgnore:
          continue;
        case SymbolId::kReplace:
          byteArrayAdd(thread, runtime, output, kASCIIReplacement);
          continue;
        case SymbolId::kSurrogateescape:
          if (isEscapedLatin1Surrogate(codepoint)) {
            byteArrayAdd(thread, runtime, output,
                         codepoint - kLowSurrogateStart);
            continue;
          }
          break;
        default:
          break;
      }
      result.atPut(0, runtime->newInt(i));
      while (byte_offset < data.length() &&
             data.codePointAt(byte_offset, &num_bytes) > kMaxASCII) {
        byte_offset += num_bytes;
        i++;
      }
      result.atPut(1, runtime->newInt(i + 1));
      return *result;
    }
  }
  result.atPut(0, byteArrayAsBytes(thread, runtime, output));
  result.atPut(1, runtime->newInt(i));
  return *result;
}

// Decodes a sequence of unicode encoded bytes into a codepoint, returns
// -1 if no value should be written, and -2 if an error occurred. Sets the
// iterating variable to where decoding should continue, and sets
// invalid_escape_index if it doesn't recognize the escape sequence.
static int32_t decodeEscaped(const Bytes& bytes, word* i,
                             word* invalid_escape_index) {
  word length = bytes.length();
  switch (byte ch = bytes.byteAt((*i)++)) {
      // \x escapes
    case '\n':
      return -1;
    case '\\':
    case '\'':
    case '\"':
      return ch;
    case 'b':
      return '\b';
    case 't':
      return '\t';
    case 'n':
      return '\n';
    case 'r':
      return '\r';
    // BEL,
    case 'a':
      return '\x07';
    // VT
    case 'v':
      return '\x0B';
    // FF
    case 'f':
      return '\x0C';

    // \OOO (octal) escapes
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7': {
      word escaped = ch - '0';
      word octal_index = *i;
      if (octal_index < length) {
        word ch2 = bytes.byteAt(octal_index);
        if ('0' <= ch2 && ch2 <= '7') {
          escaped = (escaped << 3) + ch2 - '0';
          if (++octal_index < length) {
            word ch3 = bytes.byteAt(octal_index);
            if ('0' <= ch3 && ch3 <= '7') {
              octal_index++;
              escaped = (escaped << 3) + ch3 - '0';
            }
          }
        }
      }
      *i = octal_index;
      return escaped;
    }

    // hex escapes
    // \xXX
    case 'x': {
      word hex_index = *i;
      if (hex_index + 1 < length) {
        int digit1, digit2;
        digit1 = _PyLong_DigitValue[bytes.byteAt(hex_index)];
        digit2 = _PyLong_DigitValue[bytes.byteAt(hex_index + 1)];
        if (digit1 < 16 && digit2 < 16) {
          *i += 2;
          return (digit1 << 4) + digit2;
        }
      }
      return -2;
    }
    default:
      *invalid_escape_index = *i - 1;
      return ch;
  }
}

RawObject UnderCodecsModule::underEscapeDecode(Thread* thread, Frame* frame,
                                               word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object bytes_obj(&scope, args.get(0));
  Object errors_obj(&scope, args.get(1));
  Object recode_obj(&scope, args.get(2));
  Runtime* runtime = thread->runtime();
  if (runtime->isInstanceOfStr(*bytes_obj)) {
    // TODO(T44739505): Make sure we can decode a str
    UNIMPLEMENTED("_codecs.escape_decode with a str");
  }
  DCHECK(runtime->isInstanceOfBytes(*bytes_obj),
         "First arg to _escape_decode must be str or bytes");
  DCHECK(runtime->isInstanceOfStr(*errors_obj),
         "Second arg to _escape_decode must be str");
  DCHECK(runtime->isInstanceOfStr(*recode_obj),
         "Third arg to _escape_decode must be str");
  Bytes bytes(&scope, bytesUnderlying(thread, bytes_obj));
  Str errors(&scope, strUnderlying(thread, errors_obj));

  ByteArray dst(&scope, runtime->newByteArray());
  word length = bytes.length();
  runtime->byteArrayEnsureCapacity(thread, dst, length);
  word first_invalid_escape_index = -1;
  for (word i = 0; i < length;) {
    byte ch = bytes.byteAt(i++);
    if (ch != '\\') {
      // TODO(T45134397): Support the recode_encoding parameter
      if (ch <= kMaxASCII) {
        byteArrayAdd(thread, runtime, dst, ch);
        continue;
      }
      Str temp(&scope, SmallStr::fromCodePoint(ch));
      byteArrayAdd(thread, runtime, dst, temp.charAt(0));
      byteArrayAdd(thread, runtime, dst, temp.charAt(1));
      continue;
    }
    if (i >= length) {
      return runtime->newStrFromCStr("Trailing \\ in string");
    }
    word invalid_escape_index = -1;
    int32_t decoded = decodeEscaped(bytes, &i, &invalid_escape_index);
    if (invalid_escape_index != -1) {
      byteArrayAdd(thread, runtime, dst, '\\');
      if (first_invalid_escape_index == -1) {
        first_invalid_escape_index = invalid_escape_index;
      }
    }
    if (decoded >= 0) {
      byteArrayAdd(thread, runtime, dst, decoded);
      continue;
    }
    if (decoded == -1) {
      continue;
    }
    SymbolId error_id = lookupSymbolForErrorHandler(errors);
    switch (error_id) {
      case SymbolId::kStrict:
        return runtime->newStrFromFmt("invalid \\x escape at position %d",
                                      i - 2);
      case SymbolId::kReplace: {
        byteArrayAdd(thread, runtime, dst, '?');
        break;
      }
      case SymbolId::kIgnore:
        break;
      default:
        return runtime->newStrFromFmt(
            "decoding error; unknown error handling code: %S", &errors);
    }
    if (i < length && (_Py_ctype_table[bytes.byteAt(i)] & 0x10)) {
      i++;
    }
  }
  Tuple result(&scope, runtime->newTuple(3));
  result.atPut(0, byteArrayAsBytes(thread, runtime, dst));
  result.atPut(1, runtime->newInt(length));
  result.atPut(2, runtime->newInt(first_invalid_escape_index));
  return *result;
}

RawObject UnderCodecsModule::underLatin1Decode(Thread* thread, Frame* frame,
                                               word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object bytes_obj(&scope, args.get(0));
  DCHECK(runtime->isInstanceOfBytes(*bytes_obj),
         "First arg to _latin_1_decode must be str");
  Bytes bytes(&scope, bytesUnderlying(thread, bytes_obj));
  StrArray array(&scope, runtime->newStrArray());
  word length = bytes.length();
  runtime->strArrayEnsureCapacity(thread, array, length);
  // First, try a quick ASCII decoding
  word num_bytes = asciiDecode(thread, array, bytes, 0);
  if (num_bytes != length) {
    // A non-ASCII character was found; switch to a Latin-1 decoding for the
    // remainder of the input sequence
    Str str(&scope, Str::empty());
    for (word i = num_bytes; i < length; ++i) {
      byte code_point = bytes.byteAt(i);
      if (code_point <= kMaxASCII) {
        runtime->strArrayAddASCII(thread, array, code_point);
      } else {
        str = SmallStr::fromCodePoint(code_point);
        runtime->strArrayAddStr(thread, array, str);
      }
    }
  }
  Tuple result(&scope, runtime->newTuple(2));
  result.atPut(0, runtime->strFromStrArray(array));
  result.atPut(1, runtime->newInt(length));
  return *result;
}

RawObject UnderCodecsModule::underLatin1Encode(Thread* thread, Frame* frame,
                                               word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object data_obj(&scope, args.get(0));
  Object errors_obj(&scope, args.get(1));
  Object index_obj(&scope, args.get(2));
  Object output_obj(&scope, args.get(3));
  DCHECK(runtime->isInstanceOfStr(*data_obj),
         "First arg to _latin_1_encode must be str");
  DCHECK(runtime->isInstanceOfStr(*errors_obj),
         "Second arg to _latin_1_encode must be str");
  DCHECK(runtime->isInstanceOfInt(*index_obj),
         "Third arg to _latin_1_encode must be int");
  DCHECK(runtime->isInstanceOfByteArray(*output_obj),
         "Fourth arg to _latin_1_encode must be bytearray");
  Str data(&scope, strUnderlying(thread, data_obj));
  Str errors(&scope, strUnderlying(thread, errors_obj));
  Int index_int(&scope, intUnderlying(thread, index_obj));
  ByteArray output(&scope, *output_obj);

  Tuple result(&scope, runtime->newTuple(2));
  SymbolId error_symbol = lookupSymbolForErrorHandler(errors);
  word i = index_int.asWord();
  for (word byte_offset = data.offsetByCodePoints(0, i);
       byte_offset < data.length(); i++) {
    word num_bytes;
    int32_t codepoint = data.codePointAt(byte_offset, &num_bytes);
    byte_offset += num_bytes;
    if (codepoint <= kMaxByte) {
      byteArrayAdd(thread, runtime, output, codepoint);
    } else {
      switch (error_symbol) {
        case SymbolId::kIgnore:
          continue;
        case SymbolId::kReplace:
          byteArrayAdd(thread, runtime, output, kASCIIReplacement);
          continue;
        case SymbolId::kSurrogateescape:
          if (isEscapedLatin1Surrogate(codepoint)) {
            byteArrayAdd(thread, runtime, output,
                         codepoint - kLowSurrogateStart);
            continue;
          }
          break;
        default:
          break;
      }
      result.atPut(0, runtime->newInt(i));
      while (byte_offset < data.length() &&
             data.codePointAt(byte_offset, &num_bytes) > kMaxByte) {
        byte_offset += num_bytes;
        i++;
      }
      result.atPut(1, runtime->newInt(i + 1));
      return *result;
    }
  }
  result.atPut(0, byteArrayAsBytes(thread, runtime, output));
  result.atPut(1, runtime->newInt(i));
  return *result;
}

// Decodes a sequence of hexadecimal encoded bytes into a codepoint or returns
// a negative value if the value could not be decoded. Sets the start variable
// to where decoding should continue.
static int32_t decodeHexEscaped(const Bytes& bytes, word* start, word count) {
  DCHECK_BOUND(count, 8);
  word result = 0;
  word i = *start;
  for (word len = bytes.length(); i < len && count != 0; i++, count--) {
    byte ch = bytes.byteAt(i);
    result <<= 4;
    if (ch >= '0' && ch <= '9') {
      result += ch - '0';
    } else if (ch >= 'a' && ch <= 'f') {
      result += ch - ('a' - 10);
    } else if (ch >= 'A' && ch <= 'F') {
      result += ch - ('A' - 10);
    } else {
      break;  // not a hexadecimal digit, stop reading
    }
  }
  *start = i;
  if (count != 0) {
    return -1;
  }
  // if count is 4, result could be a 32-bit unicode character
  if (result > kMaxUnicode) {
    return -2;
  }
  return result;
}

// Decodes a sequence of unicode encoded bytes into a codepoint or returns
// a negative value if no value should be written. Sets the iterating variable
// to where decoding should continue, sets invalid_escape_index if it doesn't
// recognize the escape sequence, and sets error_message if an error occurred.
static int32_t decodeUnicodeEscaped(const Bytes& bytes, word* i,
                                    word* invalid_escape_index,
                                    const char** error_message) {
  switch (byte ch = bytes.byteAt((*i)++)) {
    // \x escapes
    case '\n':
      return -1;
    case '\\':
    case '\'':
    case '\"':
      return ch;
    case 'b':
      return '\b';
    case 't':
      return '\t';
    case 'n':
      return '\n';
    case 'r':
      return '\r';
    // BEL
    case 'a':
      return '\007';
    // FF
    case 'f':
      return '\014';
    // VT
    case 'v':
      return '\013';

    // \OOO (octal) escapes
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7': {
      word escaped = ch - '0';
      word octal_index = *i;
      word length = bytes.length();
      if (octal_index < length) {
        word ch2 = bytes.byteAt(octal_index);
        if ('0' <= ch2 && ch2 <= '7') {
          escaped = (escaped << 3) + ch2 - '0';
          if (++octal_index < length) {
            word ch3 = bytes.byteAt(octal_index);
            if ('0' <= ch3 && ch3 <= '7') {
              octal_index++;
              escaped = (escaped << 3) + ch3 - '0';
            }
          }
        }
      }
      *i = octal_index;
      return escaped;
    }

    // hex escapes
    // \xXX
    case 'x': {
      word escaped;
      if ((escaped = decodeHexEscaped(bytes, i, 2)) < 0) {
        *error_message = (escaped == -1 ? "truncated \\xXX escape"
                                        : "illegal Unicode character");
        return -1;
      }
      return escaped;
    }

    // \uXXXX
    case 'u': {
      word escaped;
      if ((escaped = decodeHexEscaped(bytes, i, 4)) < 0) {
        *error_message = (escaped == -1 ? "truncated \\uXXXX escape"
                                        : "illegal Unicode character");
        return -1;
      }
      return escaped;
    }

    // \UXXXXXXXX
    case 'U': {
      word escaped;
      if ((escaped = decodeHexEscaped(bytes, i, 8)) < 0) {
        *error_message = (escaped == -1 ? "truncated \\uXXXXXXXX escape"
                                        : "illegal Unicode character");
        return -1;
      }
      return escaped;
    }

    case 'N':
      // TODO(T39917408): Use the PyCapsule API to import UnicodeData
      UNIMPLEMENTED("Requires PyCapsule_Import");
      break;

    default: {
      *invalid_escape_index = *i - 1;
      return ch;
    }
  }
}

RawObject UnderCodecsModule::underUnicodeEscapeDecode(Thread* thread,
                                                      Frame* frame,
                                                      word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object bytes_obj(&scope, args.get(0));
  Object errors_obj(&scope, args.get(1));
  Object index_obj(&scope, args.get(2));
  Object output_obj(&scope, args.get(3));
  Runtime* runtime = thread->runtime();
  DCHECK(runtime->isInstanceOfBytes(*bytes_obj),
         "First arg to _unicode_escape_decode must be str");
  DCHECK(runtime->isInstanceOfStr(*errors_obj),
         "Second arg to _unicode_escape_decode must be str");
  DCHECK(runtime->isInstanceOfInt(*index_obj),
         "Third arg to _unicode_escape_decode must be int");
  DCHECK(output_obj.isStrArray(),
         "Fourth arg to _unicode_escape_decode must be _strarray");
  // TODO(T36619847): Bytes subclass handling
  Bytes bytes(&scope, *bytes_obj);
  Str errors(&scope, strUnderlying(thread, errors_obj));
  Int index(&scope, intUnderlying(thread, index_obj));
  StrArray dst(&scope, *output_obj);

  Tuple result(&scope, runtime->newTuple(4));
  word length = bytes.length();
  runtime->strArrayEnsureCapacity(thread, dst, length);
  word first_invalid_escape_index = -1;
  for (word i = index.asWord(); i < length;) {
    const char* message = nullptr;
    word start_pos = i;
    byte ch = bytes.byteAt(i++);
    if (ch != '\\') {
      if (ch <= kMaxASCII) {
        runtime->strArrayAddASCII(thread, dst, ch);
        continue;
      }
      Str temp(&scope, SmallStr::fromCodePoint(ch));
      runtime->strArrayAddStr(thread, dst, temp);
      continue;
    }
    if (i >= length) {
      message = "\\ at end of string";
    } else {
      word invalid_escape_index = -1;
      int32_t decoded =
          decodeUnicodeEscaped(bytes, &i, &invalid_escape_index, &message);
      if (invalid_escape_index != -1) {
        runtime->strArrayAddASCII(thread, dst, '\\');
        if (first_invalid_escape_index == -1) {
          first_invalid_escape_index = invalid_escape_index;
        }
      }
      if (decoded != -1) {
        if (decoded <= kMaxASCII) {
          runtime->strArrayAddASCII(thread, dst, decoded);
          continue;
        }
        Str temp(&scope, SmallStr::fromCodePoint(decoded));
        runtime->strArrayAddStr(thread, dst, temp);
        continue;
      }
    }
    if (message != nullptr) {
      SymbolId error_id = lookupSymbolForErrorHandler(errors);
      switch (error_id) {
        case SymbolId::kReplace: {
          Str temp(&scope, SmallStr::fromCodePoint(0xFFFD));
          runtime->strArrayAddStr(thread, dst, temp);
          break;
        }
        case SymbolId::kIgnore:
          break;
        default:
          result.atPut(0, runtime->newInt(start_pos));
          result.atPut(1, runtime->newInt(i));
          result.atPut(2, runtime->newStrFromCStr(message));
          result.atPut(3, runtime->newInt(first_invalid_escape_index));
          return *result;
      }
    }
  }
  result.atPut(0, runtime->strFromStrArray(dst));
  result.atPut(1, runtime->newInt(length));
  result.atPut(2, runtime->newStrFromCStr(""));
  result.atPut(3, runtime->newInt(first_invalid_escape_index));
  return *result;
}

RawObject UnderCodecsModule::underUtf8Encode(Thread* thread, Frame* frame,
                                             word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object data_obj(&scope, args.get(0));
  Object errors_obj(&scope, args.get(1));
  Object index_obj(&scope, args.get(2));
  Object output_obj(&scope, args.get(3));
  DCHECK(runtime->isInstanceOfStr(*data_obj),
         "First arg to _utf_8_encode must be str");
  DCHECK(runtime->isInstanceOfStr(*errors_obj),
         "Second arg to _utf_8_encode must be str");
  DCHECK(runtime->isInstanceOfInt(*index_obj),
         "Third arg to _utf_8_encode must be int");
  DCHECK(runtime->isInstanceOfByteArray(*output_obj),
         "Fourth arg to _utf_8_encode must be bytearray");
  Str data(&scope, strUnderlying(thread, data_obj));
  Str errors(&scope, strUnderlying(thread, errors_obj));
  Int index_int(&scope, intUnderlying(thread, index_obj));
  ByteArray output(&scope, *output_obj);

  Tuple result(&scope, runtime->newTuple(2));
  SymbolId error_symbol = lookupSymbolForErrorHandler(errors);
  word i = index_int.asWord();
  for (word byte_offset = data.offsetByCodePoints(0, i);
       byte_offset < data.length(); i++) {
    word num_bytes;
    int32_t codepoint = data.codePointAt(byte_offset, &num_bytes);
    byte_offset += num_bytes;
    if (!isSurrogate(codepoint)) {
      for (word j = byte_offset - num_bytes; j < byte_offset; j++) {
        byteArrayAdd(thread, runtime, output, data.charAt(j));
      }
    } else {
      switch (error_symbol) {
        case SymbolId::kIgnore:
          continue;
        case SymbolId::kReplace:
          byteArrayAdd(thread, runtime, output, kASCIIReplacement);
          continue;
        case SymbolId::kSurrogateescape:
          if (isEscapedLatin1Surrogate(codepoint)) {
            byteArrayAdd(thread, runtime, output,
                         codepoint - kLowSurrogateStart);
            continue;
          }
          break;
        default:
          break;
      }
      result.atPut(0, runtime->newInt(i));
      while (byte_offset < data.length() &&
             isSurrogate(data.codePointAt(byte_offset, &num_bytes))) {
        byte_offset += num_bytes;
        i++;
      }
      result.atPut(1, runtime->newInt(i + 1));
      return *result;
    }
  }
  result.atPut(0, byteArrayAsBytes(thread, runtime, output));
  result.atPut(1, runtime->newInt(i));
  return *result;
}

static void appendUtf16ToByteArray(Thread* thread, Runtime* runtime,
                                   const ByteArray& writer, int32_t codepoint,
                                   endian endianness) {
  if (endianness == endian::little) {
    byteArrayAdd(thread, runtime, writer, codepoint);
    byteArrayAdd(thread, runtime, writer, codepoint >> kBitsPerByte);
  } else {
    byteArrayAdd(thread, runtime, writer, codepoint >> kBitsPerByte);
    byteArrayAdd(thread, runtime, writer, codepoint);
  }
}

static int32_t highSurrogate(int32_t codepoint) {
  return kHighSurrogateStart - (0x10000 >> 10) + (codepoint >> 10);
}

static int32_t lowSurrogate(int32_t codepoint) {
  return kLowSurrogateStart + (codepoint & 0x3FF);
}

RawObject UnderCodecsModule::underUtf16Encode(Thread* thread, Frame* frame,
                                              word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object data_obj(&scope, args.get(0));
  Object errors_obj(&scope, args.get(1));
  Object index_obj(&scope, args.get(2));
  Object output_obj(&scope, args.get(3));
  Object byteorder_obj(&scope, args.get(4));
  DCHECK(runtime->isInstanceOfStr(*data_obj),
         "First arg to _utf_16_encode must be str");
  DCHECK(runtime->isInstanceOfStr(*errors_obj),
         "Second arg to _utf_16_encode must be str");
  DCHECK(runtime->isInstanceOfInt(*index_obj),
         "Third arg to _utf_16_encode must be int");
  DCHECK(runtime->isInstanceOfByteArray(*output_obj),
         "Fourth arg to _utf_16_encode must be bytearray");
  DCHECK(runtime->isInstanceOfInt(*byteorder_obj),
         "Fifth arg to _utf_16_encode must be int");
  Str data(&scope, strUnderlying(thread, data_obj));
  Str errors(&scope, strUnderlying(thread, errors_obj));
  Int index_int(&scope, intUnderlying(thread, index_obj));
  ByteArray output(&scope, *output_obj);
  Int byteorder_int(&scope, intUnderlying(thread, byteorder_obj));
  OptInt<int32_t> byteorder = byteorder_int.asInt<int32_t>();
  if (byteorder.error != CastError::None) {
    return thread->raiseWithFmt(LayoutId::kOverflowError,
                                "Python int too large to convert to C int");
  }

  Tuple result(&scope, runtime->newTuple(2));
  SymbolId error_id = lookupSymbolForErrorHandler(errors);
  word i = index_int.asWord();
  for (word byte_offset = data.offsetByCodePoints(0, i);
       byte_offset < data.length(); i++) {
    endian endianness = byteorder.value <= 0 ? endian::little : endian::big;
    word num_bytes;
    int32_t codepoint = data.codePointAt(byte_offset, &num_bytes);
    byte_offset += num_bytes;
    if (!isSurrogate(codepoint)) {
      if (codepoint < kHighSurrogateStart) {
        appendUtf16ToByteArray(thread, runtime, output, codepoint, endianness);
      } else {
        appendUtf16ToByteArray(thread, runtime, output,
                               highSurrogate(codepoint), endianness);
        appendUtf16ToByteArray(thread, runtime, output, lowSurrogate(codepoint),
                               endianness);
      }
    } else {
      switch (error_id) {
        case SymbolId::kIgnore:
          continue;
        case SymbolId::kReplace:
          appendUtf16ToByteArray(thread, runtime, output, kASCIIReplacement,
                                 endianness);
          continue;
        case SymbolId::kSurrogateescape:
          if (isEscapedLatin1Surrogate(codepoint)) {
            appendUtf16ToByteArray(thread, runtime, output,
                                   codepoint - kLowSurrogateStart, endianness);
            continue;
          }
          break;
        default:
          break;
      }
      result.atPut(0, runtime->newInt(i));
      while (byte_offset < data.length() &&
             isSurrogate(data.codePointAt(byte_offset, &num_bytes))) {
        byte_offset += num_bytes;
        i++;
      }
      result.atPut(1, runtime->newInt(i + 1));
      return *result;
    }
  }
  result.atPut(0, byteArrayAsBytes(thread, runtime, output));
  result.atPut(1, runtime->newInt(i));
  return *result;
}

static void appendUtf32ToByteArray(Thread* thread, Runtime* runtime,
                                   const ByteArray& writer, int32_t codepoint,
                                   endian endianness) {
  if (endianness == endian::little) {
    byteArrayAdd(thread, runtime, writer, codepoint);
    byteArrayAdd(thread, runtime, writer, codepoint >> (kBitsPerByte));
    byteArrayAdd(thread, runtime, writer, codepoint >> (kBitsPerByte * 2));
    byteArrayAdd(thread, runtime, writer, codepoint >> (kBitsPerByte * 3));
  } else {
    byteArrayAdd(thread, runtime, writer, codepoint >> (kBitsPerByte * 3));
    byteArrayAdd(thread, runtime, writer, codepoint >> (kBitsPerByte * 2));
    byteArrayAdd(thread, runtime, writer, codepoint >> (kBitsPerByte));
    byteArrayAdd(thread, runtime, writer, codepoint);
  }
}

RawObject UnderCodecsModule::underUtf32Encode(Thread* thread, Frame* frame,
                                              word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object data_obj(&scope, args.get(0));
  Object errors_obj(&scope, args.get(1));
  Object index_obj(&scope, args.get(2));
  Object output_obj(&scope, args.get(3));
  Object byteorder_obj(&scope, args.get(4));
  DCHECK(runtime->isInstanceOfStr(*data_obj),
         "First arg to _utf_32_encode must be str");
  DCHECK(runtime->isInstanceOfStr(*errors_obj),
         "Second arg to _utf_32_encode must be str");
  DCHECK(runtime->isInstanceOfInt(*index_obj),
         "Third arg to _utf_32_encode must be int");
  DCHECK(runtime->isInstanceOfByteArray(*output_obj),
         "Fourth arg to _utf_32_encode must be bytearray");
  DCHECK(runtime->isInstanceOfInt(*byteorder_obj),
         "Fifth arg to _utf_32_encode must be int");
  Str data(&scope, strUnderlying(thread, data_obj));
  Str errors(&scope, strUnderlying(thread, errors_obj));
  Int index_int(&scope, intUnderlying(thread, index_obj));
  ByteArray output(&scope, *output_obj);
  Int byteorder_int(&scope, intUnderlying(thread, byteorder_obj));
  OptInt<int32_t> byteorder = byteorder_int.asInt<int32_t>();
  if (byteorder.error != CastError::None) {
    return thread->raiseWithFmt(LayoutId::kOverflowError,
                                "Python int too large to convert to C int");
  }

  Tuple result(&scope, runtime->newTuple(2));
  SymbolId error_id = lookupSymbolForErrorHandler(errors);
  word i = index_int.asWord();
  for (word byte_offset = data.offsetByCodePoints(0, i);
       byte_offset < data.length(); i++) {
    endian endianness = byteorder.value <= 0 ? endian::little : endian::big;
    word num_bytes;
    int32_t codepoint = data.codePointAt(byte_offset, &num_bytes);
    byte_offset += num_bytes;
    if (!isSurrogate(codepoint)) {
      appendUtf32ToByteArray(thread, runtime, output, codepoint, endianness);
    } else {
      switch (error_id) {
        case SymbolId::kIgnore:
          continue;
        case SymbolId::kReplace:
          appendUtf32ToByteArray(thread, runtime, output, kASCIIReplacement,
                                 endianness);
          continue;
        case SymbolId::kSurrogateescape:
          if (isEscapedLatin1Surrogate(codepoint)) {
            appendUtf32ToByteArray(thread, runtime, output,
                                   codepoint - kLowSurrogateStart, endianness);
            continue;
          }
          break;
        default:
          break;
      }
      result.atPut(0, runtime->newInt(i));
      while (byte_offset < data.length() &&
             isSurrogate(data.codePointAt(byte_offset, &num_bytes))) {
        byte_offset += num_bytes;
        i++;
      }
      result.atPut(1, runtime->newInt(i + 1));
      return *result;
    }
  }
  result.atPut(0, byteArrayAsBytes(thread, runtime, output));
  result.atPut(1, runtime->newInt(i));
  return *result;
}

// Takes a ByteArray and a Str object, and appends each byte in the Str to the
// ByteArray one by one
RawObject UnderCodecsModule::underByteArrayStringAppend(Thread* thread,
                                                        Frame* frame,
                                                        word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  ByteArray dst(&scope, args.get(0));
  Str data(&scope, args.get(1));
  for (word i = 0; i < data.length(); ++i) {
    byteArrayAdd(thread, thread->runtime(), dst, data.charAt(i));
  }
  return NoneType::object();
}

}  // namespace python
