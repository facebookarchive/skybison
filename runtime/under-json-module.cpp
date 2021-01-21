#include "builtins.h"
#include "dict-builtins.h"
#include "float-builtins.h"
#include "handles.h"
#include "objects.h"
#include "runtime.h"
#include "str-builtins.h"
#include "thread.h"
#include "unicode.h"
#include "utils.h"

namespace py {

static const int kNumUEscapeChars = 4;

enum class LoadsArg {
  kString = 0,
  kEncoding = 1,
  kCls = 2,
  kObjectHook = 3,
  kParseFloat = 4,
  kParseInt = 5,
  kParseConstant = 6,
  kObjectPairsHook = 7,
  kKw = 8,
};

struct JSONParser {
  // Index of next byte to read.
  word next;
  word length;
  Arguments args;
  bool has_object_hook;
  bool has_object_pairs_hook;
  bool has_parse_constant;
  bool has_parse_float;
  bool has_parse_int;
  bool strict;
};

static NEVER_INLINE RawObject callObjectHook(Thread* thread, JSONParser* env,
                                             const Object& dict) {
  HandleScope scope(thread);
  DCHECK(dict.isDict(), "expected dict");
  if (env->has_object_pairs_hook) {
    Object hook(&scope,
                env->args.get(static_cast<word>(LoadsArg::kObjectPairsHook)));
    Object items(&scope, thread->invokeMethod1(dict, ID(items)));
    if (items.isErrorException()) return *items;
    Object list_type(&scope, thread->runtime()->typeAt(LayoutId::kList));
    Object list(&scope, Interpreter::call1(thread, list_type, items));
    if (list.isErrorException()) return *list;
    return Interpreter::call1(thread, hook, list);
  }
  Object hook(&scope, env->args.get(static_cast<word>(LoadsArg::kObjectHook)));
  return Interpreter::call1(thread, hook, dict);
}

static NEVER_INLINE int callParseConstant(Thread* thread, JSONParser* env,
                                          const DataArray& data, word length,
                                          Object* value_out) {
  HandleScope scope(thread);
  Object hook(&scope,
              env->args.get(static_cast<word>(LoadsArg::kParseConstant)));
  Str string(&scope, dataArraySubstr(thread, data, env->next - length, length));
  *value_out = Interpreter::call1(thread, hook, string);
  if (value_out->isErrorException()) return -1;
  return 0;
}

static NEVER_INLINE RawObject callParseFloat(Thread* thread, JSONParser* env,
                                             const DataArray& data, word begin,
                                             word length) {
  HandleScope scope(thread);
  Object hook(&scope, env->args.get(static_cast<word>(LoadsArg::kParseFloat)));
  Object str(&scope, dataArraySubstr(thread, data, begin, length));
  return Interpreter::call1(thread, hook, str);
}

static NEVER_INLINE RawObject callParseInt(Thread* thread, JSONParser* env,
                                           const DataArray& data, word begin) {
  HandleScope scope(thread);
  Object hook(&scope, env->args.get(static_cast<word>(LoadsArg::kParseInt)));
  Object str(&scope, dataArraySubstr(thread, data, begin, env->next - begin));
  return Interpreter::call1(thread, hook, str);
}

static byte nextNonWhitespace(Thread*, JSONParser* env, const DataArray& data) {
  word next = env->next;
  word length = env->length;
  byte b;
  do {
    if (next >= length) {
      // Set `next` to `length + 1` to indicate EOF (end of file).
      env->next = length + 1;
      return 0;
    }
    b = data.byteAt(next++);
  } while (b == ' ' || b == '\t' || b == '\n' || b == '\r');
  env->next = next;
  return b;
}

static NEVER_INLINE RawObject raiseJSONDecodeError(Thread* thread,
                                                   JSONParser* env,
                                                   const DataArray& data,
                                                   word index,
                                                   const char* msg) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object json_decode_error(&scope, runtime->lookupNameInModule(
                                       thread, ID(_json), ID(JSONDecodeError)));
  CHECK(json_decode_error.isType(), "_json.JSONDecodeError not found");

  // TODO(T81331502): Add helper function for byte offset to code point index
  // translation.
  word pos = 0;
  for (word i = 0, cp_length; i < index; i += cp_length) {
    data.codePointAt(i, &cp_length);
    pos++;
  }

  // Convert byte position to codepoint.
  Object msg_str(&scope, runtime->newStrFromCStr(msg));
  Object doc(&scope, env->args.get(static_cast<word>(LoadsArg::kString)));
  Object pos_obj(&scope, runtime->newInt(pos));
  Object args(&scope, runtime->newTupleWith3(msg_str, doc, pos_obj));
  return thread->raiseWithType(*json_decode_error, *args);
}

static RawObject scanEscapeSequence(Thread* thread, JSONParser* env,
                                    const DataArray& data, word begin) {
  word next = env->next;
  word length = env->length;
  if (next >= length) {
    return raiseJSONDecodeError(thread, env, data, begin - 1,
                                "Unterminated string starting at");
  }
  byte ascii_result;
  byte b = data.byteAt(next++);
  switch (b) {
    case '"':
    case '\\':
    case '/':
      ascii_result = b;
      break;
    case 'b':
      ascii_result = '\b';
      break;
    case 'f':
      ascii_result = '\f';
      break;
    case 'n':
      ascii_result = '\n';
      break;
    case 'r':
      ascii_result = '\r';
      break;
    case 't':
      ascii_result = '\t';
      break;
    case 'u': {
      int32_t code_point;
      if (next >= length - kNumUEscapeChars) {
        return raiseJSONDecodeError(thread, env, data, next - 1,
                                    "Invalid \\uXXXX escape");
      }
      code_point = 0;
      word end = next + kNumUEscapeChars;
      do {
        b = data.byteAt(next++);
        code_point <<= kBitsPerHexDigit;
        if ('0' <= b && b <= '9') {
          code_point |= b - '0';
        } else if ('a' <= b && b <= 'f') {
          code_point |= b - 'a' + 10;
        } else if ('A' <= b && b <= 'F') {
          code_point |= b - 'A' + 10;
        } else {
          return raiseJSONDecodeError(thread, env, data, end - kNumUEscapeChars,
                                      "Invalid \\uXXXX escape");
        }
      } while (next < end);
      if (Unicode::isHighSurrogate(code_point) &&
          next < length - (kNumUEscapeChars + 2) && data.byteAt(next) == '\\' &&
          data.byteAt(next + 1) == 'u') {
        word next2 = next + 2;
        int32_t code_point2 = 0;
        word end2 = next2 + kNumUEscapeChars;
        do {
          byte b2 = data.byteAt(next2++);
          code_point2 <<= kBitsPerHexDigit;
          if ('0' <= b2 && b2 <= '9') {
            code_point2 |= b2 - '0';
          } else if ('a' <= b2 && b2 <= 'f') {
            code_point2 |= b2 - 'a' + 10;
          } else if ('A' <= b2 && b2 <= 'F') {
            code_point2 |= b2 - 'A' + 10;
          } else {
            code_point2 = 0;
            break;
          }
        } while (next2 < end2);
        if (Unicode::isLowSurrogate(code_point2)) {
          code_point = Unicode::combineSurrogates(code_point, code_point2);
          next = end2;
        }
      }
      env->next = next;
      return SmallStr::fromCodePoint(code_point);
    }
    default:
      return raiseJSONDecodeError(thread, env, data, next - 2,
                                  "Invalid \\escape");
  }
  env->next = next;
  return SmallStr::fromCodePoint(ascii_result);
}

static RawObject scanFloat(Thread* thread, JSONParser* env,
                           const DataArray& data, byte b, word begin) {
  word next = env->next;
  word length = env->length;
  if (b == '.') {
    // Need at least 1 digit.
    if (next >= length) {
      return raiseJSONDecodeError(thread, env, data, next - 1, "Extra data");
    }
    b = data.byteAt(next++);
    if (b < '0' || b > '9') {
      return raiseJSONDecodeError(thread, env, data, next - 2, "Extra data");
    }
    // Optionally followed by more digits.
    do {
      if (next >= length) {
        b = 0;
        next++;
        break;
      }
      b = data.byteAt(next++);
    } while ('0' <= b && b <= '9');
  }
  if (b == 'e' || b == 'E') {
    word e_begin = next;
    if (next >= length) {
      return raiseJSONDecodeError(thread, env, data, e_begin - 1, "Extra data");
    }
    b = data.byteAt(next++);
    if (b == '+' || b == '-') {
      if (next >= length) {
        return raiseJSONDecodeError(thread, env, data, e_begin - 1,
                                    "Extra data");
      }
      b = data.byteAt(next++);
    }
    // Need at least 1 digit.
    if (b < '0' || b > '9') {
      return raiseJSONDecodeError(thread, env, data, e_begin - 1, "Extra data");
    }
    // Optionally followed by more digits.
    do {
      if (next >= length) {
        b = 0;
        next++;
        break;
      }
      b = data.byteAt(next++);
    } while ('0' <= b && b <= '9');
  }
  next--;
  env->next = next;

  word number_length = next - begin;
  if (env->has_parse_float) {
    return callParseFloat(thread, env, data, begin, number_length);
  }
  unique_c_ptr<byte> buf(static_cast<byte*>(std::malloc(number_length + 1)));
  data.copyToStartAt(buf.get(), number_length, begin);
  buf.get()[number_length] = '\0';
  return floatFromDigits(thread, reinterpret_cast<char*>(buf.get()),
                         number_length);
}

static RawObject scanLargeInt(Thread* thread, JSONParser* env,
                              const DataArray& data, byte b, word begin,
                              bool negative, word value) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  word next = env->next;
  word length = env->length;
  Int result(&scope, SmallInt::fromWord(value));
  Int factor(&scope, SmallInt::fromWord(SmallInt::kMaxDigits10Pow));
  Int value_int(&scope, SmallInt::fromWord(0));

  value = 0;
  word digits = 0;
  for (;;) {
    value += b - '0';
    if (next >= length) break;
    b = data.byteAt(next++);
    if ('0' <= b && b <= '9') {
      digits++;
      if (digits >= SmallInt::kMaxDigits10) {
        value_int = Int::cast(SmallInt::fromWord(value));
        result = runtime->intMultiply(thread, result, factor);
        result = runtime->intAdd(thread, result, value_int);
        digits = 0;
        value = 0;
      } else {
        value *= 10;
      }
      continue;
    }

    if (b == '.' || b == 'e' || b == 'E') {
      env->next = next;
      return scanFloat(thread, env, data, b, begin);
    }

    next--;
    break;
  }
  env->next = next;
  if (env->has_parse_int) {
    return callParseInt(thread, env, data, begin);
  }

  word f = negative ? -10 : 10;
  for (word i = 0; i < digits; i++) {
    f *= 10;
  }
  factor = Int::cast(SmallInt::fromWord(f));
  result = runtime->intMultiply(thread, result, factor);
  value_int = Int::cast(SmallInt::fromWord(value));
  if (negative) {
    result = runtime->intSubtract(thread, result, value_int);
  } else {
    result = runtime->intAdd(thread, result, value_int);
  }
  return *result;
}

static RawObject scanString(Thread* thread, JSONParser* env,
                            const DataArray& data) {
  struct Segment {
    int32_t begin_or_negative_length;
    int32_t length_or_utf8;
  };

  Runtime* runtime = thread->runtime();
  word next = env->next;
  word length = env->length;
  word result_length = 0;
  Vector<Segment> segments;
  word begin = next;
  word segment_begin;
  word segment_length;
  for (;;) {
    segment_begin = next;
    byte b;
    for (;;) {
      if (next >= length) {
        return raiseJSONDecodeError(thread, env, data, begin - 1,
                                    "Unterminated string starting at");
      }
      b = data.byteAt(next++);
      if (b == '"' || b == '\\') {
        break;
      }
      if (ASCII::isControlCharacter(b) && env->strict) {
        return raiseJSONDecodeError(thread, env, data, next - 1,
                                    "Invalid control character at");
      }
    }
    // Segment ends before the current `"` or `\` character.
    segment_length = next - segment_begin - 1;
    if (b == '"') {
      break;
    }

    if (segment_length > 0) {
      segments.push_back(Segment{static_cast<int32_t>(segment_begin),
                                 static_cast<int32_t>(segment_length)});
      result_length += segment_length;
    }

    DCHECK(b == '\\', "Expected backslash");
    env->next = next;
    RawObject escape_result = scanEscapeSequence(thread, env, data, begin);
    if (escape_result.isErrorException()) return escape_result;
    next = env->next;
    RawSmallStr str = SmallStr::cast(escape_result);
    word str_length = str.length();
    Segment segment;
    segment.begin_or_negative_length = -str_length;
    segment.length_or_utf8 = 0;
    CHECK(str_length <= static_cast<word>(sizeof(segment.length_or_utf8)),
          "encoded codepoint should fit in `length_or_utf8`");
    str.copyTo(reinterpret_cast<byte*>(&segment.length_or_utf8), str_length);
    result_length += str_length;
    segments.push_back(segment);
  }
  env->next = next;
  if (segments.size() == 0) {
    return dataArraySubstr(thread, data, segment_begin, segment_length);
  }
  if (segment_length > 0) {
    segments.push_back(Segment{static_cast<int32_t>(segment_begin),
                               static_cast<int32_t>(segment_length)});
    result_length += segment_length;
  }
  HandleScope scope(thread);
  MutableBytes result(&scope,
                      runtime->newMutableBytesUninitialized(result_length));
  word result_index = 0;
  for (Segment segment : segments) {
    word begin_or_negative_length = segment.begin_or_negative_length;
    word length_or_utf8 = segment.length_or_utf8;
    if (begin_or_negative_length >= 0) {
      result.replaceFromWithStartAt(result_index, *data, length_or_utf8,
                                    begin_or_negative_length);
      result_index += length_or_utf8;
    } else {
      word utf8_length = -begin_or_negative_length;
      result.replaceFromWithAll(
          result_index,
          View<byte>(reinterpret_cast<byte*>(&length_or_utf8), utf8_length));
      result_index += utf8_length;
    }
  }
  DCHECK(result_index == result_length, "index/length mismatch");
  return result.becomeStr();
}

static RawObject scanNumber(Thread* thread, JSONParser* env,
                            const DataArray& data, byte b) {
  word begin = env->next - 1;
  word next = env->next;
  word length = env->length;
  bool negative = (b == '-');
  if (negative) {
    if (next >= length) {
      return raiseJSONDecodeError(thread, env, data, length - 1,
                                  "Expecting value");
    }
    negative = true;
    b = data.byteAt(next++);
    if (b < '0' || b > '9') {
      return raiseJSONDecodeError(thread, env, data, next - 2,
                                  "Expecting value");
    }
  }
  if (b == '0') {
    if (next < length) {
      b = data.byteAt(next++);
      if (b == '.' || b == 'e' || b == 'E') {
        env->next = next;
        return scanFloat(thread, env, data, b, begin);
      }
      next--;
    }
    env->next = next;
    if (env->has_parse_int) {
      return callParseInt(thread, env, data, begin);
    }
    return SmallInt::fromWord(0);
  }

  word value = 0;
  word digits_left = SmallInt::kMaxDigits10;
  for (;;) {
    value += b - '0';
    if (next >= length) break;
    b = data.byteAt(next++);
    if ('0' <= b && b <= '9') {
      digits_left--;
      if (digits_left == 0) {
        env->next = next;
        return scanLargeInt(thread, env, data, b, begin, negative, value);
      }
      value *= 10;
      continue;
    }

    if (b == '.' || b == 'e' || b == 'E') {
      env->next = next;
      return scanFloat(thread, env, data, b, begin);
    }

    next--;
    break;
  }
  env->next = next;
  if (env->has_parse_int) {
    return callParseInt(thread, env, data, begin);
  }
  return SmallInt::fromWord(negative ? -value : value);
}

static int scan(Thread* thread, JSONParser* env, const DataArray& data, byte b,
                Object* value_out) {
  for (;;) {
    word next = env->next;
    word length = env->length;

    switch (b) {
      case '"': {
        *value_out = scanString(thread, env, data);
        if (value_out->isErrorException()) return -1;
        return 0;
      }
      case '{':
        return '{';
      case '[':
        return '[';

      case '-':  // `-Infinity` or number
        if (next <= length - 8 && data.byteAt(next) == 'I' &&
            data.byteAt(next + 1) == 'n' && data.byteAt(next + 2) == 'f' &&
            data.byteAt(next + 3) == 'i' && data.byteAt(next + 4) == 'n' &&
            data.byteAt(next + 5) == 'i' && data.byteAt(next + 6) == 't' &&
            data.byteAt(next + 7) == 'y') {
          env->next = next + 8;
          if (env->has_parse_constant) {
            return callParseConstant(thread, env, data, 9, value_out);
          }
          *value_out = thread->runtime()->newFloat(-kDoubleInfinity);
          return 0;
        }
        FALLTHROUGH;
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9': {
        RawObject value = scanNumber(thread, env, data, b);
        *value_out = value;
        if (value.isErrorException()) return -1;
        return 0;
      }

      case 'n':  // `null`
        if (next <= length - 3 && data.byteAt(next) == 'u' &&
            data.byteAt(next + 1) == 'l' && data.byteAt(next + 2) == 'l') {
          env->next = next + 3;
          *value_out = NoneType::object();
          return 0;
        }
        break;
      case 't':  // `true`
        if (next <= length - 3 && data.byteAt(next) == 'r' &&
            data.byteAt(next + 1) == 'u' && data.byteAt(next + 2) == 'e') {
          env->next = next + 3;
          *value_out = Bool::trueObj();
          return 0;
        }
        break;
      case 'f':  // `false`
        if (next <= length - 4 && data.byteAt(next) == 'a' &&
            data.byteAt(next + 1) == 'l' && data.byteAt(next + 2) == 's' &&
            data.byteAt(next + 3) == 'e') {
          env->next = next + 4;
          *value_out = Bool::falseObj();
          return 0;
        }
        break;
      case 'N':  // `NaN`
        if (next <= length - 2 && data.byteAt(next) == 'a' &&
            data.byteAt(next + 1) == 'N') {
          env->next = next + 2;
          if (env->has_parse_constant) {
            return callParseConstant(thread, env, data, 3, value_out);
          }
          *value_out = thread->runtime()->newFloat(kDoubleNaN);
          return 0;
        }
        break;
      case 'I':  // `Infinity`
        if (next <= length - 7 && data.byteAt(next) == 'n' &&
            data.byteAt(next + 1) == 'f' && data.byteAt(next + 2) == 'i' &&
            data.byteAt(next + 3) == 'n' && data.byteAt(next + 4) == 'i' &&
            data.byteAt(next + 5) == 't' && data.byteAt(next + 6) == 'y') {
          env->next = next + 7;
          if (env->has_parse_constant) {
            return callParseConstant(thread, env, data, 8, value_out);
          }
          *value_out = thread->runtime()->newFloat(kDoubleInfinity);
          return 0;
        }
        break;
      default:
        break;
    }
    DCHECK(b != ' ' && b != '\t' && b != '\r' && b != '\n',
           "whitespace not skipped");
    *value_out =
        raiseJSONDecodeError(thread, env, data, next - 1, "Expecting value");
    return -1;
  }
}

static RawObject parse(Thread* thread, JSONParser* env, const DataArray& data) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();

  Object container(&scope, NoneType::object());
  Object dict_key(&scope, NoneType::object());
  Object value(&scope, NoneType::object());
  byte b = nextNonWhitespace(thread, env, data);
  for (;;) {
    int scan_result = scan(thread, env, data, b, &value);
    switch (scan_result) {
      case 0:
        // Already have a finished object.
        b = nextNonWhitespace(thread, env, data);
        break;
      case '[':
        value = runtime->newList();
        b = nextNonWhitespace(thread, env, data);
        if (b != ']') {
          if (thread->wouldStackOverflow(kPointerSize) &&
              thread->handleInterrupt(kPointerSize)) {
            return Error::exception();
          }
          thread->stackPush(*container);
          container = *value;
          continue;
        }
        b = nextNonWhitespace(thread, env, data);
        break;
      case '{':
        value = runtime->newDict();
        b = nextNonWhitespace(thread, env, data);
        if (b != '}') {
          if (b != '"') {
            return raiseJSONDecodeError(
                thread, env, data, env->next - 1,
                "Expecting property name enclosed in double quotes");
          }
          if (thread->wouldStackOverflow(2 * kPointerSize) &&
              thread->handleInterrupt(2 * kPointerSize)) {
            return Error::exception();
          }
          thread->stackPush(*container);
          container = *value;

          value = scanString(thread, env, data);
          if (value.isErrorException()) return *value;
          b = nextNonWhitespace(thread, env, data);
          if (b != ':') {
            return raiseJSONDecodeError(thread, env, data, env->next - 1,
                                        "Expecting ':' delimiter");
          }
          b = nextNonWhitespace(thread, env, data);
          thread->stackPush(*value);
          continue;
        }
        if (env->has_object_hook) {
          value = callObjectHook(thread, env, value);
          if (value.isErrorException()) return *value;
        }
        b = nextNonWhitespace(thread, env, data);
        break;
      default:
        DCHECK(value.isErrorException(), "expected error raised");
        return *value;
    }

    for (;;) {
      // We finished reading the object `value`. Add it to the outer container
      // or return if there is no container left.

      if (container.isList()) {
        List list(&scope, *container);
        runtime->listAdd(thread, list, value);
        if (b == ',') {
          b = nextNonWhitespace(thread, env, data);
          break;
        }
        if (b == ']') {
          value = *container;
          container = thread->stackPop();
          b = nextNonWhitespace(thread, env, data);
          continue;
        }
        return raiseJSONDecodeError(thread, env, data, env->next - 1,
                                    "Expecting ',' delimiter");
      }

      if (container.isDict()) {
        Dict dict(&scope, *container);
        dict_key = thread->stackPop();
        dictAtPutByStr(thread, dict, dict_key, value);
        if (b == ',') {
          b = nextNonWhitespace(thread, env, data);
          if (b != '"') {
            return raiseJSONDecodeError(
                thread, env, data, env->next - 1,
                "Expecting property name enclosed in double quotes");
          }
          value = scanString(thread, env, data);
          if (value.isErrorException()) return *value;
          thread->stackPush(*value);
          b = nextNonWhitespace(thread, env, data);
          if (b != ':') {
            return raiseJSONDecodeError(thread, env, data, env->next - 1,
                                        "Expecting ':' delimiter");
          }
          b = nextNonWhitespace(thread, env, data);
          break;
        }
        if (b == '}') {
          value = *container;
          container = thread->stackPop();
          b = nextNonWhitespace(thread, env, data);

          if (env->has_object_hook) {
            value = callObjectHook(thread, env, value);
            if (value.isErrorException()) return *value;
          }
          continue;
        }
        return raiseJSONDecodeError(thread, env, data, env->next - 1,
                                    "Expecting ',' delimiter");
      }

      DCHECK(container.isNoneType(), "expected no container");
      if (env->next <= env->length) {
        return raiseJSONDecodeError(thread, env, data, env->next - 1,
                                    "Extra data");
      }
      return *value;
    }
  }
}

RawObject FUNC(_json, loads)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  DataArray data(&scope, runtime->emptyMutableBytes());
  Object s(&scope, args.get(static_cast<word>(LoadsArg::kString)));
  word length;
  word next = 0;
  if (runtime->isInstanceOfStr(*s)) {
    s = strUnderlying(*s);
    length = Str::cast(*s).length();
  } else if (runtime->isInstanceOfBytes(*s) ||
             runtime->isInstanceOfBytearray(*s)) {
    UNIMPLEMENTED("bytes/bytearray");
  } else {
    return thread->raiseWithFmt(
        LayoutId::kTypeError,
        "the JSON object must be str, bytes or bytearray, not %T", &s);
  }

  if (s.isSmallStr()) {
    DCHECK(length == SmallStr::cast(*s).length(), "length mismatch");
    MutableBytes copy(&scope, runtime->newMutableBytesUninitialized(length));
    copy.replaceFromWithStr(0, Str::cast(*s), length);
    data = *copy;
  } else if (s.isLargeStr()) {
    DCHECK(length == LargeStr::cast(*s).length(), "length mismatch");
    data = LargeStr::cast(*s);
  }

  Dict kw(&scope, args.get(static_cast<word>(LoadsArg::kKw)));
  Object strict_obj(&scope, dictAtById(thread, kw, ID(strict)));
  bool strict;
  bool had_strict = false;
  if (!strict_obj.isErrorNotFound()) {
    if (!runtime->isInstanceOfInt(*strict_obj)) {
      return thread->raiseRequiresType(strict_obj, ID(int));
    }
    had_strict = true;
    strict = !intUnderlying(*strict_obj).isZero();
  } else {
    strict = true;
  }

  Object cls(&scope, args.get(static_cast<word>(LoadsArg::kCls)));
  if (!cls.isNoneType() || kw.numItems() > static_cast<word>(had_strict)) {
    UNIMPLEMENTED("custom cls");
  }

  JSONParser env;
  memset(&env, 0, sizeof(env));
  env.next = next;
  env.length = length;
  env.args = args;
  env.strict = strict;

  if (!args.get(static_cast<word>(LoadsArg::kObjectHook)).isNoneType()) {
    env.has_object_hook = true;
  }
  if (!args.get(static_cast<word>(LoadsArg::kParseFloat)).isNoneType()) {
    env.has_parse_float = true;
  }
  if (!args.get(static_cast<word>(LoadsArg::kParseInt)).isNoneType()) {
    env.has_parse_int = true;
  }
  if (!args.get(static_cast<word>(LoadsArg::kParseConstant)).isNoneType()) {
    env.has_parse_constant = true;
  }
  if (!args.get(static_cast<word>(LoadsArg::kObjectPairsHook)).isNoneType()) {
    env.has_object_hook = true;
    env.has_object_pairs_hook = true;
  }
  return parse(thread, &env, data);
}

}  // namespace py
