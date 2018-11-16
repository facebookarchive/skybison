#include "str-builtins.h"

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

Object* builtinStringEq(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(caller, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isString() && other->isString()) {
    return Boolean::fromBool(String::cast(self)->compare(other) == 0);
  }
  // TODO(cshapiro): handle user-defined subtypes of string.
  return thread->runtime()->notImplemented();
}

Object* builtinStringGe(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(caller, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isString() && other->isString()) {
    return Boolean::fromBool(String::cast(self)->compare(other) >= 0);
  }
  // TODO(cshapiro): handle user-defined subtypes of string.
  return thread->runtime()->notImplemented();
}

Object* builtinStringGt(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(caller, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isString() && other->isString()) {
    return Boolean::fromBool(String::cast(self)->compare(other) > 0);
  }
  // TODO(cshapiro): handle user-defined subtypes of string.
  return thread->runtime()->notImplemented();
}

Object* builtinStringLe(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(caller, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isString() && other->isString()) {
    return Boolean::fromBool(String::cast(self)->compare(other) <= 0);
  }
  // TODO(cshapiro): handle user-defined subtypes of string.
  return thread->runtime()->notImplemented();
}

Object* builtinStringLt(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(caller, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isString() && other->isString()) {
    return Boolean::fromBool(String::cast(self)->compare(other) < 0);
  }
  // TODO(cshapiro): handle user-defined subtypes of string.
  return thread->runtime()->notImplemented();
}

Object* builtinStringNe(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(caller, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isString() && other->isString()) {
    return Boolean::fromBool(String::cast(self)->compare(other) != 0);
  }
  // TODO(cshapiro): handle user-defined subtypes of string.
  return thread->runtime()->notImplemented();
}

} // namespace python
