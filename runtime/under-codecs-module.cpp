#include "builtins.h"
#include "bytearray-builtins.h"
#include "bytes-builtins.h"
#include "byteslike.h"
#include "frame.h"
#include "int-builtins.h"
#include "modules.h"
#include "runtime.h"
#include "str-builtins.h"
#include "unicode-db.h"
#include "unicode.h"
#include "utils.h"

namespace py {

const char kASCIIReplacement = '?';

static SymbolId lookupSymbolForErrorHandler(const Str& error) {
  if (error.equalsCStr("strict")) {
    return ID(strict);
  }
  if (error.equalsCStr("ignore")) {
    return ID(ignore);
  }
  if (error.equalsCStr("replace")) {
    return ID(replace);
  }
  if (error.equalsCStr("surrogateescape")) {
    return ID(surrogateescape);
  }
  if (error.equalsCStr("surrogatepass")) {
    return ID(surrogatepass);
  }
  return SymbolId::kInvalid;
}

static int asciiDecode(Thread* thread, const StrArray& dst,
                       const Byteslike& src, word start, word end) {
  // TODO(T41032331): Implement a fastpass to read longs instead of chars
  Runtime* runtime = thread->runtime();
  for (word i = start; i < end; i++) {
    byte ch = src.byteAt(i);
    if (ch > kMaxASCII) {
      return i;
    }
    runtime->strArrayAddASCII(thread, dst, ch);
  }
  return end;
}

RawObject FUNC(_codecs, _ascii_decode)(Thread* thread, Arguments args) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Object data(&scope, args.get(0));
  Str errors(&scope, strUnderlying(args.get(1)));
  word index = intUnderlying(args.get(2)).asWord();
  StrArray dst(&scope, args.get(3));

  Byteslike bytes(&scope, thread, *data);
  word length = bytes.length();
  runtime->strArrayEnsureCapacity(thread, dst, length);
  word outpos = asciiDecode(thread, dst, bytes, index, length);
  if (outpos == length) {
    Object dst_obj(&scope, runtime->strFromStrArray(dst));
    Object length_obj(&scope, runtime->newInt(length));
    return runtime->newTupleWith2(dst_obj, length_obj);
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
      case ID(replace): {
        Str temp(&scope, SmallStr::fromCodePoint(0xFFFD));
        runtime->strArrayAddStr(thread, dst, temp);
        ++outpos;
        break;
      }
      case ID(surrogateescape): {
        Str temp(&scope,
                 SmallStr::fromCodePoint(Unicode::kLowSurrogateStart + c));
        runtime->strArrayAddStr(thread, dst, temp);
        ++outpos;
        break;
      }
      case ID(ignore):
        ++outpos;
        break;
      default: {
        Object outpos1(&scope, runtime->newIntFromUnsigned(outpos));
        Object outpos2(&scope, runtime->newIntFromUnsigned(outpos + 1));
        return runtime->newTupleWith2(outpos1, outpos2);
      }
    }
  }
  Object dst_obj(&scope, runtime->strFromStrArray(dst));
  Object length_obj(&scope, runtime->newInt(length));
  return runtime->newTupleWith2(dst_obj, length_obj);
}

// CPython encodes latin1 codepoints into the low-surrogate range, and is able
// to recover the original codepoints from those decodable surrogate points.
static bool isEscapedLatin1Surrogate(int32_t codepoint) {
  return (Unicode::kLowSurrogateStart + kMaxASCII) < codepoint &&
         codepoint <= (Unicode::kLowSurrogateStart + kMaxByte);
}

RawObject FUNC(_codecs, _ascii_encode)(Thread* thread, Arguments args) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Object output_obj(&scope, args.get(3));
  DCHECK(runtime->isInstanceOfBytearray(*output_obj),
         "Fourth arg to _ascii_encode must be bytearray");
  Str data(&scope, strUnderlying(args.get(0)));
  Str errors(&scope, strUnderlying(args.get(1)));
  word i = intUnderlying(args.get(2)).asWord();
  Bytearray output(&scope, *output_obj);

  SymbolId error_symbol = lookupSymbolForErrorHandler(errors);
  // TODO(T43252439): Optimize this by first checking whether the entire string
  // is ASCII, and just memcpy into a string if so
  for (word byte_offset = thread->strOffset(data, i);
       byte_offset < data.length(); i++) {
    word num_bytes;
    int32_t codepoint = data.codePointAt(byte_offset, &num_bytes);
    byte_offset += num_bytes;
    if (codepoint <= kMaxASCII) {
      bytearrayAdd(thread, runtime, output, codepoint);
    } else {
      switch (error_symbol) {
        case ID(ignore):
          continue;
        case ID(replace):
          bytearrayAdd(thread, runtime, output, kASCIIReplacement);
          continue;
        case ID(surrogateescape):
          if (isEscapedLatin1Surrogate(codepoint)) {
            bytearrayAdd(thread, runtime, output,
                         codepoint - Unicode::kLowSurrogateStart);
            continue;
          }
          break;
        default:
          break;
      }
      Object outpos1(&scope, runtime->newInt(i));
      while (byte_offset < data.length() &&
             data.codePointAt(byte_offset, &num_bytes) > kMaxASCII) {
        byte_offset += num_bytes;
        i++;
      }
      Object outpos2(&scope, runtime->newInt(i + 1));
      return runtime->newTupleWith2(outpos1, outpos2);
    }
  }
  Object output_bytes(&scope, bytearrayAsBytes(thread, output));
  Object outpos_obj(&scope, runtime->newInt(i));
  return runtime->newTupleWith2(output_bytes, outpos_obj);
}

// Decodes a sequence of unicode encoded bytes into a codepoint, returns
// -1 if no value should be written, and -2 if an error occurred. Sets the
// iterating variable to where decoding should continue, and sets
// invalid_escape_index if it doesn't recognize the escape sequence.
static int32_t decodeEscaped(const Byteslike& bytes, word* i,
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

RawObject FUNC(_codecs, _escape_decode)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object bytes_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (runtime->isInstanceOfStr(*bytes_obj)) {
    // TODO(T44739505): Make sure we can decode a str
    UNIMPLEMENTED("_codecs.escape_decode with a str");
  }
  DCHECK(runtime->isInstanceOfStr(args.get(2)),
         "Third arg to _escape_decode must be str");
  Byteslike bytes(&scope, thread, *bytes_obj);
  Str errors(&scope, strUnderlying(args.get(1)));

  Bytearray dst(&scope, runtime->newBytearray());
  word length = bytes.length();
  runtime->bytearrayEnsureCapacity(thread, dst, length);
  word first_invalid_escape_index = -1;
  for (word i = 0; i < length;) {
    byte ch = bytes.byteAt(i++);
    if (ch != '\\') {
      // TODO(T45134397): Support the recode_encoding parameter
      if (ch <= kMaxASCII) {
        bytearrayAdd(thread, runtime, dst, ch);
        continue;
      }
      Str temp(&scope, SmallStr::fromCodePoint(ch));
      bytearrayAdd(thread, runtime, dst, temp.byteAt(0));
      bytearrayAdd(thread, runtime, dst, temp.byteAt(1));
      continue;
    }
    if (i >= length) {
      return runtime->newStrFromCStr("Trailing \\ in string");
    }
    word invalid_escape_index = -1;
    int32_t decoded = decodeEscaped(bytes, &i, &invalid_escape_index);
    if (invalid_escape_index != -1) {
      bytearrayAdd(thread, runtime, dst, '\\');
      if (first_invalid_escape_index == -1) {
        first_invalid_escape_index = invalid_escape_index;
      }
    }
    if (decoded >= 0) {
      bytearrayAdd(thread, runtime, dst, decoded);
      continue;
    }
    if (decoded == -1) {
      continue;
    }
    SymbolId error_id = lookupSymbolForErrorHandler(errors);
    switch (error_id) {
      case ID(strict):
        return runtime->newStrFromFmt("invalid \\x escape at position %d",
                                      i - 2);
      case ID(replace): {
        bytearrayAdd(thread, runtime, dst, '?');
        break;
      }
      case ID(ignore):
        break;
      default:
        return runtime->newStrFromFmt(
            "decoding error; unknown error handling code: %S", &errors);
    }
    if (i < length && Byte::isHexDigit(bytes.byteAt(i))) {
      i++;
    }
  }
  Object dst_obj(&scope, bytearrayAsBytes(thread, dst));
  Object length_obj(&scope, runtime->newInt(length));
  Object escape_obj(&scope, runtime->newInt(first_invalid_escape_index));
  return runtime->newTupleWith3(dst_obj, length_obj, escape_obj);
}

RawObject FUNC(_codecs, _latin_1_decode)(Thread* thread, Arguments args) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Object data(&scope, args.get(0));
  StrArray array(&scope, runtime->newStrArray());
  word length;
  Byteslike bytes(&scope, thread, *data);
  length = bytes.length();
  runtime->strArrayEnsureCapacity(thread, array, length);
  // First, try a quick ASCII decoding
  word num_bytes = asciiDecode(thread, array, bytes, 0, length);
  if (num_bytes != length) {
    // A non-ASCII character was found; switch to a Latin-1 decoding for the
    // remainder of the input sequence
    for (word i = num_bytes; i < length; ++i) {
      byte code_point = bytes.byteAt(i);
      if (code_point <= kMaxASCII) {
        runtime->strArrayAddASCII(thread, array, code_point);
      } else {
        runtime->strArrayAddCodePoint(thread, array, code_point);
      }
    }
  }
  Object array_str(&scope, runtime->strFromStrArray(array));
  Object length_obj(&scope, runtime->newInt(length));
  return runtime->newTupleWith2(array_str, length_obj);
}

RawObject FUNC(_codecs, _latin_1_encode)(Thread* thread, Arguments args) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Object output_obj(&scope, args.get(3));
  DCHECK(runtime->isInstanceOfBytearray(*output_obj),
         "Fourth arg to _latin_1_encode must be bytearray");
  Str data(&scope, strUnderlying(args.get(0)));
  Str errors(&scope, strUnderlying(args.get(1)));
  word i = intUnderlying(args.get(2)).asWord();
  Bytearray output(&scope, *output_obj);

  SymbolId error_symbol = lookupSymbolForErrorHandler(errors);
  for (word byte_offset = thread->strOffset(data, i);
       byte_offset < data.length(); i++) {
    word num_bytes;
    int32_t codepoint = data.codePointAt(byte_offset, &num_bytes);
    byte_offset += num_bytes;
    if (codepoint <= kMaxByte) {
      bytearrayAdd(thread, runtime, output, codepoint);
    } else {
      switch (error_symbol) {
        case ID(ignore):
          continue;
        case ID(replace):
          bytearrayAdd(thread, runtime, output, kASCIIReplacement);
          continue;
        case ID(surrogateescape):
          if (isEscapedLatin1Surrogate(codepoint)) {
            bytearrayAdd(thread, runtime, output,
                         codepoint - Unicode::kLowSurrogateStart);
            continue;
          }
          break;
        default:
          break;
      }
      Object outpos1(&scope, runtime->newInt(i));
      while (byte_offset < data.length() &&
             data.codePointAt(byte_offset, &num_bytes) > kMaxByte) {
        byte_offset += num_bytes;
        i++;
      }
      Object outpos2(&scope, runtime->newInt(i + 1));
      return runtime->newTupleWith2(outpos1, outpos2);
    }
  }
  Object output_bytes(&scope, bytearrayAsBytes(thread, output));
  Object outpos(&scope, runtime->newInt(i));
  return runtime->newTupleWith2(output_bytes, outpos);
}

// Decodes a sequence of hexadecimal encoded bytes into a codepoint or returns
// a negative value if the value could not be decoded. Sets the start variable
// to where decoding should continue.
static int32_t decodeHexEscaped(const Byteslike& bytes, word* start,
                                word count) {
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
static int32_t decodeUnicodeEscaped(const Byteslike& bytes, word* i,
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

    // \N{name}
    case 'N': {
      *error_message = "malformed \\N character escape";
      word length = bytes.length();
      if (*i >= length || bytes.byteAt(*i) != '{') {
        return -1;
      }
      word start = ++(*i);
      while (*i < length && bytes.byteAt(*i) != '}') {
        *i += 1;
      }
      word size = *i - start;
      if (size == 0 || *i == length) {
        return -1;
      }
      *i += 1;
      *error_message = "unknown Unicode character name";

      unique_c_ptr<byte> buffer(reinterpret_cast<byte*>(std::malloc(size)));
      bytes.copyToStartAt(buffer.get(), size, start);
      return codePointFromName(buffer.get(), size);
    }

    default: {
      *invalid_escape_index = *i - 1;
      return ch;
    }
  }
}

RawObject FUNC(_codecs, _unicode_escape_decode)(Thread* thread,
                                                Arguments args) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object data(&scope, args.get(0));
  Str errors(&scope, strUnderlying(args.get(1)));
  word index = intUnderlying(args.get(2)).asWord();
  StrArray dst(&scope, args.get(3));

  Byteslike bytes(&scope, thread, *data);
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
        case ID(replace): {
          Str temp(&scope, SmallStr::fromCodePoint(0xFFFD));
          runtime->strArrayAddStr(thread, dst, temp);
          break;
        }
        case ID(ignore):
          break;
        default: {
          Object start_pos_obj(&scope, runtime->newInt(start_pos));
          Object outpos_obj(&scope, runtime->newInt(i));
          Object message_obj(&scope, runtime->newStrFromCStr(message));
          Object escape_obj(&scope,
                            runtime->newInt(first_invalid_escape_index));
          return runtime->newTupleWith4(start_pos_obj, outpos_obj, message_obj,
                                        escape_obj);
        }
      }
    }
  }
  Object dst_obj(&scope, runtime->strFromStrArray(dst));
  Object length_obj(&scope, runtime->newInt(length));
  Object message_obj(&scope, runtime->newStrFromCStr(""));
  Object escape_obj(&scope, runtime->newInt(first_invalid_escape_index));
  return runtime->newTupleWith4(dst_obj, length_obj, message_obj, escape_obj);
}

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
static Utf8DecoderResult isValidUtf8Codepoint(const Byteslike& bytes,
                                              word index) {
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
    if (!UTF8::isTrailByte(bytes.byteAt(index + 1))) {
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
      if (!UTF8::isTrailByte(ch2) || (ch2 < 0xA0 ? ch == 0xE0 : ch == 0xED)) {
        return kInvalidContinuation1;
      }
      return kUnexpectedEndOfData;
    }
    byte ch2 = bytes.byteAt(index + 1);
    if (!UTF8::isTrailByte(ch2)) {
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
    if (!UTF8::isTrailByte(bytes.byteAt(index + 2))) {
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
      if (!UTF8::isTrailByte(ch2) || (ch2 < 0x90 ? ch == 0xF0 : ch == 0xF4)) {
        return kInvalidContinuation1;
      }
      if (index + 2 >= length) {
        return kUnexpectedEndOfData;
      }
      if (!UTF8::isTrailByte(bytes.byteAt(index + 2))) {
        return kInvalidContinuation2;
      }
      return kUnexpectedEndOfData;
    }
    byte ch2 = bytes.byteAt(index + 1);
    if (!UTF8::isTrailByte(ch2)) {
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
    if (!UTF8::isTrailByte(bytes.byteAt(index + 2))) {
      return kInvalidContinuation2;
    }
    if (!UTF8::isTrailByte(bytes.byteAt(index + 3))) {
      return kInvalidContinuation3;
    }
    return k4Byte;
  }
  return kInvalidStart;
}

RawObject FUNC(_codecs, _utf_8_decode)(Thread* thread, Arguments args) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Object final_obj(&scope, args.get(4));
  DCHECK(final_obj.isBool(), "Fifth arg to _utf_8_decode must be bool");
  Object data(&scope, args.get(0));
  Str errors(&scope, strUnderlying(args.get(1)));
  word index = intUnderlying(args.get(2)).asWord();
  StrArray dst(&scope, args.get(3));

  word length;
  Byteslike bytes(&scope, thread, *data);
  length = bytes.length();
  runtime->strArrayEnsureCapacity(thread, dst, length);
  word i = asciiDecode(thread, dst, bytes, index, length);
  if (i == length) {
    Object dst_obj(&scope, runtime->strFromStrArray(dst));
    Object length_obj(&scope, runtime->newInt(length));
    Object message_obj(&scope, runtime->newStrFromCStr(""));
    return runtime->newTupleWith3(dst_obj, length_obj, message_obj);
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
      case ID(replace): {
        Str temp(&scope, SmallStr::fromCodePoint(kReplacementCharacter));
        runtime->strArrayAddStr(thread, dst, temp);
        i = error_end;
        break;
      }
      case ID(surrogateescape): {
        for (; i < error_end; ++i) {
          Str temp(&scope, SmallStr::fromCodePoint(Unicode::kLowSurrogateStart +
                                                   bytes.byteAt(i)));
          runtime->strArrayAddStr(thread, dst, temp);
        }
        break;
      }
      case ID(ignore):
        i = error_end;
        break;
      default: {
        Object outpos_obj(&scope, runtime->newInt(i));
        Object error_end_obj(&scope, runtime->newInt(error_end));
        Object message_obj(&scope, runtime->newStrFromCStr(error_message));
        return runtime->newTupleWith3(outpos_obj, error_end_obj, message_obj);
      }
    }
  }
  Object dst_obj(&scope, runtime->strFromStrArray(dst));
  Object outpos_obj(&scope, runtime->newInt(i));
  Object message_obj(&scope, Str::empty());
  return runtime->newTupleWith3(dst_obj, outpos_obj, message_obj);
}

RawObject FUNC(_codecs, _utf_8_encode)(Thread* thread, Arguments args) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Object output_obj(&scope, args.get(3));
  DCHECK(runtime->isInstanceOfBytearray(*output_obj),
         "Fourth arg to _utf_8_encode must be bytearray");
  Str data(&scope, strUnderlying(args.get(0)));
  Str errors(&scope, strUnderlying(args.get(1)));
  word index = intUnderlying(args.get(2)).asWord();
  Bytearray output(&scope, *output_obj);

  SymbolId error_symbol = lookupSymbolForErrorHandler(errors);
  for (word byte_offset = thread->strOffset(data, index);
       byte_offset < data.length(); index++) {
    word num_bytes;
    int32_t codepoint = data.codePointAt(byte_offset, &num_bytes);
    byte_offset += num_bytes;
    if (!Unicode::isSurrogate(codepoint)) {
      for (word j = byte_offset - num_bytes; j < byte_offset; j++) {
        bytearrayAdd(thread, runtime, output, data.byteAt(j));
      }
    } else {
      switch (error_symbol) {
        case ID(ignore):
          continue;
        case ID(replace):
          bytearrayAdd(thread, runtime, output, kASCIIReplacement);
          continue;
        case ID(surrogateescape):
          if (isEscapedLatin1Surrogate(codepoint)) {
            bytearrayAdd(thread, runtime, output,
                         codepoint - Unicode::kLowSurrogateStart);
            continue;
          }
          break;
        case ID(surrogatepass):
          if (Unicode::isSurrogate(codepoint)) {
            bytearrayAdd(thread, runtime, output, data.byteAt(byte_offset - 3));
            bytearrayAdd(thread, runtime, output, data.byteAt(byte_offset - 2));
            bytearrayAdd(thread, runtime, output, data.byteAt(byte_offset - 1));
            continue;
          }
          break;
        default:
          break;
      }
      Object outpos1(&scope, runtime->newInt(index));
      while (byte_offset < data.length() &&
             Unicode::isSurrogate(data.codePointAt(byte_offset, &num_bytes))) {
        byte_offset += num_bytes;
        index++;
      }
      Object outpos2(&scope, runtime->newInt(index + 1));
      return runtime->newTupleWith2(outpos1, outpos2);
    }
  }
  Object output_bytes(&scope, bytearrayAsBytes(thread, output));
  Object index_obj(&scope, runtime->newInt(index));
  return runtime->newTupleWith2(output_bytes, index_obj);
}

static void appendUtf16ToBytearray(Thread* thread, Runtime* runtime,
                                   const Bytearray& writer, int32_t codepoint,
                                   endian endianness) {
  if (endianness == endian::little) {
    bytearrayAdd(thread, runtime, writer, codepoint);
    bytearrayAdd(thread, runtime, writer, codepoint >> kBitsPerByte);
  } else {
    bytearrayAdd(thread, runtime, writer, codepoint >> kBitsPerByte);
    bytearrayAdd(thread, runtime, writer, codepoint);
  }
}

static int32_t highSurrogate(int32_t codepoint) {
  return Unicode::kHighSurrogateStart - (0x10000 >> 10) + (codepoint >> 10);
}

static int32_t lowSurrogate(int32_t codepoint) {
  return Unicode::kLowSurrogateStart + (codepoint & 0x3FF);
}

RawObject FUNC(_codecs, _utf_16_encode)(Thread* thread, Arguments args) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Object output_obj(&scope, args.get(3));
  DCHECK(runtime->isInstanceOfBytearray(*output_obj),
         "Fourth arg to _utf_16_encode must be bytearray");
  Str data(&scope, strUnderlying(args.get(0)));
  Str errors(&scope, strUnderlying(args.get(1)));
  word index = intUnderlying(args.get(2)).asWord();
  Bytearray output(&scope, *output_obj);
  OptInt<int32_t> byteorder = intUnderlying(args.get(4)).asInt<int32_t>();
  if (byteorder.error != CastError::None) {
    return thread->raiseWithFmt(LayoutId::kOverflowError,
                                "Python int too large to convert to C int");
  }

  SymbolId error_id = lookupSymbolForErrorHandler(errors);
  for (word byte_offset = thread->strOffset(data, index);
       byte_offset < data.length(); index++) {
    endian endianness = byteorder.value <= 0 ? endian::little : endian::big;
    word num_bytes;
    int32_t codepoint = data.codePointAt(byte_offset, &num_bytes);
    byte_offset += num_bytes;
    if (!Unicode::isSurrogate(codepoint)) {
      if (codepoint < Unicode::kHighSurrogateStart) {
        appendUtf16ToBytearray(thread, runtime, output, codepoint, endianness);
      } else {
        appendUtf16ToBytearray(thread, runtime, output,
                               highSurrogate(codepoint), endianness);
        appendUtf16ToBytearray(thread, runtime, output, lowSurrogate(codepoint),
                               endianness);
      }
    } else {
      switch (error_id) {
        case ID(ignore):
          continue;
        case ID(replace):
          appendUtf16ToBytearray(thread, runtime, output, kASCIIReplacement,
                                 endianness);
          continue;
        case ID(surrogateescape):
          if (isEscapedLatin1Surrogate(codepoint)) {
            appendUtf16ToBytearray(thread, runtime, output,
                                   codepoint - Unicode::kLowSurrogateStart,
                                   endianness);
            continue;
          }
          break;
        default:
          break;
      }
      Object outpos1(&scope, runtime->newInt(index));
      while (byte_offset < data.length() &&
             Unicode::isSurrogate(data.codePointAt(byte_offset, &num_bytes))) {
        byte_offset += num_bytes;
        index++;
      }
      Object outpos2(&scope, runtime->newInt(index + 1));
      return runtime->newTupleWith2(outpos1, outpos2);
    }
  }
  Object output_bytes(&scope, bytearrayAsBytes(thread, output));
  Object index_obj(&scope, runtime->newInt(index));
  return runtime->newTupleWith2(output_bytes, index_obj);
}

static void appendUtf32ToBytearray(Thread* thread, Runtime* runtime,
                                   const Bytearray& writer, int32_t codepoint,
                                   endian endianness) {
  if (endianness == endian::little) {
    bytearrayAdd(thread, runtime, writer, codepoint);
    bytearrayAdd(thread, runtime, writer, codepoint >> (kBitsPerByte));
    bytearrayAdd(thread, runtime, writer, codepoint >> (kBitsPerByte * 2));
    bytearrayAdd(thread, runtime, writer, codepoint >> (kBitsPerByte * 3));
  } else {
    bytearrayAdd(thread, runtime, writer, codepoint >> (kBitsPerByte * 3));
    bytearrayAdd(thread, runtime, writer, codepoint >> (kBitsPerByte * 2));
    bytearrayAdd(thread, runtime, writer, codepoint >> (kBitsPerByte));
    bytearrayAdd(thread, runtime, writer, codepoint);
  }
}

RawObject FUNC(_codecs, _utf_32_encode)(Thread* thread, Arguments args) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Object output_obj(&scope, args.get(3));
  DCHECK(runtime->isInstanceOfBytearray(*output_obj),
         "Fourth arg to _utf_32_encode must be bytearray");
  Str data(&scope, strUnderlying(args.get(0)));
  Str errors(&scope, strUnderlying(args.get(1)));
  word index = intUnderlying(args.get(2)).asWord();
  Bytearray output(&scope, *output_obj);
  OptInt<int32_t> byteorder = intUnderlying(args.get(4)).asInt<int32_t>();
  if (byteorder.error != CastError::None) {
    return thread->raiseWithFmt(LayoutId::kOverflowError,
                                "Python int too large to convert to C int");
  }

  SymbolId error_id = lookupSymbolForErrorHandler(errors);
  for (word byte_offset = thread->strOffset(data, index);
       byte_offset < data.length(); index++) {
    endian endianness = byteorder.value <= 0 ? endian::little : endian::big;
    word num_bytes;
    int32_t codepoint = data.codePointAt(byte_offset, &num_bytes);
    byte_offset += num_bytes;
    if (!Unicode::isSurrogate(codepoint)) {
      appendUtf32ToBytearray(thread, runtime, output, codepoint, endianness);
    } else {
      switch (error_id) {
        case ID(ignore):
          continue;
        case ID(replace):
          appendUtf32ToBytearray(thread, runtime, output, kASCIIReplacement,
                                 endianness);
          continue;
        case ID(surrogateescape):
          if (isEscapedLatin1Surrogate(codepoint)) {
            appendUtf32ToBytearray(thread, runtime, output,
                                   codepoint - Unicode::kLowSurrogateStart,
                                   endianness);
            continue;
          }
          break;
        default:
          break;
      }
      Object outpos1(&scope, runtime->newInt(index));
      while (byte_offset < data.length() &&
             Unicode::isSurrogate(data.codePointAt(byte_offset, &num_bytes))) {
        byte_offset += num_bytes;
        index++;
      }
      Object outpos2(&scope, runtime->newInt(index + 1));
      return runtime->newTupleWith2(outpos1, outpos2);
    }
  }
  Object output_bytes(&scope, bytearrayAsBytes(thread, output));
  Object index_obj(&scope, runtime->newInt(index));
  return runtime->newTupleWith2(output_bytes, index_obj);
}

// Takes a Bytearray and a Str object, and appends each byte in the Str to the
// Bytearray one by one
RawObject FUNC(_codecs, _bytearray_string_append)(Thread* thread,
                                                  Arguments args) {
  HandleScope scope(thread);
  Bytearray dst(&scope, args.get(0));
  Str data(&scope, args.get(1));
  for (word i = 0; i < data.length(); ++i) {
    bytearrayAdd(thread, thread->runtime(), dst, data.byteAt(i));
  }
  return NoneType::object();
}

RawObject FUNC(_codecs, _raw_unicode_escape_encode)(Thread* thread,
                                                    Arguments args) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Str data(&scope, strUnderlying(args.get(0)));
  word size = data.codePointLength();
  Bytearray dst(&scope, runtime->newBytearray());
  word length = data.length();

  // 2 byte codepoints can be expanded to 4 bytes + 2 escape characters
  // 4 byte codepoints well be expanded to 8 bytes + 2 escape characters
  // To be safe we double the bytecount and add space for 2 escape characters
  // per codepoint.
  word expanded_size = length * 2 + size * 2;
  runtime->bytearrayEnsureCapacity(thread, dst, expanded_size);
  word num_bytes;
  for (word index = 0, byte_offset = thread->strOffset(data, index);
       byte_offset < data.length(); index++) {
    int32_t codepoint = data.codePointAt(byte_offset, &num_bytes);
    byte_offset += num_bytes;
    // U+0000-U+00ff range: Copy 8-bit characters as-is
    if (codepoint < 0x100) {
      bytearrayAdd(thread, runtime, dst, codepoint);
    }
    // U+0100-U+ffff range: Map 16-bit characters to '\uHHHH'
    else if (codepoint < 0x10000) {
      bytearrayAdd(thread, runtime, dst, '\\');
      bytearrayAdd(thread, runtime, dst, 'u');
      bytearrayAdd(thread, runtime, dst,
                   Utils::kHexDigits[(codepoint >> 12) & 0xf]);
      bytearrayAdd(thread, runtime, dst,
                   Utils::kHexDigits[(codepoint >> 8) & 0xf]);
      bytearrayAdd(thread, runtime, dst,
                   Utils::kHexDigits[(codepoint >> 4) & 0xf]);
      bytearrayAdd(thread, runtime, dst, Utils::kHexDigits[codepoint & 15]);
    }
    // U+010000-U+10ffff range: Map 32-bit characters to '\U00HHHHHH'
    else {
      CHECK(codepoint <= kMaxUnicode, "expected a valid unicode code point");
      bytearrayAdd(thread, runtime, dst, '\\');
      bytearrayAdd(thread, runtime, dst, 'U');
      bytearrayAdd(thread, runtime, dst, '0');
      bytearrayAdd(thread, runtime, dst, '0');
      bytearrayAdd(thread, runtime, dst,
                   Utils::kHexDigits[(codepoint >> 20) & 0xf]);
      bytearrayAdd(thread, runtime, dst,
                   Utils::kHexDigits[(codepoint >> 16) & 0xf]);
      bytearrayAdd(thread, runtime, dst,
                   Utils::kHexDigits[(codepoint >> 12) & 0xf]);
      bytearrayAdd(thread, runtime, dst,
                   Utils::kHexDigits[(codepoint >> 8) & 0xf]);
      bytearrayAdd(thread, runtime, dst,
                   Utils::kHexDigits[(codepoint >> 4) & 0xf]);
      bytearrayAdd(thread, runtime, dst, Utils::kHexDigits[codepoint & 15]);
    }
  }
  Object output_bytes(&scope, bytearrayAsBytes(thread, dst));
  Object size_obj(&scope, runtime->newInt(size));
  return runtime->newTupleWith2(output_bytes, size_obj);
}

RawObject FUNC(_codecs, _raw_unicode_escape_decode)(Thread* thread,
                                                    Arguments args) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object data(&scope, args.get(0));
  Str errors(&scope, strUnderlying(args.get(1)));
  word index = intUnderlying(args.get(2)).asWord();
  StrArray dst(&scope, args.get(3));

  Byteslike bytes(&scope, thread, *data);
  word length = bytes.length();
  runtime->strArrayEnsureCapacity(thread, dst, length);
  for (word i = index; i < length;) {
    const char* message = nullptr;
    word start_pos = i;
    byte ch = bytes.byteAt(i);
    i++;
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
      // \\ at end of string
      runtime->strArrayAddASCII(thread, dst, '\\');
    } else {
      int32_t decoded;
      ch = bytes.byteAt(i);
      i++;
      // Only care about \uXXXX and \UXXXXXXXX when decoding raw unicode.
      switch (ch) {
        // \uXXXX
        case 'u': {
          if ((decoded = decodeHexEscaped(bytes, &i, 4)) < 0) {
            message = (decoded == -1 ? "truncated \\uXXXX escape"
                                     : "illegal Unicode character");
          }
          break;
        }
        // \UXXXXXXXX
        case 'U': {
          if ((decoded = decodeHexEscaped(bytes, &i, 8)) < 0) {
            if (decoded == -1) {
              message = "truncated \\UXXXXXXXX escape";
            } else if (decoded == -2) {
              message = "\\Uxxxxxxxx out of range";
            } else {
              message = "illegal Unicode character";
            }
          }
          break;
        }
        default: {
          runtime->strArrayAddASCII(thread, dst, '\\');
          decoded = ch;
        }
      }
      if (decoded >= 0) {
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
        case ID(replace): {
          Str temp(&scope, SmallStr::fromCodePoint(0xFFFD));
          runtime->strArrayAddStr(thread, dst, temp);
          break;
        }
        case ID(ignore):
          break;
        default: {
          Object start_pos_obj(&scope, runtime->newInt(start_pos));
          Object outpos_obj(&scope, runtime->newInt(i));
          Object message_obj(&scope, runtime->newStrFromCStr(message));
          return runtime->newTupleWith3(start_pos_obj, outpos_obj, message_obj);
        }
      }
    }
  }
  Object dst_obj(&scope, runtime->strFromStrArray(dst));
  Object length_obj(&scope, runtime->newInt(length));
  Object message_obj(&scope, runtime->newStrFromCStr(""));
  return runtime->newTupleWith3(dst_obj, length_obj, message_obj);
}
}  // namespace py
