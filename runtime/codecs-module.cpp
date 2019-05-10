#include "codecs-module.h"

#include "bytearray-builtins.h"
#include "frame.h"
#include "frozen-modules.h"
#include "int-builtins.h"
#include "runtime.h"
#include "str-builtins.h"

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

// Encodes a codepoint into a UTF-8 encoded byte array and returns the number
// of bytes used in the encoding.
// It assumes that it was passed in a byte array at least 4 bytes long
// It returns -1 if the codepoint fails a bounds check
static int encodeUTF8CodePoint(int32_t codepoint, byte* byte_pattern) {
  if (codepoint < 0 || codepoint > kMaxUnicode) {
    return -1;
  }
  if (codepoint <= kMaxASCII) {
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
    {SymbolId::kUnderAsciiEncode, underAsciiEncode},
    {SymbolId::kUnderAsciiDecode, underAsciiDecode},
    {SymbolId::kUnderLatin1Encode, underLatin1Encode},
    {SymbolId::kUnderUtf16Encode, underUtf16Encode},
    {SymbolId::kUnderUtf32Encode, underUtf32Encode},
    {SymbolId::kUnderUtf8Encode, underUtf8Encode},
    {SymbolId::kUnderByteArrayStringAppend, underByteArrayStringAppend},
    {SymbolId::kUnderByteArrayToString, underByteArrayToString},
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
  DCHECK(runtime->isInstanceOfByteArray(*output_obj),
         "Fourth arg to _ascii_decode must be bytearray");
  // TODO(T36619847): Bytes subclass handling
  Bytes bytes(&scope, *bytes_obj);
  Str errors(&scope, strUnderlying(thread, errors_obj));
  Int index(&scope, intUnderlying(thread, index_obj));
  ByteArray dst(&scope, *output_obj);

  Tuple result(&scope, runtime->newTuple(2));
  word length = bytes.length();
  runtime->byteArrayEnsureCapacity(thread, dst, length);
  word outpos = asciiDecode(thread, dst, bytes, index.asWord());
  if (outpos == length) {
    result.atPut(0, runtime->newStrFromByteArray(dst));
    result.atPut(1, runtime->newInt(length));
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
        int num_bytes = encodeUTF8CodePoint(kLowSurrogateStart + c, chars);
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

// Takes a ByteArray and returns the call to newStrFromByteArray
RawObject UnderCodecsModule::underByteArrayToString(Thread* thread,
                                                    Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  ByteArray src(&scope, args.get(3));
  return thread->runtime()->newStrFromByteArray(src);
}

}  // namespace python
