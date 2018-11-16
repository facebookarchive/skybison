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
  if (self->isFloat() && other->isFloat()) {
    Float* left = Float::cast(self);
    Float* right = Float::cast(other);
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
  if (self->isFloat() && other->isFloat()) {
    Float* left = Float::cast(self);
    Float* right = Float::cast(other);
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
  if (self->isFloat() && other->isFloat()) {
    Float* left = Float::cast(self);
    Float* right = Float::cast(other);
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
  if (self->isFloat() && other->isFloat()) {
    Float* left = Float::cast(self);
    Float* right = Float::cast(other);
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
  if (self->isFloat() && other->isFloat()) {
    Float* left = Float::cast(self);
    Float* right = Float::cast(other);
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
  if (self->isFloat() && other->isFloat()) {
    Float* left = Float::cast(self);
    Float* right = Float::cast(other);
    return Boolean::fromBool(left->value() != right->value());
  } else if (self->isInteger() || other->isInteger()) {
    UNIMPLEMENTED("integer to float conversion");
  }
  return thread->runtime()->notImplemented();
}

Object* builtinDoubleNew(Thread* thread, Frame* frame, word nargs) {
  if (nargs < 1) {
    return thread->throwTypeErrorFromCString(
        "float.__new__(): not enough arguments");
  }
  if (nargs > 2) {
    return thread->throwTypeError(thread->runtime()->newStringFromFormat(
        "float expected at most 1 arguments, got %ld", nargs));
  }
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Handle<Object> obj(&scope, args.get(0));
  if (!runtime->hasSubClassFlag(*obj, Type::Flag::kClassSubclass)) {
    return thread->throwTypeErrorFromCString(
        "float.__new__(X): X is not a type object");
  }
  Handle<Type> type(&scope, *obj);
  if (!type->hasFlag(Type::Flag::kFloatSubclass)) {
    return thread->throwTypeErrorFromCString(
        "float.__new__(X): X is not a subtype of float");
  }
  // No arguments.
  if (nargs == 1) {
    return runtime->newFloat(0.0);
  }
  Handle<Layout> layout(&scope, type->instanceLayout());
  if (layout->id() != LayoutId::kFloat) {
    // TODO(dulinr): Implement __new__ with subtypes of float.
    UNIMPLEMENTED("float.__new__(<subtype of float>, ...)");
  }
  Handle<Object> arg(&scope, args.get(1));
  if (arg->isFloat()) {
    return *arg;
  }
  UNIMPLEMENTED("Handle arguments to float() that aren't floats");
}

Object* builtinDoubleAdd(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }

  Arguments args(frame, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (!self->isFloat()) {
    return thread->throwTypeErrorFromCString(
        "__add__() must be called with float instance as first argument");
  }

  double left = Float::cast(self)->value();
  if (other->isFloat()) {
    double right = Float::cast(other)->value();
    return thread->runtime()->newFloat(left + right);
  }
  if (other->isInteger()) {
    double right = Integer::cast(other)->floatValue();
    return thread->runtime()->newFloat(left + right);
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
  if (!self->isFloat()) {
    return thread->throwTypeErrorFromCString(
        "__sub__() must be called with float instance as first argument");
  }

  double left = Float::cast(self)->value();
  if (other->isFloat()) {
    double right = Float::cast(other)->value();
    return thread->runtime()->newFloat(left - right);
  }
  if (other->isInteger()) {
    double right = Integer::cast(other)->floatValue();
    return thread->runtime()->newFloat(left - right);
  }
  return thread->runtime()->notImplemented();
}

}  // namespace python
