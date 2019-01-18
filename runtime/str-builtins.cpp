#include "str-builtins.h"

#include "frame.h"
#include "globals.h"
#include "list-builtins.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"
#include "trampolines-inl.h"

namespace python {

RawObject strConcat(Thread* thread, const Str& left, const Str& right) {
  HandleScope scope;
  word left_len = left->length();
  word right_len = right->length();
  word result_len = left_len + right_len;
  // Small result
  if (result_len <= RawSmallStr::kMaxLength) {
    byte buffer[RawSmallStr::kMaxLength];
    left->copyTo(buffer, left_len);
    right->copyTo(buffer + left_len, right_len);
    return SmallStr::fromBytes(View<byte>(buffer, result_len));
  }
  // Large result
  LargeStr result(&scope,
                  thread->runtime()->heap()->createLargeStr(result_len));
  left->copyTo(reinterpret_cast<byte*>(result->address()), left_len);
  right->copyTo(reinterpret_cast<byte*>(result->address() + left_len),
                right_len);
  return *result;
}

RawObject strJoin(Thread* thread, const Str& sep, const Tuple& items,
                  word allocated) {
  HandleScope scope(thread);
  word result_len = 0;
  Runtime* runtime = thread->runtime();
  for (word i = 0; i < allocated; ++i) {
    Object elt(&scope, items->at(i));
    if (!elt->isStr() && !runtime->isInstanceOfStr(*elt)) {
      return thread->raiseTypeError(runtime->newStrFromFormat(
          "sequence item %ld: expected str instance", i));
    }
    Str str(&scope, items->at(i));
    result_len += str->length();
  }
  if (allocated > 1) {
    result_len += sep->length() * (allocated - 1);
  }
  // Small result
  if (result_len <= RawSmallStr::kMaxLength) {
    byte buffer[RawSmallStr::kMaxLength];
    for (word i = 0, offset = 0; i < allocated; ++i) {
      Str str(&scope, items->at(i));
      word str_len = str->length();
      str->copyTo(&buffer[offset], str_len);
      offset += str_len;
      if ((i + 1) < allocated) {
        word sep_len = sep->length();
        sep->copyTo(&buffer[offset], sep_len);
        offset += sep->length();
      }
    }
    return SmallStr::fromBytes(View<byte>(buffer, result_len));
  }
  // Large result
  LargeStr result(&scope, runtime->heap()->createLargeStr(result_len));
  for (word i = 0, offset = 0; i < allocated; ++i) {
    Str str(&scope, items->at(i));
    word str_len = str->length();
    str->copyTo(reinterpret_cast<byte*>(result->address() + offset), str_len);
    offset += str_len;
    if ((i + 1) < allocated) {
      word sep_len = sep->length();
      sep->copyTo(reinterpret_cast<byte*>(result->address() + offset), sep_len);
      offset += sep_len;
    }
  }
  return *result;
}

RawObject strSubstr(Thread* thread, const Str& str, word start, word length) {
  DCHECK(start >= 0, "from should be > 0");
  if (length <= 0) {
    return SmallStr::fromCStr("");
  }
  word str_len = str->length();
  DCHECK(start + length <= str_len, "overflow");
  if (start == 0 && length == str_len) {
    return *str;
  }
  // SmallStr result
  if (length <= RawSmallStr::kMaxLength) {
    byte buffer[RawSmallStr::kMaxLength];
    for (word i = 0; i < length; i++) {
      buffer[i] = str->charAt(start + i);
    }
    return SmallStr::fromBytes(View<byte>(buffer, length));
  }
  // LargeStr result
  HandleScope scope;
  LargeStr source(&scope, *str);
  LargeStr result(&scope, thread->runtime()->heap()->createLargeStr(length));
  std::memcpy(reinterpret_cast<void*>(result->address()),
              reinterpret_cast<void*>(source->address() + start), length);
  return *result;
}

word strSpan(const Str& src, const Str& str) {
  word length = src->length();
  word str_length = str->length();
  word first = 0;
  for (; first < length; first++) {
    bool has_match = false;
    byte ch = src->charAt(first);
    for (word j = 0; j < str_length; j++) {
      if (ch == str->charAt(j)) {
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
  word length = src->length();
  word str_length = str->length();
  word result = 0;
  for (word i = length - 1; i >= rend; i--, result++) {
    byte ch = src->charAt(i);
    bool has_match = false;
    for (word j = 0; j < str_length; j++) {
      if (ch == str->charAt(j)) {
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
  word length = src->length();
  if (length == 0) {
    return *src;
  }
  if (length == 1 && isAsciiSpace(src->charAt(0))) {
    return SmallStr::fromCStr("");
  }

  word first = 0;
  if (direction == StrStripDirection::Left ||
      direction == StrStripDirection::Both) {
    while (first < length && isAsciiSpace(src->charAt(first))) {
      ++first;
    }
  }

  word last = 0;
  if (direction == StrStripDirection::Right ||
      direction == StrStripDirection::Both) {
    for (word i = length - 1; i >= first && isAsciiSpace(src->charAt(i)); i--) {
      last++;
    }
  }
  return strSubstr(thread, src, first, length - first - last);
}

RawObject strStrip(Thread* thread, const Str& src, const Str& str,
                   StrStripDirection direction) {
  word length = src->length();
  if (length == 0 || str->length() == 0) {
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
  return strSubstr(thread, src, first, length - first - last);
}

RawObject strIteratorNext(Thread* thread, StrIterator& iter) {
  HandleScope scope(thread);
  word idx = iter.index();
  Str underlying(&scope, iter.str());
  if (idx >= underlying->length()) {
    return Error::object();
  }

  char item = underlying->charAt(idx);
  char buffer[] = {item, '\0'};
  iter.setIndex(idx + 1);
  return RawSmallStr::fromCStr(buffer);
}

void SmallStrBuiltins::initialize(Runtime* runtime) {
  HandleScope scope;

  Type type(&scope,
            runtime->addEmptyBuiltinType(SymbolId::kSmallStr,
                                         LayoutId::kSmallStr, LayoutId::kStr));
  type->setBuiltinBase(LayoutId::kStr);
}

const BuiltinMethod StrBuiltins::kMethods[] = {
    {SymbolId::kDunderAdd, nativeTrampoline<dunderAdd>},
    {SymbolId::kDunderEq, nativeTrampoline<dunderEq>},
    {SymbolId::kDunderGe, nativeTrampoline<dunderGe>},
    {SymbolId::kDunderGetItem, nativeTrampoline<dunderGetItem>},
    {SymbolId::kDunderGt, nativeTrampoline<dunderGt>},
    {SymbolId::kDunderIter, nativeTrampoline<dunderIter>},
    {SymbolId::kDunderLe, nativeTrampoline<dunderLe>},
    {SymbolId::kDunderLen, nativeTrampoline<dunderLen>},
    {SymbolId::kDunderLt, nativeTrampoline<dunderLt>},
    {SymbolId::kDunderMod, nativeTrampoline<dunderMod>},
    {SymbolId::kDunderNe, nativeTrampoline<dunderNe>},
    {SymbolId::kDunderNew, nativeTrampoline<dunderNew>},
    {SymbolId::kDunderRepr, nativeTrampoline<dunderRepr>},
    {SymbolId::kJoin, nativeTrampoline<join>},
    {SymbolId::kLower, nativeTrampoline<lower>},
    {SymbolId::kLStrip, nativeTrampoline<lstrip>},
    {SymbolId::kRStrip, nativeTrampoline<rstrip>},
    {SymbolId::kStrip, nativeTrampoline<strip>},
};

void StrBuiltins::initialize(Runtime* runtime) {
  HandleScope scope;
  Type type(&scope,
            runtime->addBuiltinTypeWithMethods(SymbolId::kStr, LayoutId::kStr,
                                               LayoutId::kObject, kMethods));

  Type largestr_type(
      &scope, runtime->addEmptyBuiltinType(
                  SymbolId::kLargeStr, LayoutId::kLargeStr, LayoutId::kStr));
  largestr_type->setBuiltinBase(LayoutId::kStr);
}

RawObject StrBuiltins::dunderAdd(Thread* thread, Frame* frame, word nargs) {
  if (nargs == 0) {
    return thread->raiseTypeErrorWithCStr("str.__add__ needs an argument");
  }
  if (nargs != 2) {
    return thread->raiseTypeError(thread->runtime()->newStrFromFormat(
        "expected 1 arguments, got %ld", nargs - 1));
  }
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  Object other(&scope, args.get(1));
  if (!runtime->isInstanceOfStr(self)) {
    return thread->raiseTypeErrorWithCStr("str.__add__ requires a str object");
  }
  if (!runtime->isInstanceOfStr(other)) {
    return thread->raiseTypeErrorWithCStr("can only concatenate str to str");
  }
  if (!self->isStr()) {
    UNIMPLEMENTED("Strict subclass of string");
  }
  if (!other->isStr()) {
    UNIMPLEMENTED("Strict subclass of string");
  }
  Str self_str(&scope, *self);
  Str other_str(&scope, *other);
  return strConcat(thread, self_str, other_str);
}

RawObject StrBuiltins::dunderEq(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("expected 1 argument");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Object other(&scope, args.get(1));
  if (self->isStr() && other->isStr()) {
    return Bool::fromBool(RawStr::cast(self)->compare(other) == 0);
  }
  // TODO(cshapiro): handle user-defined subtypes of string.
  return thread->runtime()->notImplemented();
}

RawObject StrBuiltins::dunderGe(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("expected 1 argument");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Object other(&scope, args.get(1));
  if (self->isStr() && other->isStr()) {
    return Bool::fromBool(RawStr::cast(self)->compare(other) >= 0);
  }
  // TODO(cshapiro): handle user-defined subtypes of string.
  return thread->runtime()->notImplemented();
}

RawObject StrBuiltins::dunderGt(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("expected 1 argument");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Object other(&scope, args.get(1));
  if (self->isStr() && other->isStr()) {
    return Bool::fromBool(RawStr::cast(self)->compare(other) > 0);
  }
  // TODO(cshapiro): handle user-defined subtypes of string.
  return thread->runtime()->notImplemented();
}

RawObject StrBuiltins::join(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("expected 1 argument");
  }
  Runtime* runtime = thread->runtime();
  Arguments args(frame, nargs);
  if (!runtime->isInstanceOfStr(args.get(0))) {
    return thread->raiseTypeErrorWithCStr("'join' requires a 'str' object");
  }
  HandleScope scope(thread);
  Str sep(&scope, args.get(0));
  Object iterable(&scope, args.get(1));
  // Tuples of strings
  if (iterable->isTuple()) {
    Tuple tuple(&scope, *iterable);
    return strJoin(thread, sep, tuple, tuple->length());
  }
  // Lists of strings
  if (iterable->isList()) {
    List list(&scope, *iterable);
    Tuple tuple(&scope, list->items());
    return strJoin(thread, sep, tuple, list->numItems());
  }
  // Iterators of strings
  List list(&scope, runtime->newList());
  listExtend(thread, list, iterable);
  Tuple tuple(&scope, list->items());
  return strJoin(thread, sep, tuple, list->numItems());
}

RawObject StrBuiltins::dunderLe(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("expected 1 argument");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Object other(&scope, args.get(1));
  if (self->isStr() && other->isStr()) {
    return Bool::fromBool(RawStr::cast(self)->compare(other) <= 0);
  }
  // TODO(cshapiro): handle user-defined subtypes of string.
  return thread->runtime()->notImplemented();
}

RawObject StrBuiltins::dunderLen(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("expected 0 arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (self->isStr()) {
    // TODO(T33085486): __len__ for unicode should return number of code points,
    // not bytes
    return SmallInt::fromWord(RawStr::cast(*self)->length());
  }
  return thread->raiseTypeErrorWithCStr(
      "descriptor '__len__' requires a 'str' object");
}

RawObject StrBuiltins::lower(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("expected 0 arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object obj(&scope, args.get(0));
  if (!obj->isStr()) {
    return thread->raiseTypeErrorWithCStr("str.lower(self): self is not a str");
  }
  Str self(&scope, *obj);
  byte* buf = new byte[self->length()];
  for (word i = 0; i < self->length(); i++) {
    byte c = self->charAt(i);
    // TODO(dulinr): Handle UTF-8 code points that need to have their case
    // changed.
    if (c >= 'A' && c <= 'Z') {
      buf[i] = c - 'A' + 'a';
    } else {
      buf[i] = c;
    }
  }
  Str result(&scope,
             thread->runtime()->newStrWithAll(View<byte>{buf, self->length()}));
  delete[] buf;
  return *result;
}

RawObject StrBuiltins::dunderLt(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("expected 1 argument");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Object other(&scope, args.get(1));
  if (self->isStr() && other->isStr()) {
    return Bool::fromBool(RawStr::cast(self)->compare(other) < 0);
  }
  // TODO(cshapiro): handle user-defined subtypes of string.
  return thread->runtime()->notImplemented();
}

RawObject StrBuiltins::strFormatBufferLength(Thread* thread, const Str& fmt,
                                             const Tuple& args) {
  word arg_idx = 0;
  word len = 0;
  for (word fmt_idx = 0; fmt_idx < fmt->length(); fmt_idx++, len++) {
    byte ch = fmt->charAt(fmt_idx);
    if (ch != '%') {
      continue;
    }
    if (++fmt_idx >= fmt->length()) {
      return thread->raiseValueErrorWithCStr("Incomplete format");
    }
    switch (fmt->charAt(fmt_idx)) {
      case 'd': {
        len--;
        if (!args->at(arg_idx)->isInt()) {
          return thread->raiseTypeErrorWithCStr("Argument mismatch");
        }
        len += snprintf(nullptr, 0, "%ld",
                        RawInt::cast(args->at(arg_idx))->asWord());
        arg_idx++;
      } break;
      case 'g': {
        len--;
        if (!args->at(arg_idx)->isFloat()) {
          return thread->raiseTypeErrorWithCStr("Argument mismatch");
        }
        len += snprintf(nullptr, 0, "%g",
                        RawFloat::cast(args->at(arg_idx))->value());
        arg_idx++;
      } break;
      case 's': {
        len--;
        if (!args->at(arg_idx)->isStr()) {
          return thread->raiseTypeErrorWithCStr("Argument mismatch");
        }
        len += RawStr::cast(args->at(arg_idx))->length();
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
  for (word fmt_idx = 0; fmt_idx < fmt->length(); fmt_idx++) {
    byte ch = fmt->charAt(fmt_idx);
    if (ch != '%') {
      dst[dst_idx++] = ch;
      continue;
    }
    switch (fmt->charAt(++fmt_idx)) {
      case 'd': {
        word value = RawInt::cast(args->at(arg_idx++))->asWord();
        dst_idx += snprintf(&dst[dst_idx], len - dst_idx + 1, "%ld", value);
      } break;
      case 'g': {
        double value = RawFloat::cast(args->at(arg_idx++))->value();
        dst_idx += snprintf(&dst[dst_idx], len - dst_idx + 1, "%g", value);
      } break;
      case 's': {
        RawStr value = RawStr::cast(args->at(arg_idx));
        value->copyTo(reinterpret_cast<byte*>(&dst[dst_idx]), value->length());
        dst_idx += value->length();
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
  if (fmt->length() == 0) {
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
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("expected 1 argument");
  }
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(caller, nargs);
  Object self(&scope, args.get(0));
  Object other(&scope, args.get(1));
  if (self->isStr()) {
    Str format(&scope, *self);
    Tuple format_args(&scope, runtime->newTuple(0));
    if (other->isTuple()) {
      format_args = *other;
    } else {
      Tuple tuple(&scope, runtime->newTuple(1));
      tuple->atPut(0, *other);
      format_args = *tuple;
    }
    return strFormat(thread, format, format_args);
  }
  // TODO(cshapiro): handle user-defined subtypes of string.
  return runtime->notImplemented();
}

RawObject StrBuiltins::dunderNe(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("expected 1 argument");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Object other(&scope, args.get(1));
  if (self->isStr() && other->isStr()) {
    return Bool::fromBool(RawStr::cast(self)->compare(other) != 0);
  }
  // TODO(cshapiro): handle user-defined subtypes of string.
  return thread->runtime()->notImplemented();
}

RawObject StrBuiltins::dunderNew(Thread* thread, Frame* frame, word nargs) {
  if (nargs == 0) {
    return thread->raiseTypeErrorWithCStr(
        "str.__new__(): not enough arguments");
  }
  if (nargs > 4) {
    return thread->raiseTypeErrorWithCStr(
        "str() takes at most three arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object type_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfType(type_obj)) {
    return thread->raiseTypeErrorWithCStr(
        "str.__new__(X): X is not a type object");
  }
  Type type(&scope, *type_obj);
  if (type->builtinBase() != LayoutId::kStr) {
    return thread->raiseTypeErrorWithCStr(
        "str.__new__(X): X is not a subtype of str");
  }
  Layout layout(&scope, type->instanceLayout());
  if (layout->id() != LayoutId::kStr) {
    // TODO(T36406531): Implement __new__ with subtypes of str.
    UNIMPLEMENTED("str.__new__(<subtype of str>, ...)");
  }
  if (nargs == 1) {
    // No argument to str, return empty string.
    return runtime->newStrFromCStr("");
  }
  if (nargs > 2) {
    UNIMPLEMENTED("str() with encoding");
  }
  // Only one argument, the value to be stringified.
  Object arg(&scope, args.get(1));
  // If it's already exactly a string, return it immediately.
  if (arg->isStr()) {
    return *arg;
  }
  // If it's not exactly a string, call its __str__.
  Object method(&scope, Interpreter::lookupMethod(thread, frame, arg,
                                                  SymbolId::kDunderStr));
  DCHECK(!method->isError(),
         "No __str__ found on the object even though everything inherits one");
  Object ret(&scope, Interpreter::callMethod1(thread, frame, method, arg));
  if (!ret->isError() && !runtime->isInstanceOfStr(ret)) {
    return thread->raiseTypeErrorWithCStr("__str__ returned non-string");
  }
  return *ret;
}

RawObject StrBuiltins::slice(Thread* thread, const Str& str,
                             const Slice& slice) {
  word start, stop, step;
  slice->unpack(&start, &stop, &step);
  word length = Slice::adjustIndices(str->length(), &start, &stop, step);
  std::unique_ptr<char[]> buf(new char[length + 1]);
  buf[length] = '\0';
  for (word i = 0, index = start; i < length; i++, index += step) {
    buf[i] = str->charAt(index);
  }
  return thread->runtime()->newStrFromCStr(buf.get());
}

RawObject StrBuiltins::dunderGetItem(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("expected 1 argument");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfStr(*self)) {
    return thread->raiseTypeErrorWithCStr(
        "'__getitem__' requires a 'str' instance");
  }

  // TODO(T36619828): strict subclass of str
  Str string(&scope, *self);
  Object index(&scope, args.get(1));
  if (runtime->isInstanceOfInt(*index)) {
    // TODO(T38780562): strict subclass of int
    if (!index->isSmallInt()) {
      return thread->raiseIndexErrorWithCStr(
          "cannot fit index into an index-sized integer");
    }
    word idx = RawSmallInt::cast(index)->value();
    if (idx < 0) {
      idx += string->length();
    }
    if (idx < 0 || idx >= string->length()) {
      return thread->raiseIndexErrorWithCStr("string index out of range");
    }
    byte c = string->charAt(idx);
    return SmallStr::fromBytes(View<byte>(&c, 1));
  }
  if (index->isSlice()) {
    Slice str_slice(&scope, *index);
    return slice(thread, string, str_slice);
  }
  // TODO(T27897506): use __index__ to get index
  return thread->raiseTypeErrorWithCStr(
      "string indices must be integers or slices");
}

RawObject StrBuiltins::dunderIter(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("__iter__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self->isStr()) {
    if (thread->runtime()->isInstanceOfStr(self)) {
      UNIMPLEMENTED("str.__iter__(<subtype of str>)");
    }
    return thread->raiseTypeErrorWithCStr(
        "__iter__() must be called with a str instance as the first argument");
  }
  return thread->runtime()->newStrIterator(self);
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
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("expected 0 arguments");
  }
  Runtime* runtime = thread->runtime();
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object obj(&scope, args.get(0));
  if (!runtime->isInstanceOfStr(obj)) {
    return thread->raiseTypeErrorWithCStr(
        "str.__repr__(self): self is not a str");
  }
  if (!obj->isStr()) {
    UNIMPLEMENTED("Strict subclass of string");
  }
  Str self(&scope, *obj);
  const word self_len = self->length();
  word output_size = 0;
  word squote = 0;
  word dquote = 0;
  // Precompute the size so that only one string allocation is necessary.
  for (word i = 0; i < self_len; ++i) {
    word incr = 1;
    byte ch = self->charAt(i);
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
    self->copyTo(buf.get() + 1, self_len);
  } else {
    byte* curr = buf.get() + 1;
    for (word i = 0; i < self_len; ++i) {
      byte ch = self->charAt(i);
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
  if (nargs == 0) {
    return thread->raiseTypeErrorWithCStr("str.lstrip() needs an argument");
  }
  if (nargs > 2) {
    return thread->raiseTypeError(thread->runtime()->newStrFromFormat(
        "str.lstrip() takes at most 1 argument (%ld given)", nargs - 1));
  }
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  if (!runtime->isInstanceOfStr(self)) {
    return thread->raiseTypeErrorWithCStr("str.lstrip() requires a str object");
  }
  if (!self->isStr()) {
    UNIMPLEMENTED("Strict subclass of string");
  }
  Str str(&scope, *self);
  if (nargs == 1) {
    return strStripSpace(thread, str, StrStripDirection::Left);
  }
  // nargs == 2
  Object other(&scope, args.get(1));
  if (other->isNoneType()) {
    return strStripSpace(thread, str, StrStripDirection::Left);
  }
  if (!runtime->isInstanceOfStr(other)) {
    return thread->raiseTypeErrorWithCStr(
        "str.lstrip() arg must be None or str");
  }
  if (!other->isStr()) {
    UNIMPLEMENTED("Strict subclass of string");
  }
  Str chars(&scope, *other);
  return strStrip(thread, str, chars, StrStripDirection::Left);
}

RawObject StrBuiltins::rstrip(Thread* thread, Frame* frame, word nargs) {
  if (nargs == 0) {
    return thread->raiseTypeErrorWithCStr("str.rstrip() needs an argument");
  }
  if (nargs > 2) {
    return thread->raiseTypeError(thread->runtime()->newStrFromFormat(
        "str.rstrip() takes at most 1 argument (%ld given)", nargs - 1));
  }
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  if (!runtime->isInstanceOfStr(self)) {
    return thread->raiseTypeErrorWithCStr("str.rstrip() requires a str object");
  }
  if (!self->isStr()) {
    UNIMPLEMENTED("Strict subclass of string");
  }
  Str str(&scope, *self);
  if (nargs == 1) {
    return strStripSpace(thread, str, StrStripDirection::Right);
  }
  // nargs == 2
  Object other(&scope, args.get(1));
  if (other->isNoneType()) {
    return strStripSpace(thread, str, StrStripDirection::Right);
  }
  if (!runtime->isInstanceOfStr(other)) {
    return thread->raiseTypeErrorWithCStr(
        "str.rstrip() arg must be None or str");
  }
  if (!other->isStr()) {
    UNIMPLEMENTED("Strict subclass of string");
  }
  Str chars(&scope, *other);
  return strStrip(thread, str, chars, StrStripDirection::Right);
}

RawObject StrBuiltins::strip(Thread* thread, Frame* frame, word nargs) {
  if (nargs == 0) {
    return thread->raiseTypeErrorWithCStr("str.strip() needs an argument");
  }
  if (nargs > 2) {
    return thread->raiseTypeError(thread->runtime()->newStrFromFormat(
        "str.strip() takes at most 1 argument (%ld given)", nargs - 1));
  }
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  if (!runtime->isInstanceOfStr(self)) {
    return thread->raiseTypeErrorWithCStr("str.strip() requires a str object");
  }
  if (!self->isStr()) {
    UNIMPLEMENTED("Strict subclass of string");
  }
  Str str(&scope, *self);
  if (nargs == 1) {
    return strStripSpace(thread, str, StrStripDirection::Both);
  }
  // nargs == 2
  Object other(&scope, args.get(1));
  if (other->isNoneType()) {
    return strStripSpace(thread, str, StrStripDirection::Both);
  }
  if (!runtime->isInstanceOfStr(other)) {
    return thread->raiseTypeErrorWithCStr(
        "str.strip() arg must be None or str");
  }
  if (!other->isStr()) {
    UNIMPLEMENTED("Strict subclass of string");
  }
  Str chars(&scope, *other);
  return strStrip(thread, str, chars, StrStripDirection::Both);
}

const BuiltinMethod StrIteratorBuiltins::kMethods[] = {
    {SymbolId::kDunderIter, nativeTrampoline<dunderIter>},
    {SymbolId::kDunderNext, nativeTrampoline<dunderNext>},
    {SymbolId::kDunderLengthHint, nativeTrampoline<dunderLengthHint>}};

void StrIteratorBuiltins::initialize(Runtime* runtime) {
  HandleScope scope;
  Type str_iter(&scope, runtime->addBuiltinTypeWithMethods(
                            SymbolId::kStrIterator, LayoutId::kStrIterator,
                            LayoutId::kObject, kMethods));
}

RawObject StrIteratorBuiltins::dunderIter(Thread* thread, Frame* frame,
                                          word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("__iter__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self->isStrIterator()) {
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
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("__next__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self->isStrIterator()) {
    return thread->raiseTypeErrorWithCStr(
        "__next__() must be called with a str iterator instance as the first "
        "argument");
  }
  StrIterator iter(&scope, *self);
  Object value(&scope, strIteratorNext(thread, iter));
  if (value->isError()) {
    return thread->raiseStopIteration(NoneType::object());
  }
  return *value;
}

RawObject StrIteratorBuiltins::dunderLengthHint(Thread* thread, Frame* frame,
                                                word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr(
        "__length_hint__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self->isStrIterator()) {
    return thread->raiseTypeErrorWithCStr(
        "__length_hint__() must be called with a str iterator instance as the "
        "first argument");
  }
  StrIterator str_iterator(&scope, *self);
  Str str(&scope, str_iterator->str());
  return SmallInt::fromWord(str->length() - str_iterator->index());
}

}  // namespace python
