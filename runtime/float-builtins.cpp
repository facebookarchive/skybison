#include "float-builtins.h"

#include <cfloat>
#include <cmath>
#include <limits>

#include "float-conversion.h"
#include "frame.h"
#include "globals.h"
#include "int-builtins.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace py {

RawObject asFloatObject(Thread* thread, const Object& obj) {
  // Object is float
  if (obj.isFloat()) return *obj;

  // Object is subclass of float
  Runtime* runtime = thread->runtime();
  if (runtime->isInstanceOfFloat(*obj)) {
    return floatUnderlying(thread, obj);
  }

  // Try calling __float__
  HandleScope scope(thread);
  Object flt_obj(&scope, thread->invokeMethod1(obj, SymbolId::kDunderFloat));
  if (flt_obj.isError()) {
    if (flt_obj.isErrorNotFound()) {
      return thread->raiseWithFmt(LayoutId::kTypeError,
                                  "must be a real number");
    }
    return *flt_obj;
  }
  if (flt_obj.isFloat()) return *flt_obj;
  if (!runtime->isInstanceOfFloat(*flt_obj)) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "%T.__float__ returned non-float (type %T)",
                                &obj, &flt_obj);
  }
  return floatUnderlying(thread, flt_obj);
}

// Convert `object` to double.
// Returns a NoneType and sets `value` if the conversion was successful.
// Returns an error or unimplemented otherwise. This does specifically not
// look for `__float__` to match the behavior of `CONVERT_TO_DOUBLE()` in
// cpython.
static RawObject convertToDouble(Thread* thread, const Object& object,
                                 double* result) {
  Runtime* runtime = thread->runtime();
  if (runtime->isInstanceOfFloat(*object)) {
    *result = Float::cast(floatUnderlying(thread, object)).value();
    return NoneType::object();
  }
  if (runtime->isInstanceOfInt(*object)) {
    HandleScope scope(thread);
    Int value(&scope, intUnderlying(thread, object));
    return convertIntToDouble(thread, value, result);
  }
  return NotImplementedType::object();
}

RawObject floatUnderlying(Thread* thread, const Object& obj) {
  if (obj.isFloat()) return *obj;
  DCHECK(thread->runtime()->isInstanceOfFloat(*obj),
         "cannot get a base float value from a non-float");
  HandleScope scope(thread);
  UserFloatBase user_float(&scope, *obj);
  return user_float.value();
}

const BuiltinMethod FloatBuiltins::kBuiltinMethods[] = {
    {SymbolId::kDunderAbs, dunderAbs},
    {SymbolId::kDunderAdd, dunderAdd},
    {SymbolId::kDunderBool, dunderBool},
    {SymbolId::kDunderEq, dunderEq},
    {SymbolId::kDunderFloat, dunderFloat},
    {SymbolId::kDunderGe, dunderGe},
    {SymbolId::kDunderGt, dunderGt},
    {SymbolId::kDunderHash, dunderHash},
    {SymbolId::kDunderInt, dunderInt},
    {SymbolId::kDunderLe, dunderLe},
    {SymbolId::kDunderLt, dunderLt},
    {SymbolId::kDunderMul, dunderMul},
    {SymbolId::kDunderNeg, dunderNeg},
    {SymbolId::kDunderNew, dunderNew},
    {SymbolId::kDunderPow, dunderPow},
    {SymbolId::kDunderRound, dunderRound},
    {SymbolId::kDunderRtruediv, dunderRtrueDiv},
    {SymbolId::kDunderSub, dunderSub},
    {SymbolId::kDunderTruediv, dunderTrueDiv},
    {SymbolId::kDunderTrunc, dunderTrunc},
    {SymbolId::kSentinelId, nullptr},
};

const BuiltinAttribute FloatBuiltins::kAttributes[] = {
    {SymbolId::kInvalid, UserFloatBase::kValueOffset},
    {SymbolId::kSentinelId, 0},
};

RawObject FloatBuiltins::floatFromObject(Thread* thread, Frame* frame,
                                         word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object obj(&scope, args.get(1));
  if (runtime->isInstanceOfFloat(*obj)) {
    return floatUnderlying(thread, obj);
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
    if (method.isErrorNotFound()) {
      return thread->raiseWithFmt(
          LayoutId::kTypeError,
          "TypeError: float() argument must have a __float__");
    }
    return *method;
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
  Float self(&scope, floatUnderlying(thread, self_obj));
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
  Float self(&scope, floatUnderlying(thread, self_obj));
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
  double left = Float::cast(floatUnderlying(thread, self)).value();

  Object right(&scope, args.get(1));
  bool result;
  if (runtime->isInstanceOfFloat(*right)) {
    result = left == Float::cast(floatUnderlying(thread, right)).value();
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
  if (!thread->runtime()->isInstanceOfFloat(*self)) {
    return thread->raiseRequiresType(self, SymbolId::kFloat);
  }
  return floatUnderlying(thread, self);
}

RawObject FloatBuiltins::dunderGe(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfFloat(*self)) {
    return thread->raiseRequiresType(self, SymbolId::kFloat);
  }
  double left = Float::cast(floatUnderlying(thread, self)).value();

  Object right(&scope, args.get(1));
  bool result;
  if (runtime->isInstanceOfFloat(*right)) {
    result = left >= Float::cast(floatUnderlying(thread, right)).value();
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
    return thread->raiseRequiresType(self, SymbolId::kFloat);
  }
  double left = Float::cast(floatUnderlying(thread, self)).value();

  Object right(&scope, args.get(1));
  bool result;
  if (runtime->isInstanceOfFloat(*right)) {
    result = left > Float::cast(floatUnderlying(thread, right)).value();
  } else if (runtime->isInstanceOfInt(*right)) {
    Int right_int(&scope, intUnderlying(thread, right));
    result = compareDoubleWithInt(thread, left, right_int, GT);
  } else {
    return NotImplementedType::object();
  }
  return Bool::fromBool(result);
}

void decodeDouble(double value, bool* is_neg, int* exp, uint64_t* mantissa) {
  const uint64_t man_mask = (uint64_t{1} << kDoubleMantissaBits) - 1;
  const int num_exp_bits = kBitsPerDouble - kDoubleMantissaBits - 1;
  const uint64_t exp_mask = (uint64_t{1} << num_exp_bits) - 1;
  const int exp_bias = (1 << (num_exp_bits - 1)) - 1;
  uint64_t value_bits = bit_cast<uint64_t>(value);
  *is_neg = value_bits >> (kBitsPerDouble - 1);
  *exp = ((value_bits >> kDoubleMantissaBits) & exp_mask) - exp_bias;
  *mantissa = value_bits & man_mask;
}

static RawObject intFromDouble(Thread* thread, double value) {
  bool is_neg;
  int exp;
  uint64_t man;
  decodeDouble(value, &is_neg, &exp, &man);
  const int exp_bits = kBitsPerDouble - kDoubleMantissaBits - 1;
  const int max_exp = 1 << (exp_bits - 1);
  if (exp == max_exp) {
    if (man == 0) {
      return thread->raiseWithFmt(LayoutId::kOverflowError,
                                  "cannot convert float infinity to integer");
    }
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "cannot convert float NaN to integer");
  }

  // No integral part.
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
  Runtime* runtime = thread->runtime();
  if (result_bits <= kBitsPerWord) {
    const word result =
        (exp > kDoubleMantissaBits
             ? (man_with_implicit_one << (exp - kDoubleMantissaBits))
             : (man_with_implicit_one >> (kDoubleMantissaBits - exp)));
    return runtime->newInt(is_neg ? -result : result);
  }
  // TODO(djang): Make another interface for intBinaryLshift() to accept
  // words directly.
  HandleScope scope(thread);
  Int unshifted_result(&scope, runtime->newInt(is_neg ? -man_with_implicit_one
                                                      : man_with_implicit_one));
  Int shifting_bits(&scope, runtime->newInt(exp - kDoubleMantissaBits));
  return runtime->intBinaryLshift(thread, unshifted_result, shifting_bits);
}

word doubleHash(double value) {
  bool is_neg;
  int exp;
  uint64_t mantissa;
  decodeDouble(value, &is_neg, &exp, &mantissa);
  const int exp_bits = kBitsPerDouble - kDoubleMantissaBits - 1;
  const int max_exp = 1 << (exp_bits - 1);
  const int min_exp = -(1 << (exp_bits - 1)) + 1;

  if (exp == max_exp) {
    word result;
    if (mantissa == 0) {
      result = is_neg ? -kHashInf : kHashInf;
    } else {
      result = kHashNan;
    }
    return result;
  }

  // The problem in the following is that for float numbers that compare equal
  // to an int number, the hash values have to equal the hash values produced
  // when hashing the integer. To achieve this we base the hashing on the same
  // ideas as `longIntHash()`. Here we want to compute
  // `(mantissa << (exp - mantissa_bits)) % kArithmeticHashModulus`.
  // `mantissa` is guaranteed to be smaller than `kArithmeticHashModulus` so as
  // explained in `longIntHash()` this just means we have to rotate it's bits by
  // `exp` for the result.

  // Add implicit one to mantissa if the number is not a subnormal.
  if (exp > min_exp) {
    mantissa |= uint64_t{1} << kDoubleMantissaBits;
  } else if (mantissa == 0) {
    // Shortcut for 0.0 / -0.0.
    return 0;
  } else {
    // sub-normal number, adjust exponent.
    exp += 1;
  }

  // Compute `mantissa % kArithmeticHashModulus` which is just `mantissa`.
  static_assert(uword{1} << (kDoubleMantissaBits + 1) < kArithmeticHashModulus,
                "assumption `mantissa < modulus` does not hold");
  uword result = mantissa;

  // `mantissa` represented `kDoubleMantissaBits` bits shifted by `exp`. We want
  // to align the first integral bit to bit 0 in the result, so we have to
  // rotate by `exp - kDoubleMantissaBits`.
  exp -= kDoubleMantissaBits;
  exp = exp >= 0 ? exp % kArithmeticHashBits
                 : kArithmeticHashBits - 1 - ((-1 - exp) % kArithmeticHashBits);
  result = ((result << exp) & kArithmeticHashModulus) |
           result >> (kArithmeticHashBits - exp);

  if (is_neg) {
    result = -result;
  }

  // cpython replaces `-1` results with -2, because -1 is used as an
  // "uninitialized hash" marker in some situations. We do not use the same
  // marker, but do the same to match behavior.
  if (result == static_cast<uword>(word{-1})) {
    result--;
  }

  // Note: We cannot cache the hash value in the object header, because the
  // result must correspond to the hash values of SmallInt/LargeInt. The object
  // header however has fewer bits and can only store non-negative hash codes.
  return static_cast<word>(result);
}

RawObject FloatBuiltins::dunderHash(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfFloat(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kFloat);
  }
  Object self_float(&scope, floatUnderlying(thread, self_obj));
  return SmallInt::fromWord(floatHash(*self_float));
}

RawObject FloatBuiltins::dunderInt(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfFloat(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kFloat);
  }
  double self = Float::cast(floatUnderlying(thread, self_obj)).value();
  return intFromDouble(thread, self);
}

RawObject FloatBuiltins::dunderLe(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfFloat(*self)) {
    return thread->raiseRequiresType(self, SymbolId::kFloat);
  }
  double left = Float::cast(floatUnderlying(thread, self)).value();

  Object right(&scope, args.get(1));
  bool result;
  if (runtime->isInstanceOfFloat(*right)) {
    result = left <= Float::cast(floatUnderlying(thread, right)).value();
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
  double left = Float::cast(floatUnderlying(thread, self)).value();

  Object right(&scope, args.get(1));
  bool result;
  if (runtime->isInstanceOfFloat(*right)) {
    result = left < Float::cast(floatUnderlying(thread, right)).value();
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
  Float self(&scope, floatUnderlying(thread, self_obj));
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
  Float self(&scope, floatUnderlying(thread, self_obj));
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
    instance.setValue(floatFromObject(thread, frame, nargs));
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
  double left = Float::cast(floatUnderlying(thread, self)).value();

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
  Float self(&scope, floatUnderlying(thread, self_obj));
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

RawObject FloatBuiltins::dunderRound(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfFloat(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kFloat);
  }
  Float value_float(&scope, floatUnderlying(thread, self_obj));
  double value = value_float.value();

  // If ndigits is None round to nearest integer.
  Object ndigits_obj(&scope, args.get(1));
  if (ndigits_obj.isNoneType()) {
    double result = std::round(value);
    // round to even.
    if (std::fabs(value - result) == 0.5) {
      result = 2.0 * std::round(value / 2.0);
    }
    return intFromDouble(thread, result);
  }

  // Round to ndigits decimals.
  if (!runtime->isInstanceOfInt(*ndigits_obj)) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "'%T' cannot be interpreted as an integer",
                                &ndigits_obj);
  }
  Int ndigits_int(&scope, intUnderlying(thread, ndigits_obj));
  if (ndigits_int.isLargeInt()) {
    return ndigits_int.isNegative() ? runtime->newFloat(0.0) : *value_float;
  }
  word ndigits = ndigits_int.asWord();

  // Keep NaNs and infinities unchanged.
  if (!std::isfinite(value)) {
    return *value_float;
  }

  // Set some reasonable bounds on ndigits and clip otherwise.
  // For `ndigits > ndigits_max`, `value` always rounds to itself.
  // For `ndigits < ndigits_min`, `value` always rounds to +-0.0.
  // Here 0.30103 is an upper bound for `log10(2)`.
  static const word ndigits_max =
      static_cast<word>((DBL_MANT_DIG - DBL_MIN_EXP) * 0.30103);
  static const word ndigits_min =
      -static_cast<word>((DBL_MAX_EXP + 1) * 0.30103);
  if (ndigits > ndigits_max) {
    return *value_float;
  }
  double result;
  if (ndigits < ndigits_min) {
    result = std::copysign(0.0, value);
  } else {
    result = doubleRoundDecimals(value, static_cast<int>(ndigits));
    if (result == HUGE_VAL || result == -HUGE_VAL) {
      return thread->raiseWithFmt(LayoutId::kOverflowError,
                                  "rounded value too large to represent");
    }
  }
  return runtime->newFloat(result);
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
  Float self(&scope, floatUnderlying(thread, self_obj));
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
  double left = Float::cast(floatUnderlying(thread, self)).value();

  double right;
  Object other(&scope, args.get(1));
  Object maybe_error(&scope, convertToDouble(thread, other, &right));
  // May have returned NotImplemented or raised an exception.
  if (!maybe_error.isNoneType()) return *maybe_error;

  return runtime->newFloat(left - right);
}

RawObject FloatBuiltins::dunderTrunc(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfFloat(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kFloat);
  }
  double self = Float::cast(floatUnderlying(thread, self_obj)).value();
  double integral_part;
  static_cast<void>(std::modf(self, &integral_part));
  return intFromDouble(thread, integral_part);
}

RawObject FloatBuiltins::dunderPow(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfFloat(*self)) {
    return thread->raiseRequiresType(self, SymbolId::kFloat);
  }
  if (!args.get(2).isNoneType()) {
    return thread->raiseWithFmt(
        LayoutId::kTypeError,
        "pow() 3rd argument not allowed unless all arguments are integers");
  }
  double left = Float::cast(floatUnderlying(thread, self)).value();

  double right;
  Object other(&scope, args.get(1));
  Object maybe_error(&scope, convertToDouble(thread, other, &right));
  // May have returned NotImplemented or raised an exception.
  if (!maybe_error.isNoneType()) return *maybe_error;

  return runtime->newFloat(std::pow(left, right));
}

}  // namespace py
