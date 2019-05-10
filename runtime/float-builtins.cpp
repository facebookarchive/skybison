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
    return thread->raiseWithFmt(LayoutId::kTypeError, "must be a real number");
  }
  Object flt_obj(&scope,
                 Interpreter::callMethod1(thread, frame, fltmethod, obj));
  if (flt_obj.isError() || flt_obj.isFloat()) return *flt_obj;
  if (!runtime->isInstanceOfFloat(*flt_obj)) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "__float__ returned non-float");
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
    *result = Float::cast(*object).value();
    return NoneType::object();
  }
  if (thread->runtime()->isInstanceOfInt(*object)) {
    HandleScope scope(thread);
    Int value(&scope, intUnderlying(thread, object));
    return convertIntToDouble(thread, value, result);
  }
  return NotImplementedType::object();
}

const BuiltinMethod FloatBuiltins::kBuiltinMethods[] = {
    {SymbolId::kDunderAbs, dunderAbs},
    {SymbolId::kDunderAdd, dunderAdd},
    {SymbolId::kDunderBool, dunderBool},
    {SymbolId::kDunderEq, dunderEq},
    {SymbolId::kDunderFloat, dunderFloat},
    {SymbolId::kDunderGe, dunderGe},
    {SymbolId::kDunderGt, dunderGt},
    {SymbolId::kDunderInt, dunderInt},
    {SymbolId::kDunderLe, dunderLe},
    {SymbolId::kDunderLt, dunderLt},
    {SymbolId::kDunderMul, dunderMul},
    {SymbolId::kDunderNeg, dunderNeg},
    {SymbolId::kDunderNew, dunderNew},
    {SymbolId::kDunderPow, dunderPow},
    {SymbolId::kDunderRepr, dunderRepr},
    {SymbolId::kDunderRtruediv, dunderRtrueDiv},
    {SymbolId::kDunderSub, dunderSub},
    {SymbolId::kDunderTruediv, dunderTrueDiv},
    {SymbolId::kSentinelId, nullptr},
};

const BuiltinAttribute FloatBuiltins::kAttributes[] = {
    {SymbolId::kInvalid, RawUserFloatBase::kFloatOffset},
    {SymbolId::kSentinelId, 0},
};

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
    return floatFromString(thread, Str::cast(*obj));
  }

  // Not a float, call __float__ on it to convert.
  // Since float itself defines __float__, subclasses of float are automatically
  // handled here.
  Object method(&scope, Interpreter::lookupMethod(thread, frame, obj,
                                                  SymbolId::kDunderFloat));
  if (method.isError()) {
    return thread->raiseWithFmt(
        LayoutId::kTypeError,
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
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "__float__ returned non-float");
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
  char* c_str = str.toCStr();
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
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "could not convert string to float");
  }
  return thread->runtime()->newFloat(result);
}

RawObject FloatBuiltins::dunderAbs(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfFloat(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kFloat);
  }
  Float self(&scope, *self_obj);
  return runtime->newFloat(std::fabs(self.value()));
}

RawObject FloatBuiltins::dunderBool(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfFloat(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kFloat);
  }
  Float self(&scope, *self_obj);
  return Bool::fromBool(self.value() != 0.0);
}

RawObject FloatBuiltins::dunderEq(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfFloat(*self)) {
    return thread->raiseRequiresType(self, SymbolId::kFloat);
  }
  double left = Float::cast(*self).value();

  Object right(&scope, args.get(1));
  bool result;
  if (runtime->isInstanceOfFloat(*right)) {
    result = left == Float::cast(*right).value();
  } else if (runtime->isInstanceOfInt(*right)) {
    Int right_int(&scope, intUnderlying(thread, right));
    result = doubleEqualsInt(thread, left, right_int);
  } else {
    return NotImplementedType::object();
  }
  return Bool::fromBool(result);
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
  return thread->raiseRequiresType(self, SymbolId::kFloat);
}

RawObject FloatBuiltins::dunderGe(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfFloat(*self)) {
    return thread->raiseRequiresType(self, SymbolId::kFloat);
  }
  double left = Float::cast(*self).value();

  Object right(&scope, args.get(1));
  bool result;
  if (runtime->isInstanceOfFloat(*right)) {
    result = left >= Float::cast(*right).value();
  } else if (runtime->isInstanceOfInt(*right)) {
    Int right_int(&scope, intUnderlying(thread, right));
    result = compareDoubleWithInt(thread, left, right_int, GE);
  } else {
    return NotImplementedType::object();
  }
  return Bool::fromBool(result);
}

RawObject FloatBuiltins::dunderGt(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfFloat(*self)) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "'__gt__' requires a 'float' object");
  }
  double left = Float::cast(*self).value();

  Object right(&scope, args.get(1));
  bool result;
  if (runtime->isInstanceOfFloat(*right)) {
    result = left > Float::cast(*right).value();
  } else if (runtime->isInstanceOfInt(*right)) {
    Int right_int(&scope, intUnderlying(thread, right));
    result = compareDoubleWithInt(thread, left, right_int, GT);
  } else {
    return NotImplementedType::object();
  }
  return Bool::fromBool(result);
}

void FloatBuiltins::decodeDouble(double value, bool* is_neg, int* exp,
                                 uint64_t* mantissa) {
  const uword man_mask = (uword{1} << kDoubleMantissaBits) - 1;
  const uword num_exp_bits = kBitsPerDouble - kDoubleMantissaBits - 1;
  const uword exp_mask = (uword{1} << num_exp_bits) - 1;
  const uword exp_bias = (uword{1} << (num_exp_bits - 1)) - 1;
  uint64_t value_bits = bit_cast<uint64_t>(value);
  *is_neg = value_bits >> (kBitsPerDouble - 1);
  *exp = ((value_bits >> kDoubleMantissaBits) & exp_mask) - exp_bias;
  *mantissa = value_bits & man_mask;
}

RawObject FloatBuiltins::dunderInt(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfFloat(*self_obj)) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "'__int__' requires a 'float' object");
  }
  double dval = Float::cast(*self_obj).value();

  bool is_neg;
  int exp;
  uint64_t man;
  decodeDouble(dval, &is_neg, &exp, &man);
  int exp_bits = kBitsPerDouble - kDoubleMantissaBits - 1;
  int max_exp = 1 << (exp_bits - 1);
  if (exp == max_exp) {
    if (man == 0) {
      return thread->raiseWithFmt(LayoutId::kOverflowError,
                                  "cannot convert float infinity to integer");
    }
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "cannot convert float NaN to integer");
  }

  // No fractional part.
  if (exp < 0) {
    return SmallInt::fromWord(0);
  }
  // Number of bits needed to represent the result integer in 2's complement.
  // +1 for the implicit bit of value 1 and +1 for the sign bit.
  int result_bits = exp + 2;
  // If the number is the negative number of the greatest magnitude
  // (-10000...b), then no extra sign bit is needed.
  if (is_neg && man == 0) {
    result_bits = exp + 1;
  }
  // Fast path for integers that are a word or smaller in size.
  const word man_with_implicit_one = (word{1} << kDoubleMantissaBits) | man;
  // Path that fills a digit of Int, and left-shifts it to match
  // its magnitude with the given exponent.
  DCHECK(
      man_with_implicit_one >= 0,
      "man_with_implicit_one must be positive before the sign bit is applied.");
  if (result_bits <= kBitsPerWord) {
    const word result =
        (exp > kDoubleMantissaBits
             ? (man_with_implicit_one << (exp - kDoubleMantissaBits))
             : (man_with_implicit_one >> (kDoubleMantissaBits - exp)));
    return runtime->newInt(is_neg ? -result : result);
  }
  // TODO(djang): Make another interface for intBInaryLshift() to accept
  // words directly.
  Int unshifted_result(&scope, runtime->newInt(is_neg ? -man_with_implicit_one
                                                      : man_with_implicit_one));
  Int shifting_bits(&scope, runtime->newInt(exp - kDoubleMantissaBits));
  return runtime->intBinaryLshift(thread, unshifted_result, shifting_bits);
}

RawObject FloatBuiltins::dunderLe(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfFloat(*self)) {
    return thread->raiseRequiresType(self, SymbolId::kFloat);
  }
  double left = Float::cast(*self).value();

  Object right(&scope, args.get(1));
  bool result;
  if (runtime->isInstanceOfFloat(*right)) {
    result = left <= Float::cast(*right).value();
  } else if (runtime->isInstanceOfInt(*right)) {
    Int right_int(&scope, intUnderlying(thread, right));
    result = compareDoubleWithInt(thread, left, right_int, LE);
  } else {
    return NotImplementedType::object();
  }
  return Bool::fromBool(result);
}

RawObject FloatBuiltins::dunderLt(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfFloat(*self)) {
    return thread->raiseRequiresType(self, SymbolId::kFloat);
  }
  double left = Float::cast(*self).value();

  Object right(&scope, args.get(1));
  bool result;
  if (runtime->isInstanceOfFloat(*right)) {
    result = left < Float::cast(*right).value();
  } else if (runtime->isInstanceOfInt(*right)) {
    Int right_int(&scope, intUnderlying(thread, right));
    result = compareDoubleWithInt(thread, left, right_int, LT);
  } else {
    return NotImplementedType::object();
  }
  return Bool::fromBool(result);
}

RawObject FloatBuiltins::dunderMul(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfFloat(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kFloat);
  }
  Float self(&scope, *self_obj);
  double left = self.value();

  double right;
  Object other(&scope, args.get(1));
  Object maybe_error(&scope, convertToDouble(thread, other, &right));
  // May have returned NotImplemented or raised an exception.
  if (!maybe_error.isNoneType()) return *maybe_error;

  return runtime->newFloat(left * right);
}

RawObject FloatBuiltins::dunderNeg(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfFloat(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kFloat);
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
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "float.__new__(X): X is not a type object");
  }
  Type type(&scope, *obj);
  if (type.builtinBase() != LayoutId::kFloat) {
    return thread->raiseWithFmt(
        LayoutId::kTypeError, "float.__new__(X): X is not a subtype of float");
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
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object self(&scope, args.get(0));
  if (!runtime->isInstanceOfFloat(*self)) {
    return thread->raiseRequiresType(self, SymbolId::kFloat);
  }
  if (!self.isFloat()) {
    UNIMPLEMENTED("float subclass");
  }
  double left = Float::cast(*self).value();

  double right;
  Object other(&scope, args.get(1));
  Object maybe_error(&scope, convertToDouble(thread, other, &right));
  // May have returned NotImplemented or raised an exception.
  if (!maybe_error.isNoneType()) return *maybe_error;

  return runtime->newFloat(left + right);
}

RawObject FloatBuiltins::dunderTrueDiv(Thread* thread, Frame* frame,
                                       word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);

  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfFloat(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kFloat);
  }
  Float self(&scope, *self_obj);
  double left = self.value();

  double right;
  Object other(&scope, args.get(1));
  Object maybe_error(&scope, convertToDouble(thread, other, &right));
  // May have returned NotImplemented or raised an exception.
  if (!maybe_error.isNoneType()) return *maybe_error;

  if (right == 0.0) {
    return thread->raiseWithFmt(LayoutId::kZeroDivisionError,
                                "float division by zero");
  }
  return runtime->newFloat(left / right);
}

RawObject FloatBuiltins::dunderRepr(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfFloat(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kFloat);
  }
  if (!self_obj.isFloat()) {
    UNIMPLEMENTED("float subclass");
  }
  double value = Float::cast(*self_obj).value();
  int required_size = std::snprintf(nullptr, 0, "%g", value) + 1;  // NUL
  std::unique_ptr<char[]> buffer(new char[required_size]);
  int size = std::snprintf(buffer.get(), required_size, "%g", value);
  CHECK(size < int{required_size}, "buffer too small");
  return runtime->newStrFromCStr(buffer.get());
}

RawObject FloatBuiltins::dunderRtrueDiv(Thread* thread, Frame* frame,
                                        word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);

  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfFloat(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kFloat);
  }
  Float self(&scope, *self_obj);
  double right = self.value();

  double left;
  Object other(&scope, args.get(1));
  Object maybe_error(&scope, convertToDouble(thread, other, &left));
  // May have returned NotImplemented or raised an exception.
  if (!maybe_error.isNoneType()) return *maybe_error;

  if (right == 0.0) {
    return thread->raiseWithFmt(LayoutId::kZeroDivisionError,
                                "float division by zero");
  }
  return runtime->newFloat(left / right);
}

RawObject FloatBuiltins::dunderSub(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfFloat(*self)) {
    return thread->raiseRequiresType(self, SymbolId::kFloat);
  }
  if (!self.isFloat()) {
    UNIMPLEMENTED("float subclass");
  }
  double left = Float::cast(*self).value();

  double right;
  Object other(&scope, args.get(1));
  Object maybe_error(&scope, convertToDouble(thread, other, &right));
  // May have returned NotImplemented or raised an exception.
  if (!maybe_error.isNoneType()) return *maybe_error;

  return runtime->newFloat(left - right);
}

RawObject FloatBuiltins::dunderPow(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfFloat(*self)) {
    return thread->raiseRequiresType(self, SymbolId::kFloat);
  }
  if (!self.isFloat()) {
    UNIMPLEMENTED("float subclass");
  }
  if (!args.get(2).isUnbound()) {
    return thread->raiseWithFmt(
        LayoutId::kTypeError,
        "pow() 3rd argument not allowed unless all arguments are integers");
  }
  double left = Float::cast(*self).value();

  double right;
  Object other(&scope, args.get(1));
  Object maybe_error(&scope, convertToDouble(thread, other, &right));
  // May have returned NotImplemented or raised an exception.
  if (!maybe_error.isNoneType()) return *maybe_error;

  return runtime->newFloat(std::pow(left, right));
}

}  // namespace python
