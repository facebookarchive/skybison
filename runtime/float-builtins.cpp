#include "float-builtins.h"

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

Object* builtinDoubleEq(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(frame, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isDouble() && other->isDouble()) {
    Double* left = Double::cast(self);
    Double* right = Double::cast(other);
    return Boolean::fromBool(left->value() == right->value());
  } else if (self->isInteger() || other->isInteger()) {
    UNIMPLEMENTED("integer to float conversion");
  }
  return thread->runtime()->notImplemented();
}

Object* builtinDoubleGe(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(frame, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isDouble() && other->isDouble()) {
    Double* left = Double::cast(self);
    Double* right = Double::cast(other);
    return Boolean::fromBool(left->value() >= right->value());
  } else if (self->isInteger() || other->isInteger()) {
    UNIMPLEMENTED("integer to float conversion");
  }
  return thread->runtime()->notImplemented();
}

Object* builtinDoubleGt(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(frame, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isDouble() && other->isDouble()) {
    Double* left = Double::cast(self);
    Double* right = Double::cast(other);
    return Boolean::fromBool(left->value() > right->value());
  } else if (self->isInteger() || other->isInteger()) {
    UNIMPLEMENTED("integer to float conversion");
  }
  return thread->runtime()->notImplemented();
}

Object* builtinDoubleLe(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(frame, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isDouble() && other->isDouble()) {
    Double* left = Double::cast(self);
    Double* right = Double::cast(other);
    return Boolean::fromBool(left->value() <= right->value());
  } else if (self->isInteger() || other->isInteger()) {
    UNIMPLEMENTED("integer to float conversion");
  }
  return thread->runtime()->notImplemented();
}

Object* builtinDoubleLt(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(frame, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isDouble() && other->isDouble()) {
    Double* left = Double::cast(self);
    Double* right = Double::cast(other);
    return Boolean::fromBool(left->value() < right->value());
  } else if (self->isInteger() || other->isInteger()) {
    UNIMPLEMENTED("integer to float conversion");
  }
  return thread->runtime()->notImplemented();
}

Object* builtinDoubleNe(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(frame, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isDouble() && other->isDouble()) {
    Double* left = Double::cast(self);
    Double* right = Double::cast(other);
    return Boolean::fromBool(left->value() != right->value());
  } else if (self->isInteger() || other->isInteger()) {
    UNIMPLEMENTED("integer to float conversion");
  }
  return thread->runtime()->notImplemented();
}

Object* builtinDoubleSub(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }

  Arguments args(frame, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (!self->isDouble()) {
    // TODO(T30659244): More informative TypeError message
    return thread->throwTypeErrorFromCString(
        "descriptor '__sub__' requires a 'float' object");
  }

  Double* left = Double::cast(self);
  if (other->isDouble()) {
    Double* right = Double::cast(other);
    return thread->runtime()->newDouble(left->value() - right->value());
  }
  if (other->isSmallInteger()) {
    SmallInteger* right = SmallInteger::cast(other);
    return thread->runtime()->newDouble(
        left->value() - static_cast<double>(right->value()));
  }
  // TODO(T30610701): Handle LargeIntegers
  return thread->runtime()->notImplemented();
}

} // namespace python
