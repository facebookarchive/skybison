#include "str-builtins.h"

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

Object* builtinStringEq(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(frame, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isString() && other->isString()) {
    return Boolean::fromBool(String::cast(self)->compare(other) == 0);
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
    return Boolean::fromBool(String::cast(self)->compare(other) >= 0);
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
    return Boolean::fromBool(String::cast(self)->compare(other) > 0);
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
    return Boolean::fromBool(String::cast(self)->compare(other) <= 0);
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
    return Boolean::fromBool(String::cast(self)->compare(other) < 0);
  }
  // TODO(cshapiro): handle user-defined subtypes of string.
  return thread->runtime()->notImplemented();
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
    return runtime->stringFormat(thread, format, format_args);
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
    return Boolean::fromBool(String::cast(self)->compare(other) != 0);
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
  if (!runtime->hasSubClassFlag(*type, Class::Flag::kClassSubclass)) {
    return thread->throwTypeErrorFromCString(
        "str.__new__(X): X is not a type object");
  }
  if (!Class::cast(*type)->hasFlag(Class::Flag::kStrSubclass)) {
    return thread->throwTypeErrorFromCString(
        "str.__new__(X): X is not a subtype of str");
  }
  Handle<Layout> layout(&scope, Class::cast(*type)->instanceLayout());
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
      !runtime->hasSubClassFlag(ret, Class::Flag::kStrSubclass)) {
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
    if (index->isSmallInteger()) {
      word idx = SmallInteger::cast(index)->value();
      if (idx < 0) {
        idx = string->length() - idx;
      }
      if (idx < 0 || idx >= string->length()) {
        return thread->throwIndexErrorFromCString("string index out of range");
      }
      byte c = string->charAt(idx);
      return SmallString::fromBytes(View<byte>(&c, 1));
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
