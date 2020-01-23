#include "under-codecs-module.h"

#include "bytearray-builtins.h"
#include "bytes-builtins.h"
#include "frame.h"
#include "frozen-modules.h"
#include "int-builtins.h"
#include "modules.h"
#include "runtime.h"
#include "str-builtins.h"

extern "C" unsigned char _PyLong_DigitValue[];  // from Include/longobject.h
extern "C" unsigned int _Py_ctype_table[];      // from Include/pyctype.h

namespace py {

const BuiltinFunction UnderCodecsModule::kBuiltinFunctions[] = {
    {SymbolId::kUnderAsciiDecode, underAsciiDecode},
    {SymbolId::kUnderAsciiEncode, underAsciiEncode},
    {SymbolId::kUnderBytearrayStringAppend, underBytearrayStringAppend},
    {SymbolId::kUnderEscapeDecode, underEscapeDecode},
    {SymbolId::kUnderLatin1Decode, underLatin1Decode},
    {SymbolId::kUnderLatin1Encode, underLatin1Encode},
    {SymbolId::kUnderUnicodeEscapeDecode, underUnicodeEscapeDecode},
    {SymbolId::kUnderUtf16Encode, underUtf16Encode},
    {SymbolId::kUnderUtf32Encode, underUtf32Encode},
    {SymbolId::kUnderUtf8Decode, underUtf8Decode},
    {SymbolId::kUnderUtf8Encode, underUtf8Encode},
    {SymbolId::kSentinelId, nullptr},
};

void UnderCodecsModule::initialize(Thread* thread, const Module& module) {
  moduleAddBuiltinFunctions(thread, module, kBuiltinFunctions);
  executeFrozenModule(thread, kUnderCodecsModuleData, module);
}

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

RawObject UnderCodecsModule::underAsciiDecode(Thread* thread, Frame* frame,
                                              word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Bytes bytes(&scope, bytesUnderlying(args.get(0)));
  Str errors(&scope, strUnderlying(args.get(1)));
  word index = intUnderlying(args.get(2)).asWord();
  StrArray dst(&scope, args.get(3));

  Tuple result(&scope, runtime->newTuple(2));
  word length = bytes.length();
  runtime->strArrayEnsureCapacity(thread, dst, length);
  word outpos = asciiDecode(thread, dst, bytes, index);
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
  Object output_obj(&scope, args.get(3));
  DCHECK(runtime->isInstanceOfByteArray(*output_obj),
         "Fourth arg to _ascii_encode must be bytearray");
  Str data(&scope, strUnderlying(args.get(0)));
  Str errors(&scope, strUnderlying(args.get(1)));
  word i = intUnderlying(args.get(2)).asWord();
  ByteArray output(&scope, *output_obj);

  Tuple result(&scope, runtime->newTuple(2));
  SymbolId error_symbol = lookupSymbolForErrorHandler(errors);
  // TODO(T43252439): Optimize this by first checking whether the entire string
  // is ASCII, and just memcpy into a string if so
  for (word byte_offset = data.offsetByCodePoints(0, i);
       byte_offset < data.charLength(); i++) {
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
      while (byte_offset < data.charLength() &&
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
  Runtime* runtime = thread->runtime();
  if (runtime->isInstanceOfStr(*bytes_obj)) {
    // TODO(T44739505): Make sure we can decode a str
    UNIMPLEMENTED("_codecs.escape_decode with a str");
  }
  DCHECK(runtime->isInstanceOfStr(args.get(2)),
         "Third arg to _escape_decode must be str");
  Bytes bytes(&scope, bytesUnderlying(*bytes_obj));
  Str errors(&scope, strUnderlying(args.get(1)));

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
  Bytes bytes(&scope, bytesUnderlying(args.get(0)));
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
        str = Str::cast(SmallStr::fromCodePoint(code_point));
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
  Object output_obj(&scope, args.get(3));
  DCHECK(runtime->isInstanceOfByteArray(*output_obj),
         "Fourth arg to _latin_1_encode must be bytearray");
  Str data(&scope, strUnderlying(args.get(0)));
  Str errors(&scope, strUnderlying(args.get(1)));
  word i = intUnderlying(args.get(2)).asWord();
  ByteArray output(&scope, *output_obj);

  Tuple result(&scope, runtime->newTuple(2));
  SymbolId error_symbol = lookupSymbolForErrorHandler(errors);
  for (word byte_offset = data.offsetByCodePoints(0, i);
       byte_offset < data.charLength(); i++) {
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
      while (byte_offset < data.charLength() &&
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
  Runtime* runtime = thread->runtime();
  Bytes bytes(&scope, bytesUnderlying(args.get(0)));
  Str errors(&scope, strUnderlying(args.get(1)));
  word index = intUnderlying(args.get(2)).asWord();
  StrArray dst(&scope, args.get(3));

  Tuple result(&scope, runtime->newTuple(4));
  word length = bytes.length();
  runtime->strArrayEnsureCapacity(thread, dst, length);
  word first_invalid_escape_index = -1;
  for (word i = index; i < length;) {
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

static bool isContinuationByte(byte ch) { return kMaxASCII < ch && ch < 0xC0; }

enum Utf8DecoderResult {
  k1Byte = 1,
  k2Byte = 2,
  k3Byte = 3,
  k4Byte = 4,
  kInvalidStart = 0,
  kInvalidContinuation1 = -1,
  kInvalidContinuation2 = -2,
  kInvalidContinuation3 = -3,
  kUnexpectedEndOfData = -4,
};

// This functionality is taken mostly from CPython:
//   Objects/stringlib/codecs.h::utf8_decode
// This does error checking to ensure well-formedness of the passed in UTF-8
// bytes, and returns the number of bytes of the codepoint at `index` as a
// Utf8DecoderResult enum value.
// Since this is supposed to work as an incremental decoder as well, this
// function returns specific values for errors to determine whether they could
// be caused by incremental decoding, or if they would be an error no matter
// what other bytes might be streamed in later.
static Utf8DecoderResult isValidUtf8Codepoint(const Bytes& bytes, word index) {
  word length = bytes.length();
  byte ch = bytes.byteAt(index);
  if (ch <= kMaxASCII) {
    return k1Byte;
  }
  if (ch < 0xE0) {
    // \xC2\x80-\xDF\xBF -- 0080-07FF
    if (ch < 0xC2) {
      // invalid sequence
      // \x80-\xBF -- continuation byte
      // \xC0-\xC1 -- fake 0000-007F
      return kInvalidStart;
    }
    if (index + 1 >= length) {
      return kUnexpectedEndOfData;
    }
    if (!isContinuationByte(bytes.byteAt(index + 1))) {
      return kInvalidContinuation1;
    }
    return k2Byte;
  }
  if (ch < 0xF0) {
    // \xE0\xA0\x80-\xEF\xBF\xBF -- 0800-FFFF
    if (index + 2 >= length) {
      if (index + 1 >= length) {
        return kUnexpectedEndOfData;
      }
      byte ch2 = bytes.byteAt(index + 1);
      if (!isContinuationByte(ch2) || (ch2 < 0xA0 ? ch == 0xE0 : ch == 0xED)) {
        return kInvalidContinuation1;
      }
      return kUnexpectedEndOfData;
    }
    byte ch2 = bytes.byteAt(index + 1);
    if (!isContinuationByte(ch2)) {
      return kInvalidContinuation1;
    }
    if (ch == 0xE0) {
      if (ch2 < 0xA0) {
        // invalid sequence
        // \xE0\x80\x80-\xE0\x9F\xBF -- fake 0000-0800
        return kInvalidContinuation1;
      }
    } else if (ch == 0xED && ch2 >= 0xA0) {
      // Decoding UTF-8 sequences in range \xED\xA0\x80-\xED\xBF\xBF
      // will result in surrogates in range D800-DFFF. Surrogates are
      // not valid UTF-8 so they are rejected.
      // See http://www.unicode.org/versions/Unicode5.2.0/ch03.pdf
      // (table 3-7) and http://www.rfc-editor.org/rfc/rfc3629.txt
      return kInvalidContinuation1;
    }
    if (!isContinuationByte(bytes.byteAt(index + 2))) {
      return kInvalidContinuation2;
    }
    return k3Byte;
  }
  if (ch < 0xF5) {
    // \xF0\x90\x80\x80-\xF4\x8F\xBF\xBF -- 10000-10FFFF
    if (index + 3 >= length) {
      if (index + 1 >= length) {
        return kUnexpectedEndOfData;
      }
      byte ch2 = bytes.byteAt(index + 1);
      if (!isContinuationByte(ch2) || (ch2 < 0x90 ? ch == 0xF0 : ch == 0xF4)) {
        return kInvalidContinuation1;
      }
      if (index + 2 >= length) {
        return kUnexpectedEndOfData;
      }
      if (!isContinuationByte(bytes.byteAt(index + 2))) {
        return kInvalidContinuation2;
      }
      return kUnexpectedEndOfData;
    }
    byte ch2 = bytes.byteAt(index + 1);
    if (!isContinuationByte(ch2)) {
      return kInvalidContinuation1;
    }
    if (ch == 0xF0) {
      if (ch2 < 0x90) {
        // invalid sequence
        // \xF0\x80\x80\x80-\xF0\x8F\xBF\xBF -- fake 0000-FFFF
        return kInvalidContinuation1;
      }
    } else if (ch == 0xF4 && ch2 >= 0x90) {
      // invalid sequence
      // \xF4\x90\x80\80- -- 110000- overflow
      return kInvalidContinuation1;
    }
    if (!isContinuationByte(bytes.byteAt(index + 2))) {
      return kInvalidContinuation2;
    }
    if (!isContinuationByte(bytes.byteAt(index + 3))) {
      return kInvalidContinuation3;
    }
    return k4Byte;
  }
  return kInvalidStart;
}

RawObject UnderCodecsModule::underUtf8Decode(Thread* thread, Frame* frame,
                                             word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object final_obj(&scope, args.get(4));
  DCHECK(final_obj.isBool(), "Fifth arg to _utf_8_decode must be bool");
  // TODO(T45849551): Handle any bytes-like object
  Bytes bytes(&scope, bytesUnderlying(args.get(0)));
  Str errors(&scope, strUnderlying(args.get(1)));
  word index = intUnderlying(args.get(2)).asWord();
  StrArray dst(&scope, args.get(3));

  Tuple result(&scope, runtime->newTuple(3));
  word length = bytes.length();
  runtime->strArrayEnsureCapacity(thread, dst, length);
  word i = asciiDecode(thread, dst, bytes, index);
  if (i == length) {
    result.atPut(0, runtime->strFromStrArray(dst));
    result.atPut(1, runtime->newInt(length));
    result.atPut(2, runtime->newStrFromCStr(""));
    return *result;
  }

  SymbolId error_id = lookupSymbolForErrorHandler(errors);
  bool is_final = Bool::cast(*final_obj).value();
  while (i < length) {
    // TODO(T41032331): Scan for non-ASCII characters by words instead of chars
    Utf8DecoderResult validator_result = isValidUtf8Codepoint(bytes, i);
    if (validator_result >= k1Byte) {
      byte codepoint[4] = {0};
      for (int codeunit = 0; codeunit + 1 <= validator_result; codeunit++) {
        codepoint[codeunit] = bytes.byteAt(i + codeunit);
      }
      i += validator_result;
      Str temp(&scope,
               runtime->newStrWithAll(View<byte>{codepoint, validator_result}));
      runtime->strArrayAddStr(thread, dst, temp);
      continue;
    }
    if (validator_result != kInvalidStart && !is_final) {
      break;
    }
    word error_end = i;
    const char* error_message = nullptr;
    switch (validator_result) {
      case kInvalidStart:
        error_end += 1;
        error_message = "invalid start byte";
        break;
      case kInvalidContinuation1:
      case kInvalidContinuation2:
      case kInvalidContinuation3:
        error_end -= validator_result;
        error_message = "invalid continuation byte";
        break;
      case kUnexpectedEndOfData:
        error_end = length;
        error_message = "unexpected end of data";
        break;
      default:
        UNREACHABLE(
            "valid utf-8 codepoints should have been decoded by this point");
    }
    switch (error_id) {
      case SymbolId::kReplace: {
        Str temp(&scope, SmallStr::fromCodePoint(kReplacementCharacter));
        runtime->strArrayAddStr(thread, dst, temp);
        i = error_end;
        break;
      }
      case SymbolId::kSurrogateescape: {
        for (; i < error_end; ++i) {
          Str temp(&scope, SmallStr::fromCodePoint(kLowSurrogateStart +
                                                   bytes.byteAt(i)));
          runtime->strArrayAddStr(thread, dst, temp);
        }
        break;
      }
      case SymbolId::kIgnore:
        i = error_end;
        break;
      default:
        result.atPut(0, runtime->newInt(i));
        result.atPut(1, runtime->newInt(error_end));
        result.atPut(2, runtime->newStrFromCStr(error_message));
        return *result;
    }
  }
  result.atPut(0, runtime->strFromStrArray(dst));
  result.atPut(1, runtime->newInt(i));
  result.atPut(2, Str::empty());
  return *result;
}

RawObject UnderCodecsModule::underUtf8Encode(Thread* thread, Frame* frame,
                                             word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object output_obj(&scope, args.get(3));
  DCHECK(runtime->isInstanceOfByteArray(*output_obj),
         "Fourth arg to _utf_8_encode must be bytearray");
  Str data(&scope, strUnderlying(args.get(0)));
  Str errors(&scope, strUnderlying(args.get(1)));
  word index = intUnderlying(args.get(2)).asWord();
  ByteArray output(&scope, *output_obj);

  Tuple result(&scope, runtime->newTuple(2));
  SymbolId error_symbol = lookupSymbolForErrorHandler(errors);
  for (word byte_offset = data.offsetByCodePoints(0, index);
       byte_offset < data.charLength(); index++) {
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
      result.atPut(0, runtime->newInt(index));
      while (byte_offset < data.charLength() &&
             isSurrogate(data.codePointAt(byte_offset, &num_bytes))) {
        byte_offset += num_bytes;
        index++;
      }
      result.atPut(1, runtime->newInt(index + 1));
      return *result;
    }
  }
  result.atPut(0, byteArrayAsBytes(thread, runtime, output));
  result.atPut(1, runtime->newInt(index));
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
  Object output_obj(&scope, args.get(3));
  DCHECK(runtime->isInstanceOfByteArray(*output_obj),
         "Fourth arg to _utf_16_encode must be bytearray");
  Str data(&scope, strUnderlying(args.get(0)));
  Str errors(&scope, strUnderlying(args.get(1)));
  word index = intUnderlying(args.get(2)).asWord();
  ByteArray output(&scope, *output_obj);
  OptInt<int32_t> byteorder = intUnderlying(args.get(4)).asInt<int32_t>();
  if (byteorder.error != CastError::None) {
    return thread->raiseWithFmt(LayoutId::kOverflowError,
                                "Python int too large to convert to C int");
  }

  Tuple result(&scope, runtime->newTuple(2));
  SymbolId error_id = lookupSymbolForErrorHandler(errors);
  for (word byte_offset = data.offsetByCodePoints(0, index);
       byte_offset < data.charLength(); index++) {
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
      result.atPut(0, runtime->newInt(index));
      while (byte_offset < data.charLength() &&
             isSurrogate(data.codePointAt(byte_offset, &num_bytes))) {
        byte_offset += num_bytes;
        index++;
      }
      result.atPut(1, runtime->newInt(index + 1));
      return *result;
    }
  }
  result.atPut(0, byteArrayAsBytes(thread, runtime, output));
  result.atPut(1, runtime->newInt(index));
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
  Object output_obj(&scope, args.get(3));
  DCHECK(runtime->isInstanceOfByteArray(*output_obj),
         "Fourth arg to _utf_32_encode must be bytearray");
  Str data(&scope, strUnderlying(args.get(0)));
  Str errors(&scope, strUnderlying(args.get(1)));
  word index = intUnderlying(args.get(2)).asWord();
  ByteArray output(&scope, *output_obj);
  OptInt<int32_t> byteorder = intUnderlying(args.get(4)).asInt<int32_t>();
  if (byteorder.error != CastError::None) {
    return thread->raiseWithFmt(LayoutId::kOverflowError,
                                "Python int too large to convert to C int");
  }

  Tuple result(&scope, runtime->newTuple(2));
  SymbolId error_id = lookupSymbolForErrorHandler(errors);
  for (word byte_offset = data.offsetByCodePoints(0, index);
       byte_offset < data.charLength(); index++) {
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
      result.atPut(0, runtime->newInt(index));
      while (byte_offset < data.charLength() &&
             isSurrogate(data.codePointAt(byte_offset, &num_bytes))) {
        byte_offset += num_bytes;
        index++;
      }
      result.atPut(1, runtime->newInt(index + 1));
      return *result;
    }
  }
  result.atPut(0, byteArrayAsBytes(thread, runtime, output));
  result.atPut(1, runtime->newInt(index));
  return *result;
}

// Takes a ByteArray and a Str object, and appends each byte in the Str to the
// ByteArray one by one
RawObject UnderCodecsModule::underBytearrayStringAppend(Thread* thread,
                                                        Frame* frame,
                                                        word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  ByteArray dst(&scope, args.get(0));
  Str data(&scope, args.get(1));
  for (word i = 0; i < data.charLength(); ++i) {
    byteArrayAdd(thread, thread->runtime(), dst, data.charAt(i));
  }
  return NoneType::object();
}

}  // namespace py
