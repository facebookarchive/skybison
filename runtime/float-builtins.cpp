#include "float-builtins.h"

#include <cmath>
#include <limits>

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

static Object* floatFromObject(Thread* thread, Frame* frame,
                               const Handle<Object>& obj) {
  if (obj->isFloat()) {
    return *obj;
  }

  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();

  // Not a float, call __float__ on it to convert.
  // Since float itself defines __float__, subclasses of float are automatically
  // handled here.
  Handle<Object> method(
      &scope,
      Interpreter::lookupMethod(thread, frame, obj, SymbolId::kDunderFloat));
  if (method->isError()) {
    return thread->throwTypeErrorFromCString(
        "TypeError: float() argument must have a __float__");
  }

  Handle<Object> converted(
      &scope, Interpreter::callMethod1(thread, frame, method, obj));
  // If there was an exception thrown during call, propagate it up.
  if (converted->isError()) {
    return *converted;
  }

  // If __float__ returns a non-float, throw a type error.
  if (!runtime->hasSubClassFlag(*converted, Type::Flag::kFloatSubclass)) {
    return thread->throwTypeErrorFromCString("__float__ returned non-float");
  }

  // __float__ used to be allowed to return any subtype of float, but that
  // behavior was deprecated.
  // TODO(dulinr): Convert this to a warning exception once that is supported.
  CHECK(converted->isFloat(),
        "__float__ returned a strict subclass of float, which is deprecated");
  return *converted;
}

static Object* floatFromString(Thread* thread, String* str) {
  char* str_end = nullptr;
  char* c_str = str->toCString();
  double result = std::strtod(c_str, &str_end);
  std::free(c_str);

  // Overflow, return infinity or negative infinity.
  if (result == HUGE_VAL) {
    return thread->runtime()->newFloat(std::numeric_limits<double>::infinity());
  }
  if (result == -HUGE_VAL) {
    return thread->runtime()->newFloat(
        -std::numeric_limits<double>::infinity());
  }

  // No conversion occurred, the string was not a valid float.
  if (c_str == str_end) {
    return thread->throwValueErrorFromCString(
        "could not convert string to float");
  }
  return thread->runtime()->newFloat(result);
}

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
    return Bool::fromBool(left->value() == right->value());
  } else if (self->isInt() || other->isInt()) {
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
    return Bool::fromBool(left->value() >= right->value());
  } else if (self->isInt() || other->isInt()) {
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
    return Bool::fromBool(left->value() > right->value());
  } else if (self->isInt() || other->isInt()) {
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
    return Bool::fromBool(left->value() <= right->value());
  } else if (self->isInt() || other->isInt()) {
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
    return Bool::fromBool(left->value() < right->value());
  } else if (self->isInt() || other->isInt()) {
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
    return Bool::fromBool(left->value() != right->value());
  } else if (self->isInt() || other->isInt()) {
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
  if (!runtime->hasSubClassFlag(*obj, Type::Flag::kTypeSubclass)) {
    return thread->throwTypeErrorFromCString(
        "float.__new__(X): X is not a type object");
  }
  Handle<Type> type(&scope, *obj);
  if (!type->hasFlag(Type::Flag::kFloatSubclass)) {
    return thread->throwTypeErrorFromCString(
        "float.__new__(X): X is not a subtype of float");
  }
  Handle<Layout> layout(&scope, type->instanceLayout());
  if (layout->id() != LayoutId::kFloat) {
    // TODO(dulinr): Implement __new__ with subtypes of float.
    UNIMPLEMENTED("float.__new__(<subtype of float>, ...)");
  }

  // No arguments.
  if (nargs == 1) {
    return runtime->newFloat(0.0);
  }

  Handle<Object> arg(&scope, args.get(1));
  // Only convert exactly strings, not subtypes of string.
  if (arg->isString()) {
    return floatFromString(thread, String::cast(*arg));
  }
  return floatFromObject(thread, frame, arg);
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
  if (other->isInt()) {
    double right = Int::cast(other)->floatValue();
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
  if (other->isInt()) {
    double right = Int::cast(other)->floatValue();
    return thread->runtime()->newFloat(left - right);
  }
  return thread->runtime()->notImplemented();
}

Object* builtinDoublePow(Thread* thread, Frame* frame, word nargs) {
  if (nargs < 2 || nargs > 3) {
    return thread->throwTypeErrorFromCString("expected at most 2 arguments");
  }
  Arguments args(frame, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (!self->isFloat()) {
    return thread->throwTypeErrorFromCString(
        "__pow__() must be called with float instance as first argument");
  }
  if (nargs == 3) {
    return thread->throwTypeErrorFromCString(
        "pow() 3rd argument not allowed unless all arguments are integers");
  }
  double left = Float::cast(self)->value();
  if (other->isFloat()) {
    double right = Float::cast(other)->value();
    return thread->runtime()->newFloat(std::pow(left, right));
  }
  if (other->isInt()) {
    double right = Int::cast(other)->floatValue();
    return thread->runtime()->newFloat(std::pow(left, right));
  }
  return thread->runtime()->notImplemented();
}

}  // namespace python
