#include "str-builtins.h"

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"
#include "trampolines-inl.h"

namespace python {

const BuiltinMethod StrBuiltins::kMethods[] = {
    {SymbolId::kDunderAdd, nativeTrampoline<dunderAdd>},
    {SymbolId::kDunderEq, nativeTrampoline<dunderEq>},
    {SymbolId::kDunderGe, nativeTrampoline<dunderGe>},
    {SymbolId::kDunderGetItem, nativeTrampoline<dunderGetItem>},
    {SymbolId::kDunderGt, nativeTrampoline<dunderGt>},
    {SymbolId::kJoin, nativeTrampoline<join>},
    {SymbolId::kDunderLe, nativeTrampoline<dunderLe>},
    {SymbolId::kDunderLen, nativeTrampoline<dunderLen>},
    {SymbolId::kLower, nativeTrampoline<lower>},
    {SymbolId::kDunderLt, nativeTrampoline<dunderLt>},
    {SymbolId::kDunderMod, nativeTrampoline<dunderMod>},
    {SymbolId::kDunderNe, nativeTrampoline<dunderNe>},
    {SymbolId::kDunderNew, nativeTrampoline<dunderNew>},
    {SymbolId::kDunderRepr, nativeTrampoline<dunderRepr>},
};

void StrBuiltins::initialize(Runtime* runtime) {
  HandleScope scope;
  Handle<Type> type(&scope,
                    runtime->addBuiltinClass(SymbolId::kStr, LayoutId::kStr,
                                             LayoutId::kObject, kMethods));
  type->setFlag(Type::Flag::kStrSubclass);
}

Object* StrBuiltins::dunderAdd(Thread* thread, Frame* frame, word nargs) {
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
  Handle<Object> self(&scope, args.get(0));
  Handle<Object> other(&scope, args.get(1));
  if (!runtime->hasSubClassFlag(*self, Type::Flag::kStrSubclass)) {
    return thread->raiseTypeErrorWithCStr("str.__add__ requires a str object");
  }
  if (!runtime->hasSubClassFlag(*other, Type::Flag::kStrSubclass)) {
    return thread->raiseTypeErrorWithCStr("can only concatenate str to str");
  }
  if (!self->isStr()) {
    UNIMPLEMENTED("Strict subclass of string");
  }
  if (!other->isStr()) {
    UNIMPLEMENTED("Strict subclass of string");
  }
  Handle<Str> self_str(&scope, *self);
  Handle<Str> other_str(&scope, *other);
  return runtime->strConcat(self_str, other_str);
}

Object* StrBuiltins::dunderEq(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("expected 1 argument");
  }
  Arguments args(frame, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isStr() && other->isStr()) {
    return Bool::fromBool(Str::cast(self)->compare(other) == 0);
  }
  // TODO(cshapiro): handle user-defined subtypes of string.
  return thread->runtime()->notImplemented();
}

Object* StrBuiltins::dunderGe(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("expected 1 argument");
  }
  Arguments args(frame, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isStr() && other->isStr()) {
    return Bool::fromBool(Str::cast(self)->compare(other) >= 0);
  }
  // TODO(cshapiro): handle user-defined subtypes of string.
  return thread->runtime()->notImplemented();
}

Object* StrBuiltins::dunderGt(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("expected 1 argument");
  }
  Arguments args(frame, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isStr() && other->isStr()) {
    return Bool::fromBool(Str::cast(self)->compare(other) > 0);
  }
  // TODO(cshapiro): handle user-defined subtypes of string.
  return thread->runtime()->notImplemented();
}

Object* StrBuiltins::join(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("expected 1 argument");
  }
  Runtime* runtime = thread->runtime();
  Arguments args(frame, nargs);
  if (!runtime->hasSubClassFlag(args.get(0), Type::Flag::kStrSubclass)) {
    return thread->raiseTypeErrorWithCStr("'join' requires a 'str' object");
  }
  HandleScope scope(thread);
  Handle<Str> sep(&scope, args.get(0));
  Handle<Object> iterable(&scope, args.get(1));
  // Tuples of strings
  if (iterable->isObjectArray()) {
    Handle<ObjectArray> tuple(&scope, *iterable);
    return thread->runtime()->strJoin(thread, sep, tuple, tuple->length());
  }
  // Lists of strings
  if (iterable->isList()) {
    Handle<List> list(&scope, *iterable);
    Handle<ObjectArray> tuple(&scope, list->items());
    return thread->runtime()->strJoin(thread, sep, tuple, list->allocated());
  }
  // Iterators of strings
  Handle<List> list(&scope, runtime->newList());
  runtime->listExtend(thread, list, iterable);
  Handle<ObjectArray> tuple(&scope, list->items());
  return thread->runtime()->strJoin(thread, sep, tuple, list->allocated());
}

Object* StrBuiltins::dunderLe(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("expected 1 argument");
  }
  Arguments args(frame, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isStr() && other->isStr()) {
    return Bool::fromBool(Str::cast(self)->compare(other) <= 0);
  }
  // TODO(cshapiro): handle user-defined subtypes of string.
  return thread->runtime()->notImplemented();
}

Object* StrBuiltins::dunderLen(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("expected 0 arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Handle<Object> self(&scope, args.get(0));
  if (self->isStr()) {
    // TODO(T33085486): __len__ for unicode should return number of code points,
    // not bytes
    return SmallInt::fromWord(Str::cast(*self)->length());
  }
  return thread->raiseTypeErrorWithCStr(
      "descriptor '__len__' requires a 'str' object");
}

Object* StrBuiltins::lower(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("expected 0 arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Handle<Object> obj(&scope, args.get(0));
  if (!obj->isStr()) {
    return thread->raiseTypeErrorWithCStr("str.lower(self): self is not a str");
  }
  Handle<Str> self(&scope, *obj);
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
  Handle<Str> result(&scope, thread->runtime()->newStrWithAll(
                                 View<byte>{buf, self->length()}));
  delete[] buf;
  return *result;
}

Object* StrBuiltins::dunderLt(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("expected 1 argument");
  }
  Arguments args(frame, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isStr() && other->isStr()) {
    return Bool::fromBool(Str::cast(self)->compare(other) < 0);
  }
  // TODO(cshapiro): handle user-defined subtypes of string.
  return thread->runtime()->notImplemented();
}

word StrBuiltins::strFormatBufferLength(const Handle<Str>& fmt,
                                        const Handle<ObjectArray>& args) {
  word arg_idx = 0;
  word len = 0;
  for (word fmt_idx = 0; fmt_idx < fmt->length(); fmt_idx++, len++) {
    byte ch = fmt->charAt(fmt_idx);
    if (ch != '%') {
      continue;
    }
    CHECK(++fmt_idx < fmt->length(), "Incomplete format");
    switch (fmt->charAt(fmt_idx)) {
      case 'd': {
        len--;
        CHECK(args->at(arg_idx)->isInt(), "Argument mismatch");
        len +=
            snprintf(nullptr, 0, "%ld", Int::cast(args->at(arg_idx))->asWord());
        arg_idx++;
      } break;
      case 'g': {
        len--;
        CHECK(args->at(arg_idx)->isFloat(), "Argument mismatch");
        len +=
            snprintf(nullptr, 0, "%g", Float::cast(args->at(arg_idx))->value());
        arg_idx++;
      } break;
      case 's': {
        len--;
        CHECK(args->at(arg_idx)->isStr(), "Argument mismatch");
        len += Str::cast(args->at(arg_idx))->length();
        arg_idx++;
      } break;
      case '%':
        break;
      default:
        UNIMPLEMENTED("Unsupported format specifier");
    }
  }
  return len;
}

static void stringFormatToBuffer(const Handle<Str>& fmt,
                                 const Handle<ObjectArray>& args, char* dst,
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
        word value = Int::cast(args->at(arg_idx++))->asWord();
        dst_idx += snprintf(&dst[dst_idx], len - dst_idx + 1, "%ld", value);
      } break;
      case 'g': {
        double value = Float::cast(args->at(arg_idx++))->value();
        dst_idx += snprintf(&dst[dst_idx], len - dst_idx + 1, "%g", value);
      } break;
      case 's': {
        Str* value = Str::cast(args->at(arg_idx));
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

Object* StrBuiltins::strFormat(Thread* thread, const Handle<Str>& fmt,
                               const Handle<ObjectArray>& args) {
  if (fmt->length() == 0) {
    return *fmt;
  }
  word len = strFormatBufferLength(fmt, args);
  char* dst = static_cast<char*>(std::malloc(len + 1));
  CHECK(dst != nullptr, "Buffer allocation failure");
  stringFormatToBuffer(fmt, args, dst, len);
  Object* result = thread->runtime()->newStrFromCStr(dst);
  std::free(dst);
  return result;
}

Object* StrBuiltins::dunderMod(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("expected 1 argument");
  }
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(caller, nargs);
  Handle<Object> self(&scope, args.get(0));
  Handle<Object> other(&scope, args.get(1));
  if (self->isStr()) {
    Handle<Str> format(&scope, *self);
    Handle<ObjectArray> format_args(&scope, runtime->newObjectArray(0));
    if (other->isObjectArray()) {
      format_args = *other;
    } else {
      Handle<ObjectArray> tuple(&scope, runtime->newObjectArray(1));
      tuple->atPut(0, *other);
      format_args = *tuple;
    }
    return strFormat(thread, format, format_args);
  }
  // TODO(cshapiro): handle user-defined subtypes of string.
  return runtime->notImplemented();
}

Object* StrBuiltins::dunderNe(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("expected 1 argument");
  }
  Arguments args(frame, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isStr() && other->isStr()) {
    return Bool::fromBool(Str::cast(self)->compare(other) != 0);
  }
  // TODO(cshapiro): handle user-defined subtypes of string.
  return thread->runtime()->notImplemented();
}

Object* StrBuiltins::dunderNew(Thread* thread, Frame* frame, word nargs) {
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
  Handle<Object> type(&scope, args.get(0));
  if (!runtime->hasSubClassFlag(*type, Type::Flag::kTypeSubclass)) {
    return thread->raiseTypeErrorWithCStr(
        "str.__new__(X): X is not a type object");
  }
  if (!Type::cast(*type)->hasFlag(Type::Flag::kStrSubclass)) {
    return thread->raiseTypeErrorWithCStr(
        "str.__new__(X): X is not a subtype of str");
  }
  Handle<Layout> layout(&scope, Type::cast(*type)->instanceLayout());
  if (layout->id() != LayoutId::kStr) {
    // TODO: Implement __new__ with subtypes of str.
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
  Handle<Object> arg(&scope, args.get(1));
  // If it's already exactly a string, return it immediately.
  if (arg->isStr()) {
    return *arg;
  }
  // If it's not exactly a string, call its __str__.
  Handle<Object> method(&scope, Interpreter::lookupMethod(
                                    thread, frame, arg, SymbolId::kDunderStr));
  DCHECK(!method->isError(),
         "No __str__ found on the object even though everything inherits one");
  Object* ret = Interpreter::callMethod1(thread, frame, method, arg);
  if (!ret->isError() &&
      !runtime->hasSubClassFlag(ret, Type::Flag::kStrSubclass)) {
    return thread->raiseTypeErrorWithCStr("__str__ returned non-string");
  }
  return ret;
}

Object* StrBuiltins::dunderGetItem(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("expected 1 argument");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Handle<Object> self(&scope, args.get(0));

  if (self->isStr()) {
    Handle<Str> string(&scope, *self);
    Object* index = args.get(1);
    if (index->isSmallInt()) {
      word idx = SmallInt::cast(index)->value();
      if (idx < 0) {
        idx = string->length() - idx;
      }
      if (idx < 0 || idx >= string->length()) {
        return thread->raiseIndexErrorWithCStr("string index out of range");
      }
      byte c = string->charAt(idx);
      return SmallStr::fromBytes(View<byte>(&c, 1));
    }
    if (index->isSlice()) {
      Handle<Slice> slice(&scope, index);
      word start, stop, step;
      slice->unpack(&start, &stop, &step);
      word length = Slice::adjustIndices(string->length(), &start, &stop, step);
      byte* buf = new byte[length];
      byte* curr = buf;
      for (word i = start; i < stop; i += step) {
        *curr++ = string->charAt(i);
      }
      Handle<Str> result(
          &scope, thread->runtime()->newStrWithAll(View<byte>{buf, length}));
      delete[] buf;
      return *result;
    }
    return thread->raiseTypeErrorWithCStr(
        "string indices must be integers or slices");
  }
  // TODO(jeethu): handle user-defined subtypes of string.
  return thread->raiseTypeErrorWithCStr(
      "__getitem__() must be called with a string instance as the first "
      "argument");
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

Object* StrBuiltins::dunderRepr(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("expected 0 arguments");
  }
  Runtime* runtime = thread->runtime();
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Handle<Object> obj(&scope, args.get(0));
  if (!runtime->hasSubClassFlag(*obj, Type::Flag::kStrSubclass)) {
    return thread->raiseTypeErrorWithCStr(
        "str.__repr__(self): self is not a str");
  }
  if (!obj->isStr()) {
    UNIMPLEMENTED("Strict subclass of string");
  }
  Handle<Str> self(&scope, *obj);
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

  byte* buf = new byte[output_size];
  // Write in the quotes.
  buf[0] = quote;
  buf[output_size - 1] = quote;
  if (unchanged) {
    // Rest of the characters were all unmodified, copy them directly into the
    // buffer.
    self->copyTo(buf + 1, self_len);
  } else {
    byte* curr = buf + 1;
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
    DCHECK(curr == buf + output_size - 1,
           "Didn't write the correct number of characters out");
  }
  Handle<Str> output(&scope,
                     runtime->newStrWithAll(View<byte>{buf, output_size}));
  delete[] buf;
  return *output;
}

}  // namespace python
