#include "str-builtins.h"

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

Object* builtinStringAdd(Thread* thread, Frame* frame, word nargs) {
  if (nargs == 0) {
    return thread->throwTypeErrorFromCString("str.__add__ needs an argument");
  }
  if (nargs != 2) {
    return thread->throwTypeError(thread->runtime()->newStringFromFormat(
        "expected 1 arguments, got %ld", nargs - 1));
  }
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Handle<Object> self(&scope, args.get(0));
  Handle<Object> other(&scope, args.get(1));
  if (!runtime->hasSubClassFlag(*self, Type::Flag::kStrSubclass)) {
    return thread->throwTypeErrorFromCString(
        "str.__add__ requires a str object");
  }
  if (!runtime->hasSubClassFlag(*other, Type::Flag::kStrSubclass)) {
    return thread->throwTypeErrorFromCString("can only concatenate str to str");
  }
  if (!self->isString()) {
    UNIMPLEMENTED("Strict subclass of string");
  }
  if (!other->isString()) {
    UNIMPLEMENTED("Strict subclass of string");
  }
  Handle<String> self_str(&scope, *self);
  Handle<String> other_str(&scope, *other);
  return runtime->stringConcat(self_str, other_str);
}

Object* builtinStringEq(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(frame, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isString() && other->isString()) {
    return Bool::fromBool(String::cast(self)->compare(other) == 0);
  }
  // TODO(cshapiro): handle user-defined subtypes of string.
  return thread->runtime()->notImplemented();
}

Object* builtinStringGe(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(frame, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isString() && other->isString()) {
    return Bool::fromBool(String::cast(self)->compare(other) >= 0);
  }
  // TODO(cshapiro): handle user-defined subtypes of string.
  return thread->runtime()->notImplemented();
}

Object* builtinStringGt(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(frame, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isString() && other->isString()) {
    return Bool::fromBool(String::cast(self)->compare(other) > 0);
  }
  // TODO(cshapiro): handle user-defined subtypes of string.
  return thread->runtime()->notImplemented();
}

Object* builtinStringLe(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(frame, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isString() && other->isString()) {
    return Bool::fromBool(String::cast(self)->compare(other) <= 0);
  }
  // TODO(cshapiro): handle user-defined subtypes of string.
  return thread->runtime()->notImplemented();
}

Object* builtinStringLt(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(frame, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isString() && other->isString()) {
    return Bool::fromBool(String::cast(self)->compare(other) < 0);
  }
  // TODO(cshapiro): handle user-defined subtypes of string.
  return thread->runtime()->notImplemented();
}

static word stringFormatBufferLength(const Handle<String>& fmt,
                                     const Handle<ObjectArray>& args) {
  word arg_idx = 0;
  word len = 0;
  for (word fmt_idx = 0; fmt_idx < fmt->length(); fmt_idx++, len++) {
    byte ch = fmt->charAt(fmt_idx);
    if (ch != '%') {
      continue;
    }
    switch (fmt->charAt(++fmt_idx)) {
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
        CHECK(args->at(arg_idx)->isString(), "Argument mismatch");
        len += String::cast(args->at(arg_idx))->length();
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

static void stringFormatToBuffer(const Handle<String>& fmt,
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
        String* value = String::cast(args->at(arg_idx));
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

Object* stringFormat(Thread* thread, const Handle<String>& fmt,
                     const Handle<ObjectArray>& args) {
  if (fmt->length() == 0) {
    return *fmt;
  }
  word len = stringFormatBufferLength(fmt, args);
  char* dst = static_cast<char*>(std::malloc(len + 1));
  CHECK(dst != nullptr, "Buffer allocation failure");
  stringFormatToBuffer(fmt, args, dst, len);
  Object* result = thread->runtime()->newStringFromCString(dst);
  std::free(dst);
  return result;
}

Object* builtinStringMod(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(caller, nargs);
  Handle<Object> self(&scope, args.get(0));
  Handle<Object> other(&scope, args.get(1));
  if (self->isString()) {
    Handle<String> format(&scope, *self);
    Handle<ObjectArray> format_args(&scope, runtime->newObjectArray(0));
    if (other->isObjectArray()) {
      format_args = *other;
    } else {
      Handle<ObjectArray> tuple(&scope, runtime->newObjectArray(1));
      tuple->atPut(0, *other);
      format_args = *tuple;
    }
    return stringFormat(thread, format, format_args);
  }
  // TODO(cshapiro): handle user-defined subtypes of string.
  return runtime->notImplemented();
}

Object* builtinStringNe(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(frame, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isString() && other->isString()) {
    return Bool::fromBool(String::cast(self)->compare(other) != 0);
  }
  // TODO(cshapiro): handle user-defined subtypes of string.
  return thread->runtime()->notImplemented();
}

Object* builtinStringNew(Thread* thread, Frame* frame, word nargs) {
  if (nargs == 0) {
    return thread->throwTypeErrorFromCString(
        "str.__new__(): not enough arguments");
  }
  if (nargs > 4) {
    return thread->throwTypeErrorFromCString(
        "str() takes at most three arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Handle<Object> type(&scope, args.get(0));
  if (!runtime->hasSubClassFlag(*type, Type::Flag::kTypeSubclass)) {
    return thread->throwTypeErrorFromCString(
        "str.__new__(X): X is not a type object");
  }
  if (!Type::cast(*type)->hasFlag(Type::Flag::kStrSubclass)) {
    return thread->throwTypeErrorFromCString(
        "str.__new__(X): X is not a subtype of str");
  }
  Handle<Layout> layout(&scope, Type::cast(*type)->instanceLayout());
  if (layout->id() != LayoutId::kString) {
    // TODO: Implement __new__ with subtypes of str.
    UNIMPLEMENTED("str.__new__(<subtype of str>, ...)");
  }
  if (nargs == 1) {
    // No argument to str, return empty string.
    return runtime->newStringFromCString("");
  }
  if (nargs > 2) {
    UNIMPLEMENTED("str() with encoding");
  }
  // Only one argument, the value to be stringified.
  Handle<Object> arg(&scope, args.get(1));
  // If it's already exactly a string, return it immediately.
  if (arg->isString()) {
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
    return thread->throwTypeErrorFromCString("__str__ returned non-string");
  }
  return ret;
}

Object* builtinStringGetItem(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Handle<Object> self(&scope, args.get(0));

  if (self->isString()) {
    Handle<String> string(&scope, *self);
    Object* index = args.get(1);
    if (index->isSmallInt()) {
      word idx = SmallInt::cast(index)->value();
      if (idx < 0) {
        idx = string->length() - idx;
      }
      if (idx < 0 || idx >= string->length()) {
        return thread->throwIndexErrorFromCString("string index out of range");
      }
      byte c = string->charAt(idx);
      return SmallStr::fromBytes(View<byte>(&c, 1));
    } else {
      // TODO(jeethu): Add support for slicing strings
      return thread->throwTypeErrorFromCString(
          "string indices must be integers");
    }
  }
  // TODO(jeethu): handle user-defined subtypes of string.
  return thread->throwTypeErrorFromCString(
      "__getitem__() must be called with a string instance as the first "
      "argument");
}

}  // namespace python
