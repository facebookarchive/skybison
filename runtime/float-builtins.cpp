#include "float-builtins.h"

#include <cmath>
#include <limits>

#include "frame.h"
#include "globals.h"
#include "int-builtins.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"
#include "trampolines-inl.h"

namespace python {

RawObject asFloatObject(Thread* thread, const Object& obj) {
  // Object is float
  if (obj.isFloat()) return *obj;

  // Object is subclass of float
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  if (runtime->isInstanceOfFloat(*obj)) {
    UserFloatBase user_float(&scope, *obj);
    return user_float.floatValue();
  }

  // Try calling __float__
  Frame* frame = thread->currentFrame();
  Object fltmethod(&scope, Interpreter::lookupMethod(thread, frame, obj,
                                                     SymbolId::kDunderFloat));
  if (fltmethod.isError()) {
    return thread->raiseTypeErrorWithCStr("must be a real number");
  }
  Object flt_obj(&scope,
                 Interpreter::callMethod1(thread, frame, fltmethod, obj));
  if (flt_obj.isError() || flt_obj.isFloat()) return *flt_obj;
  if (!runtime->isInstanceOfFloat(*flt_obj)) {
    return thread->raiseTypeErrorWithCStr("__float__ returned non-float");
  }
  UserFloatBase user_float(&scope, *obj);
  return user_float.floatValue();
}

// Convert `object` to double.
// Returns a NoneType and sets `value` if the conversion was successful.
// Returns an error or unimplemented otherwise. This does specifically not
// look for `__float__` to match the behavior of `CONVERT_TO_DOUBLE()` in
// cpython.
static RawObject convertToDouble(Thread* thread, const Object& object,
                                 double* result) {
  if (object.isFloat()) {
    *result = RawFloat::cast(*object)->value();
    return NoneType::object();
  }

  Runtime* runtime = thread->runtime();
  if (runtime->isInstanceOfInt(*object)) {
    HandleScope scope(thread);
    Int value(&scope, *object);
    return convertIntToDouble(thread, value, result);
  }

  return runtime->notImplemented();
}

const BuiltinMethod FloatBuiltins::kBuiltinMethods[] = {
    {SymbolId::kDunderAdd, dunderAdd},
    {SymbolId::kDunderTruediv, dunderTrueDiv},
    {SymbolId::kDunderEq, dunderEq},
    {SymbolId::kDunderFloat, dunderFloat},
    {SymbolId::kDunderGe, dunderGe},
    {SymbolId::kDunderGt, dunderGt},
    {SymbolId::kDunderLe, dunderLe},
    {SymbolId::kDunderLt, dunderLt},
    {SymbolId::kDunderMul, dunderMul},
    {SymbolId::kDunderNe, dunderNe},
    {SymbolId::kDunderNeg, dunderNeg},
    {SymbolId::kDunderNew, dunderNew},
    {SymbolId::kDunderPow, dunderPow},
    {SymbolId::kDunderRepr, dunderRepr},
    {SymbolId::kDunderRtruediv, dunderRtrueDiv},
    {SymbolId::kDunderSub, dunderSub},
};

const BuiltinAttribute FloatBuiltins::kAttributes[] = {
    {SymbolId::kInvalid, RawUserFloatBase::kFloatOffset},
};

void FloatBuiltins::initialize(Runtime* runtime) {
  HandleScope scope;
  Type type(&scope, runtime->addBuiltinType(SymbolId::kFloat, LayoutId::kFloat,
                                            LayoutId::kObject, kAttributes,
                                            View<NativeMethod>(nullptr, 0),
                                            kBuiltinMethods));
}

RawObject FloatBuiltins::floatFromObject(Thread* thread, Frame* frame,
                                         word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object obj(&scope, args.get(1));
  if (obj.isFloat()) {
    return *obj;
  }

  // This only converts exact strings.
  if (obj.isStr()) {
    return floatFromString(thread, RawStr::cast(*obj));
  }

  // Not a float, call __float__ on it to convert.
  // Since float itself defines __float__, subclasses of float are automatically
  // handled here.
  Object method(&scope, Interpreter::lookupMethod(thread, frame, obj,
                                                  SymbolId::kDunderFloat));
  if (method.isError()) {
    return thread->raiseTypeErrorWithCStr(
        "TypeError: float() argument must have a __float__");
  }

  Object converted(&scope,
                   Interpreter::callMethod1(thread, frame, method, obj));
  // If there was an exception thrown during call, propagate it up.
  if (converted.isError()) {
    return *converted;
  }

  // If __float__ returns a non-float, throw a type error.
  if (!runtime->isInstanceOfFloat(*converted)) {
    return thread->raiseTypeErrorWithCStr("__float__ returned non-float");
  }

  // __float__ used to be allowed to return any subtype of float, but that
  // behavior was deprecated.
  // TODO(dulinr): Convert this to a warning exception once that is supported.
  CHECK(converted.isFloat(),
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

RawObject FloatBuiltins::dunderFloat(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (self.isFloat()) {
    return *self;
  }
  if (runtime->isInstanceOfFloat(*self)) {
    UserFloatBase user_float(&scope, *self);
    return user_float.floatValue();
  }
  return thread->raiseTypeErrorWithCStr("'float' object expected");
}

RawObject FloatBuiltins::dunderGe(Thread* thread, Frame* frame, word nargs) {
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

RawObject FloatBuiltins::dunderMul(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);

  Object self_obj(&scope, args.get(0));
  if (!self_obj.isFloat()) {
    return thread->raiseTypeErrorWithCStr(
        "__mul__() must be called with float instance as first argument");
  }
  Float self(&scope, *self_obj);
  double left = self.value();

  double right;
  Object other(&scope, args.get(1));
  Object maybe_error(&scope, convertToDouble(thread, other, &right));
  // May have returned NotImplemented or raised an exception.
  if (!maybe_error.isNoneType()) return *maybe_error;

  return thread->runtime()->newFloat(left * right);
}

RawObject FloatBuiltins::dunderNe(Thread* thread, Frame* frame, word nargs) {
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

RawObject FloatBuiltins::dunderNeg(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfFloat(*self_obj)) {
    return thread->raiseTypeErrorWithCStr(
        "__neg__() must be called with float instance as first argument");
  }
  Float self(&scope, *self_obj);
  return runtime->newFloat(-self.value());
}

RawObject FloatBuiltins::dunderNew(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object obj(&scope, args.get(0));
  if (!runtime->isInstanceOfType(*obj)) {
    return thread->raiseTypeErrorWithCStr(
        "float.__new__(X): X is not a type object");
  }
  Type type(&scope, *obj);
  if (type.builtinBase() != LayoutId::kFloat) {
    return thread->raiseTypeErrorWithCStr(
        "float.__new__(X): X is not a subtype of float");
  }

  // Handle subclasses
  if (!type.isBuiltin()) {
    Layout type_layout(&scope, type.instanceLayout());
    UserFloatBase instance(&scope, runtime->newInstance(type_layout));
    instance.setFloatValue(floatFromObject(thread, frame, nargs));
    return *instance;
  }
  return floatFromObject(thread, frame, nargs);
}

RawObject FloatBuiltins::dunderAdd(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  RawObject self = args.get(0);
  if (!self->isFloat()) {
    return thread->raiseTypeErrorWithCStr(
        "__add__() must be called with float instance as first argument");
  }
  double left = RawFloat::cast(self)->value();

  double right;
  HandleScope scope(thread);
  Object other(&scope, args.get(1));
  Object maybe_error(&scope, convertToDouble(thread, other, &right));
  // May have returned NotImplemented or raised an exception.
  if (!maybe_error.isNoneType()) return *maybe_error;

  return thread->runtime()->newFloat(left + right);
}

RawObject FloatBuiltins::dunderTrueDiv(Thread* thread, Frame* frame,
                                       word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);

  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfFloat(*self_obj)) {
    return thread->raiseTypeErrorWithCStr(
        "__truediv__() must be called with float instance as first argument");
  }
  Float self(&scope, *self_obj);
  double left = self.value();

  double right;
  Object other(&scope, args.get(1));
  Object maybe_error(&scope, convertToDouble(thread, other, &right));
  // May have returned NotImplemented or raised an exception.
  if (!maybe_error.isNoneType()) return *maybe_error;

  if (right == 0.0) {
    return thread->raiseZeroDivisionErrorWithCStr("float division by zero");
  }
  return runtime->newFloat(left / right);
}

RawObject FloatBuiltins::dunderRepr(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  RawObject self_obj = args.get(0);
  if (!self_obj.isFloat()) {
    return thread->raiseTypeErrorWithCStr(
        "__repr__() must be called with float instance as first argument");
  }
  double value = RawFloat::cast(self_obj).value();
  int required_size = std::snprintf(nullptr, 0, "%g", value) + 1;  // NUL
  std::unique_ptr<char[]> buffer(new char[required_size]);
  int size = std::snprintf(buffer.get(), required_size, "%g", value);
  CHECK(size < int{required_size}, "buffer too small");
  return thread->runtime()->newStrFromCStr(buffer.get());
}

RawObject FloatBuiltins::dunderRtrueDiv(Thread* thread, Frame* frame,
                                        word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);

  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfFloat(*self_obj)) {
    return thread->raiseTypeErrorWithCStr(
        "__rtruediv__() must be called with float instance as first argument");
  }
  Float self(&scope, *self_obj);
  double right = self.value();

  double left;
  Object other(&scope, args.get(1));
  Object maybe_error(&scope, convertToDouble(thread, other, &left));
  // May have returned NotImplemented or raised an exception.
  if (!maybe_error.isNoneType()) return *maybe_error;

  if (right == 0.0) {
    return thread->raiseZeroDivisionErrorWithCStr("float division by zero");
  }
  return runtime->newFloat(left / right);
}

RawObject FloatBuiltins::dunderSub(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  RawObject self = args.get(0);
  if (!self->isFloat()) {
    return thread->raiseTypeErrorWithCStr(
        "__sub__() must be called with float instance as first argument");
  }
  double left = RawFloat::cast(self)->value();

  double right;
  HandleScope scope(thread);
  Object other(&scope, args.get(1));
  Object maybe_error(&scope, convertToDouble(thread, other, &right));
  // May have returned NotImplemented or raised an exception.
  if (!maybe_error.isNoneType()) return *maybe_error;

  return thread->runtime()->newFloat(left - right);
}

RawObject FloatBuiltins::dunderPow(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  RawObject self = args.get(0);
  if (!self->isFloat()) {
    return thread->raiseTypeErrorWithCStr(
        "__pow__() must be called with float instance as first argument");
  }
  // TODO(T40438612): Implement the modulo operation given the 3rd argument.
  if (!args.get(2).isUnboundValue()) {
    return thread->raiseTypeErrorWithCStr(
        "pow() 3rd argument not allowed unless all arguments are integers");
  }
  double left = RawFloat::cast(self)->value();

  double right;
  HandleScope scope(thread);
  Object other(&scope, args.get(1));
  Object maybe_error(&scope, convertToDouble(thread, other, &right));
  // May have returned NotImplemented or raised an exception.
  if (!maybe_error.isNoneType()) return *maybe_error;

  return thread->runtime()->newFloat(std::pow(left, right));
}

}  // namespace python
