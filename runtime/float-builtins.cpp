#include "float-builtins.h"

#include <cmath>
#include <limits>

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"
#include "trampolines-inl.h"

namespace python {

const BuiltinMethod FloatBuiltins::kMethods[] = {
    {SymbolId::kDunderAdd, nativeTrampoline<dunderAdd>},
    {SymbolId::kDunderEq, nativeTrampoline<dunderEq>},
    {SymbolId::kDunderGe, nativeTrampoline<dunderGe>},
    {SymbolId::kDunderGt, nativeTrampoline<dunderGt>},
    {SymbolId::kDunderLe, nativeTrampoline<dunderLe>},
    {SymbolId::kDunderLt, nativeTrampoline<dunderLt>},
    {SymbolId::kDunderNe, nativeTrampoline<dunderNe>},
    {SymbolId::kDunderNew, nativeTrampoline<dunderNew>},
    {SymbolId::kDunderPow, nativeTrampoline<dunderPow>},
    {SymbolId::kDunderSub, nativeTrampoline<dunderSub>},
};

const BuiltinAttribute FloatBuiltins::kAttributes[] = {
    {SymbolId::kInvalid, RawUserFloatBase::kFloatOffset},
};

void FloatBuiltins::initialize(Runtime* runtime) {
  HandleScope scope;
  Type type(&scope,
            runtime->addBuiltinType(SymbolId::kFloat, LayoutId::kFloat,
                                    LayoutId::kObject, kAttributes, kMethods));
  type->setFlag(Type::Flag::kFloatSubclass);
}

RawObject FloatBuiltins::floatFromObject(Thread* thread, Frame* frame,
                                         word nargs) {
  Runtime* runtime = thread->runtime();
  if (nargs == 1) {
    return runtime->newFloat(0.0);
  }

  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object obj(&scope, args.get(1));
  if (obj->isFloat()) {
    return *obj;
  }

  // This only converts exact strings.
  if (obj->isStr()) {
    return floatFromString(thread, RawStr::cast(*obj));
  }

  // Not a float, call __float__ on it to convert.
  // Since float itself defines __float__, subclasses of float are automatically
  // handled here.
  Object method(&scope, Interpreter::lookupMethod(thread, frame, obj,
                                                  SymbolId::kDunderFloat));
  if (method->isError()) {
    return thread->raiseTypeErrorWithCStr(
        "TypeError: float() argument must have a __float__");
  }

  Object converted(&scope,
                   Interpreter::callMethod1(thread, frame, method, obj));
  // If there was an exception thrown during call, propagate it up.
  if (converted->isError()) {
    return *converted;
  }

  // If __float__ returns a non-float, throw a type error.
  if (!runtime->hasSubClassFlag(*converted, Type::Flag::kFloatSubclass)) {
    return thread->raiseTypeErrorWithCStr("__float__ returned non-float");
  }

  // __float__ used to be allowed to return any subtype of float, but that
  // behavior was deprecated.
  // TODO(dulinr): Convert this to a warning exception once that is supported.
  CHECK(converted->isFloat(),
        "__float__ returned a strict subclass of float, which is deprecated");
  return *converted;
}

RawObject FloatBuiltins::floatFromString(Thread* thread, RawStr str) {
  char* str_end = nullptr;
  char* c_str = str->toCStr();
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
    return thread->raiseValueErrorWithCStr("could not convert string to float");
  }
  return thread->runtime()->newFloat(result);
}

RawObject FloatBuiltins::dunderEq(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("expected 1 argument");
  }
  Arguments args(frame, nargs);
  RawObject self = args.get(0);
  RawObject other = args.get(1);
  if (self->isFloat() && other->isFloat()) {
    RawFloat left = RawFloat::cast(self);
    RawFloat right = RawFloat::cast(other);
    return Bool::fromBool(left->value() == right->value());
  }
  if (self->isInt() || other->isInt()) {
    UNIMPLEMENTED("integer to float conversion");
  }
  return thread->runtime()->notImplemented();
}

RawObject FloatBuiltins::dunderGe(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("expected 1 argument");
  }
  Arguments args(frame, nargs);
  RawObject self = args.get(0);
  RawObject other = args.get(1);
  if (self->isFloat() && other->isFloat()) {
    RawFloat left = RawFloat::cast(self);
    RawFloat right = RawFloat::cast(other);
    return Bool::fromBool(left->value() >= right->value());
  }
  if (self->isInt() || other->isInt()) {
    UNIMPLEMENTED("integer to float conversion");
  }
  return thread->runtime()->notImplemented();
}

RawObject FloatBuiltins::dunderGt(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("expected 1 argument");
  }
  Arguments args(frame, nargs);
  RawObject self = args.get(0);
  RawObject other = args.get(1);
  if (self->isFloat() && other->isFloat()) {
    RawFloat left = RawFloat::cast(self);
    RawFloat right = RawFloat::cast(other);
    return Bool::fromBool(left->value() > right->value());
  }
  if (self->isInt() || other->isInt()) {
    UNIMPLEMENTED("integer to float conversion");
  }
  return thread->runtime()->notImplemented();
}

RawObject FloatBuiltins::dunderLe(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("expected 1 argument");
  }
  Arguments args(frame, nargs);
  RawObject self = args.get(0);
  RawObject other = args.get(1);
  if (self->isFloat() && other->isFloat()) {
    RawFloat left = RawFloat::cast(self);
    RawFloat right = RawFloat::cast(other);
    return Bool::fromBool(left->value() <= right->value());
  }
  if (self->isInt() || other->isInt()) {
    UNIMPLEMENTED("integer to float conversion");
  }
  return thread->runtime()->notImplemented();
}

RawObject FloatBuiltins::dunderLt(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("expected 1 argument");
  }
  Arguments args(frame, nargs);
  RawObject self = args.get(0);
  RawObject other = args.get(1);
  if (self->isFloat() && other->isFloat()) {
    RawFloat left = RawFloat::cast(self);
    RawFloat right = RawFloat::cast(other);
    return Bool::fromBool(left->value() < right->value());
  }
  if (self->isInt() || other->isInt()) {
    UNIMPLEMENTED("integer to float conversion");
  }
  return thread->runtime()->notImplemented();
}

RawObject FloatBuiltins::dunderNe(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("expected 1 argument");
  }
  Arguments args(frame, nargs);
  RawObject self = args.get(0);
  RawObject other = args.get(1);
  if (self->isFloat() && other->isFloat()) {
    RawFloat left = RawFloat::cast(self);
    RawFloat right = RawFloat::cast(other);
    return Bool::fromBool(left->value() != right->value());
  }
  if (self->isInt() || other->isInt()) {
    UNIMPLEMENTED("integer to float conversion");
  }
  return thread->runtime()->notImplemented();
}

RawObject FloatBuiltins::dunderNew(Thread* thread, Frame* frame, word nargs) {
  if (nargs < 1) {
    return thread->raiseTypeErrorWithCStr(
        "float.__new__(): not enough arguments");
  }
  if (nargs > 2) {
    return thread->raiseTypeError(thread->runtime()->newStrFromFormat(
        "float expected at most 1 arguments, got %ld", nargs));
  }
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object obj(&scope, args.get(0));
  if (!runtime->hasSubClassFlag(*obj, Type::Flag::kTypeSubclass)) {
    return thread->raiseTypeErrorWithCStr(
        "float.__new__(X): X is not a type object");
  }
  Type type(&scope, *obj);
  if (!type->hasFlag(Type::Flag::kFloatSubclass)) {
    return thread->raiseTypeErrorWithCStr(
        "float.__new__(X): X is not a subtype of float");
  }

  // Handle subclasses
  if (!type->isIntrinsicOrExtension()) {
    Layout type_layout(&scope, type->instanceLayout());
    UserFloatBase instance(&scope, runtime->newInstance(type_layout));
    instance->setFloatValue(floatFromObject(thread, frame, nargs));
    return *instance;
  }
  return floatFromObject(thread, frame, nargs);
}

RawObject FloatBuiltins::dunderAdd(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("expected 1 argument");
  }

  Arguments args(frame, nargs);
  RawObject self = args.get(0);
  RawObject other = args.get(1);
  if (!self->isFloat()) {
    return thread->raiseTypeErrorWithCStr(
        "__add__() must be called with float instance as first argument");
  }

  double left = RawFloat::cast(self)->value();
  if (other->isFloat()) {
    double right = RawFloat::cast(other)->value();
    return thread->runtime()->newFloat(left + right);
  }
  if (other->isInt()) {
    double right = RawInt::cast(other)->floatValue();
    return thread->runtime()->newFloat(left + right);
  }
  return thread->runtime()->notImplemented();
}

RawObject FloatBuiltins::dunderSub(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("expected 1 argument");
  }

  Arguments args(frame, nargs);
  RawObject self = args.get(0);
  RawObject other = args.get(1);
  if (!self->isFloat()) {
    return thread->raiseTypeErrorWithCStr(
        "__sub__() must be called with float instance as first argument");
  }

  double left = RawFloat::cast(self)->value();
  if (other->isFloat()) {
    double right = RawFloat::cast(other)->value();
    return thread->runtime()->newFloat(left - right);
  }
  if (other->isInt()) {
    double right = RawInt::cast(other)->floatValue();
    return thread->runtime()->newFloat(left - right);
  }
  return thread->runtime()->notImplemented();
}

RawObject FloatBuiltins::dunderPow(Thread* thread, Frame* frame, word nargs) {
  if (nargs < 2 || nargs > 3) {
    return thread->raiseTypeErrorWithCStr("expected at most 2 arguments");
  }
  Arguments args(frame, nargs);
  RawObject self = args.get(0);
  RawObject other = args.get(1);
  if (!self->isFloat()) {
    return thread->raiseTypeErrorWithCStr(
        "__pow__() must be called with float instance as first argument");
  }
  if (nargs == 3) {
    return thread->raiseTypeErrorWithCStr(
        "pow() 3rd argument not allowed unless all arguments are integers");
  }
  double left = RawFloat::cast(self)->value();
  if (other->isFloat()) {
    double right = RawFloat::cast(other)->value();
    return thread->runtime()->newFloat(std::pow(left, right));
  }
  if (other->isInt()) {
    double right = RawInt::cast(other)->floatValue();
    return thread->runtime()->newFloat(std::pow(left, right));
  }
  return thread->runtime()->notImplemented();
}

}  // namespace python
