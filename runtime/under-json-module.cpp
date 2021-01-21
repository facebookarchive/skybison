#include "builtins.h"
#include "dict-builtins.h"
#include "handles.h"
#include "objects.h"
#include "runtime.h"
#include "str-builtins.h"
#include "thread.h"
#include "unicode.h"
#include "utils.h"

namespace py {

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
  bool has_parse_constant;
  bool strict;
};

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

static RawObject scanString(Thread* thread, JSONParser* env,
                            const DataArray& data) {
  word next = env->next;
  word length = env->length;
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
    DCHECK(b == '\\', "Expected backslash");
    UNIMPLEMENTED("escape sequences");
  }
  env->next = next;
  return dataArraySubstr(thread, data, segment_begin, segment_length);
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
      case '9':
        UNIMPLEMENTED("scanNumber");
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

  Object value(&scope, NoneType::object());
  byte b = nextNonWhitespace(thread, env, data);
  int scan_result = scan(thread, env, data, b, &value);
  switch (scan_result) {
    case 0:
      // Already have a finished object.
      b = nextNonWhitespace(thread, env, data);
      break;
    case '[':
      UNIMPLEMENTED("lists");
    case '{':
      UNIMPLEMENTED("dictionaries");
    default:
      DCHECK(value.isErrorException(), "expected error raised");
      return *value;
  }

  if (env->next <= env->length) {
    return raiseJSONDecodeError(thread, env, data, env->next - 1, "Extra data");
  }
  return *value;
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
    UNIMPLEMENTED("object hook");
  }
  if (!args.get(static_cast<word>(LoadsArg::kParseFloat)).isNoneType()) {
    UNIMPLEMENTED("parse float hook");
  }
  if (!args.get(static_cast<word>(LoadsArg::kParseInt)).isNoneType()) {
    UNIMPLEMENTED("parse int hook");
  }
  if (!args.get(static_cast<word>(LoadsArg::kParseConstant)).isNoneType()) {
    env.has_parse_constant = true;
  }
  if (!args.get(static_cast<word>(LoadsArg::kObjectPairsHook)).isNoneType()) {
    UNIMPLEMENTED("object pairs hook");
  }
  return parse(thread, &env, data);
}

}  // namespace py
