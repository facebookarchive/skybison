#include "int-builtins.h"

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

Object* builtinSmallIntegerBool(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 1) {
    return thread->throwTypeErrorFromCString("not enough arguments");
  }
  Arguments args(caller, nargs);
  if (args.get(0)->isSmallInteger()) {
    return Boolean::fromBool(args.get(0) != SmallInteger::fromWord(0));
  }
  return thread->throwTypeErrorFromCString("unsupported type for __bool__");
}

Object* builtinSmallIntegerEq(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(caller, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isSmallInteger() && other->isSmallInteger()) {
    return Boolean::fromBool(self == other);
  }
  return thread->runtime()->notImplemented();
}

Object* builtinSmallIntegerInvert(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 1) {
    return thread->throwTypeErrorFromCString("not enough arguments");
  }
  Arguments args(caller, nargs);
  if (args.get(0)->isSmallInteger()) {
    SmallInteger* tos = SmallInteger::cast(args.get(0));
    return SmallInteger::fromWord(-(tos->value() + 1));
  }
  return thread->throwTypeErrorFromCString("unsupported type for __invert__");
}

Object* builtinSmallIntegerLe(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(caller, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isSmallInteger() && other->isSmallInteger()) {
    SmallInteger* left = SmallInteger::cast(self);
    SmallInteger* right = SmallInteger::cast(other);
    return Boolean::fromBool(left->value() <= right->value());
  }
  return thread->runtime()->notImplemented();
}

Object* builtinSmallIntegerLt(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(caller, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isSmallInteger() && other->isSmallInteger()) {
    SmallInteger* left = SmallInteger::cast(self);
    SmallInteger* right = SmallInteger::cast(other);
    return Boolean::fromBool(left->value() < right->value());
  }
  return thread->runtime()->notImplemented();
}

Object* builtinSmallIntegerGe(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(caller, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isSmallInteger() && other->isSmallInteger()) {
    SmallInteger* left = SmallInteger::cast(self);
    SmallInteger* right = SmallInteger::cast(other);
    return Boolean::fromBool(left->value() >= right->value());
  }
  return thread->runtime()->notImplemented();
}

Object* builtinSmallIntegerGt(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(caller, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isSmallInteger() && other->isSmallInteger()) {
    SmallInteger* left = SmallInteger::cast(self);
    SmallInteger* right = SmallInteger::cast(other);
    return Boolean::fromBool(left->value() > right->value());
  }
  return thread->runtime()->notImplemented();
}

Object* builtinSmallIntegerNe(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(caller, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isSmallInteger() && other->isSmallInteger()) {
    return Boolean::fromBool(self != other);
  }
  return thread->runtime()->notImplemented();
}

Object* builtinSmallIntegerNeg(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 1) {
    return thread->throwTypeErrorFromCString("not enough arguments");
  }
  Arguments args(caller, nargs);
  SmallInteger* tos = SmallInteger::cast(args.get(0));
  return SmallInteger::fromWord(-tos->value());
}

Object* builtinSmallIntegerPos(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 1) {
    return thread->throwTypeErrorFromCString("not enough arguments");
  }
  Arguments args(caller, nargs);
  return SmallInteger::cast(args.get(0));
}

Object* builtinSmallIntegerSub(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(caller, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isSmallInteger() && other->isSmallInteger()) {
    SmallInteger* left = SmallInteger::cast(self);
    SmallInteger* right = SmallInteger::cast(other);
    // TODO(cshapiro): handle overflow
    return SmallInteger::fromWord(left->value() - right->value());
  }
  return thread->runtime()->notImplemented();
}

} // namespace python
