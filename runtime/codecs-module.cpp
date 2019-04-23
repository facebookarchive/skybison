#include "codecs-module.h"

#include "bytearray-builtins.h"
#include "frame.h"
#include "frozen-modules.h"
#include "runtime.h"

namespace python {

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

// Encodes a codepoint into a UTF-8 encoded byte array and returns the number
// of bytes used in the encoding.
// It assumes that it was passed in a byte array at least 4 bytes long
// It returns -1 if the codepoint fails a bounds check
static int encodeUTF8CodePoint(int32_t codepoint, byte* byte_pattern) {
  if (codepoint < 0 || codepoint > kMaxUnicode) {
    return -1;
  }
  if (codepoint <= 0x7f) {
    byte_pattern[0] = codepoint;
    return 1;
  }
  if (codepoint <= 0x7ff) {
    byte_pattern[0] = (0xc0 | ((codepoint >> 6) & 0x1f));
    byte_pattern[1] = (0x80 | (codepoint & 0x3f));
    return 2;
  }
  if (codepoint <= 0xffff) {
    byte_pattern[0] = (0xe0 | ((codepoint >> 12) & 0x0f));
    byte_pattern[1] = (0x80 | ((codepoint >> 6) & 0x3f));
    byte_pattern[2] = (0x80 | (codepoint & 0x3f));
    return 3;
  }
  byte_pattern[0] = (0xf0 | ((codepoint >> 18) & 0x07));
  byte_pattern[1] = (0x80 | ((codepoint >> 12) & 0x3f));
  byte_pattern[2] = (0x80 | ((codepoint >> 6) & 0x3f));
  byte_pattern[3] = (0x80 | (codepoint & 0x3f));
  return 4;
}

static int asciiDecode(Thread* thread, const ByteArray& dst, const Bytes& src,
                       int index) {
  // TODO(T41032331): Implement a fastpass to read longs instead of chars
  Runtime* runtime = thread->runtime();
  for (; index < src.length(); index++) {
    byte byte = src.byteAt(index);
    if (byte & 0x80) {
      break;
    }
    byteArrayAdd(thread, runtime, dst, byte);
  }
  return index;
}

const BuiltinMethod UnderCodecsModule::kBuiltinMethods[] = {
    {SymbolId::kUnderAsciiDecode, underAsciiDecode},
    {SymbolId::kUnderUtf8Encode, underUtf8Encode},
    {SymbolId::kUnderByteArrayStringAppend, underByteArrayStringAppend},
    {SymbolId::kUnderByteArrayToString, underByteArrayToString},
    {SymbolId::kSentinelId, nullptr},
};

const char* const UnderCodecsModule::kFrozenData = kUnderCodecsModuleData;

RawObject UnderCodecsModule::underAsciiDecode(Thread* thread, Frame* frame,
                                              word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  DCHECK(args.get(0).isBytes(), "First arg to _ascii_decode must be bytes");
  Bytes bytes(&scope, args.get(0));
  DCHECK(args.get(1).isStr(), "Second arg to _ascii_decode must be str");
  Str errors(&scope, args.get(1));
  DCHECK(args.get(2).isInt(), "Third arg to _ascii_decode must be int");
  Int index_obj(&scope, args.get(2));
  word index = index_obj.asWord();
  DCHECK(args.get(3).isByteArray(),
         "Fourth arg to _ascii_decode must be bytearray");
  ByteArray dst(&scope, args.get(3));
  Runtime* runtime = thread->runtime();
  Tuple result(&scope, runtime->newTuple(2));

  word length = bytes.length();
  runtime->byteArrayEnsureCapacity(thread, dst, length);
  word outpos = asciiDecode(thread, dst, bytes, index);
  if (outpos == length) {
    result.atPut(0, runtime->newStrFromByteArray(dst));
    result.atPut(1, runtime->newIntFromUnsigned(length));
    return *result;
  }

  SymbolId error_id = lookupSymbolForErrorHandler(errors);
  while (outpos < length) {
    byte c = bytes.byteAt(outpos);
    if (c < 128) {
      byteArrayAdd(thread, runtime, dst, c);
      ++outpos;
      continue;
    }
    switch (error_id) {
      case SymbolId::kReplace: {
        byte chars[] = {0xEF, 0xBF, 0xBD};
        runtime->byteArrayExtend(thread, dst, View<byte>(chars, 3));
        ++outpos;
        break;
      }
      case SymbolId::kSurrogateescape: {
        byte chars[4];
        int num_bytes = encodeUTF8CodePoint(0xdc00 + c, chars);
        runtime->byteArrayExtend(thread, dst, View<byte>(chars, num_bytes));
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
  result.atPut(0, runtime->newStrFromByteArray(dst));
  result.atPut(1, runtime->newIntFromUnsigned(length));
  return *result;
}

static bool isSurrogate(int32_t codepoint) {
  return 0xD800 <= codepoint && codepoint <= 0xDFFF;
}

// CPython encodes latin1 codepoints into the low-surrogate range, and is able
// to recover the original codepoints from those decodable surrogate points.
static bool isEscapedLatin1Surrogate(int32_t codepoint) {
  return 0xDC80 <= codepoint && codepoint <= 0xDCFF;
}

RawObject UnderCodecsModule::underUtf8Encode(Thread* thread, Frame* frame,
                                             word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  DCHECK(args.get(0).isStr(), "First arg to _utf_8_encode must be str");
  Str data(&scope, args.get(0));
  DCHECK(args.get(1).isStr(), "Second arg to _utf_8_encode must be str");
  Str errors(&scope, args.get(1));
  DCHECK(args.get(2).isInt(), "Third arg to _utf_8_encode must be int");
  Int index_obj(&scope, args.get(2));
  DCHECK(args.get(3).isByteArray(),
         "Fourth arg to _utf_8_encode must be bytearray");
  ByteArray output(&scope, args.get(3));

  Runtime* runtime = thread->runtime();
  Tuple result(&scope, runtime->newTuple(2));
  SymbolId error_symbol = lookupSymbolForErrorHandler(errors);
  word index = index_obj.asWord();
  for (word byte_offset = data.offsetByCodePoints(0, index);
       byte_offset < data.length(); index++) {
    word num_bytes;
    int32_t codepoint = data.codePointAt(byte_offset, &num_bytes);
    byte_offset += num_bytes;
    if (!isSurrogate(codepoint)) {
      for (word i = byte_offset - num_bytes; i < byte_offset; i++) {
        byteArrayAdd(thread, runtime, output, data.charAt(i));
      }
    } else {
      switch (error_symbol) {
        case SymbolId::kIgnore:
          continue;
        case SymbolId::kReplace:
          byteArrayAdd(thread, runtime, output, '?');
          continue;
        case SymbolId::kSurrogateescape:
          if (isEscapedLatin1Surrogate(codepoint)) {
            byteArrayAdd(thread, runtime, output, codepoint - 0xDC00);
            continue;
          }
          break;
        default:
          break;
      }
      result.atPut(0, runtime->newInt(index));
      while (byte_offset < data.length() &&
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

// Takes a ByteArray and returns the call to newStrFromByteArray
RawObject UnderCodecsModule::underByteArrayToString(Thread* thread,
                                                    Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  ByteArray src(&scope, args.get(3));
  return thread->runtime()->newStrFromByteArray(src);
}

}  // namespace python
