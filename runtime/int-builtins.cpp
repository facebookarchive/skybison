#include "int-builtins.h"

#include <cinttypes>
#include <climits>
#include <cmath>

#include "bytes-builtins.h"
#include "formatter.h"
#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "str-builtins.h"
#include "thread.h"

namespace python {

void IntBuiltins::postInitialize(Runtime*, const Type& new_type) {
  new_type.setBuiltinBase(LayoutId::kInt);
}

void SmallIntBuiltins::postInitialize(Runtime* runtime, const Type& new_type) {
  new_type.setBuiltinBase(kSuperType);
  runtime->setSmallIntType(new_type);
  Layout::cast(new_type.instanceLayout())
      .setDescribedType(runtime->typeAt(kSuperType));
  // We want to lookup the class of an immediate type by using the 5-bit tag
  // value as an index into the class table.  Replicate the class object for
  // SmallInt to all locations that decode to a SmallInt tag.
  for (word i = 2; i < (1 << Object::kImmediateTagBits); i += 2) {
    DCHECK(runtime->layoutAt(static_cast<LayoutId>(i)) == NoneType::object(),
           "list collision");
    runtime->layoutAtPut(static_cast<LayoutId>(i), *new_type);
  }
}

void LargeIntBuiltins::postInitialize(Runtime* runtime, const Type& new_type) {
  new_type.setBuiltinBase(kSuperType);
  runtime->setLargeIntType(new_type);
  Layout::cast(new_type.instanceLayout())
      .setDescribedType(runtime->typeAt(kSuperType));
}

// Used only for UserIntBase as a heap-allocated object.
const BuiltinAttribute IntBuiltins::kAttributes[] = {
    {SymbolId::kInvalid, UserIntBase::kValueOffset},
    {SymbolId::kSentinelId, 0},
};

const BuiltinMethod IntBuiltins::kBuiltinMethods[] = {
    {SymbolId::kBitLength, bitLength},
    {SymbolId::kConjugate, dunderInt},
    {SymbolId::kDunderAbs, dunderAbs},
    {SymbolId::kDunderAdd, dunderAdd},
    {SymbolId::kDunderAnd, dunderAnd},
    {SymbolId::kDunderBool, dunderBool},
    {SymbolId::kDunderCeil, dunderInt},
    {SymbolId::kDunderDivmod, dunderDivmod},
    {SymbolId::kDunderEq, dunderEq},
    {SymbolId::kDunderFloat, dunderFloat},
    {SymbolId::kDunderFloor, dunderInt},
    {SymbolId::kDunderFloordiv, dunderFloordiv},
    {SymbolId::kDunderFormat, dunderFormat},
    {SymbolId::kDunderGe, dunderGe},
    {SymbolId::kDunderGt, dunderGt},
    {SymbolId::kDunderIndex, dunderInt},
    {SymbolId::kDunderInt, dunderInt},
    {SymbolId::kDunderInvert, dunderInvert},
    {SymbolId::kDunderLe, dunderLe},
    {SymbolId::kDunderLshift, dunderLshift},
    {SymbolId::kDunderLt, dunderLt},
    {SymbolId::kDunderMod, dunderMod},
    {SymbolId::kDunderMul, dunderMul},
    {SymbolId::kDunderNe, dunderNe},
    {SymbolId::kDunderNeg, dunderNeg},
    {SymbolId::kDunderOr, dunderOr},
    {SymbolId::kDunderPos, dunderInt},
    {SymbolId::kDunderRepr, dunderRepr},
    {SymbolId::kDunderRound, dunderInt},
    {SymbolId::kDunderRshift, dunderRshift},
    {SymbolId::kDunderStr, dunderRepr},
    {SymbolId::kDunderSub, dunderSub},
    {SymbolId::kDunderTruediv, dunderTrueDiv},
    {SymbolId::kDunderTrunc, dunderInt},
    {SymbolId::kDunderXor, dunderXor},
    {SymbolId::kToBytes, toBytes},
    {SymbolId::kSentinelId, nullptr},
};

RawObject convertBoolToInt(RawObject object) {
  DCHECK(object.isBool(), "conversion from bool to int requires a bool object");
  return RawSmallInt::fromWord(object == RawBool::trueObj() ? 1 : 0);
}

static RawObject intBinaryOpSubclass(Thread* thread, Frame* frame, word nargs,
                                     RawObject (*op)(Thread* t, const Int& left,
                                                     const Int& right)) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Object other_obj(&scope, args.get(1));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfInt(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kInt);
  }
  if (!runtime->isInstanceOfInt(*other_obj)) {
    return NotImplementedType::object();
  }
  Int self(&scope, intUnderlying(thread, self_obj));
  Int other(&scope, intUnderlying(thread, other_obj));
  return op(thread, self, other);
}

inline static RawObject intBinaryOp(Thread* thread, Frame* frame, word nargs,
                                    RawObject (*op)(Thread* t, const Int& left,
                                                    const Int& right)) {
  Arguments args(frame, nargs);
  if (args.get(0).isInt() && args.get(1).isInt()) {
    HandleScope scope(thread);
    Int self(&scope, args.get(0));
    Int other(&scope, args.get(1));
    return op(thread, self, other);
  }
  return intBinaryOpSubclass(thread, frame, nargs, op);
}

static RawObject intUnaryOp(Thread* thread, Frame* frame, word nargs,
                            RawObject (*op)(Thread* t, const Int& self)) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfInt(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kInt);
  }
  Int self(&scope, intUnderlying(thread, self_obj));
  return op(thread, self);
}

RawObject IntBuiltins::dunderInt(Thread* thread, Frame* frame, word nargs) {
  return intUnaryOp(thread, frame, nargs,
                    [](Thread*, const Int& self) -> RawObject {
                      if (self.isBool()) {
                        return convertBoolToInt(*self);
                      }
                      return *self;
                    });
}

RawObject IntBuiltins::bitLength(Thread* thread, Frame* frame, word nargs) {
  return intUnaryOp(thread, frame, nargs, [](Thread* t, const Int& self) {
    return t->runtime()->newInt(self.bitLength());
  });
}

RawObject IntBuiltins::dunderAbs(Thread* thread, Frame* frame, word nargs) {
  return intUnaryOp(thread, frame, nargs,
                    [](Thread* t, const Int& self) -> RawObject {
                      if (self.isNegative()) {
                        return t->runtime()->intNegate(t, self);
                      }
                      if (self.isBool()) return convertBoolToInt(*self);
                      return *self;
                    });
}

RawObject IntBuiltins::dunderAdd(Thread* thread, Frame* frame, word nargs) {
  return intBinaryOp(thread, frame, nargs,
                     [](Thread* t, const Int& left, const Int& right) {
                       return t->runtime()->intAdd(t, left, right);
                     });
}

RawObject IntBuiltins::dunderAnd(Thread* thread, Frame* frame, word nargs) {
  return intBinaryOp(thread, frame, nargs,
                     [](Thread* t, const Int& left, const Int& right) {
                       return t->runtime()->intBinaryAnd(t, left, right);
                     });
}

RawObject IntBuiltins::dunderBool(Thread* thread, Frame* frame, word nargs) {
  return intUnaryOp(
      thread, frame, nargs, [](Thread*, const Int& self) -> RawObject {
        if (self.isBool()) return *self;
        if (self.isSmallInt()) {
          return Bool::fromBool(SmallInt::cast(*self).value() != 0);
        }
        DCHECK(self.isLargeInt(), "remaining case should be LargeInt");
        return Bool::trueObj();
      });
}

RawObject IntBuiltins::dunderEq(Thread* thread, Frame* frame, word nargs) {
  return intBinaryOp(
      thread, frame, nargs,
      [](Thread*, const Int& left, const Int& right) -> RawObject {
        return Bool::fromBool(left.compare(*right) == 0);
      });
}

RawObject IntBuiltins::dunderDivmod(Thread* thread, Frame* frame, word nargs) {
  return intBinaryOp(
      thread, frame, nargs,
      [](Thread* t, const Int& left, const Int& right) -> RawObject {
        HandleScope scope(t);
        Object quotient(&scope, NoneType::object());
        Object remainder(&scope, NoneType::object());
        Runtime* runtime = t->runtime();
        if (!runtime->intDivideModulo(t, left, right, &quotient, &remainder)) {
          return t->raiseWithFmt(LayoutId::kZeroDivisionError,
                                 "integer division or modulo by zero");
        }
        Tuple result(&scope, runtime->newTuple(2));
        result.atPut(0, *quotient);
        result.atPut(1, *remainder);
        return *result;
      });
}

RawObject IntBuiltins::dunderFloat(Thread* thread, Frame* frame, word nargs) {
  return intUnaryOp(thread, frame, nargs, [](Thread* t, const Int& self) {
    HandleScope scope(t);
    double value = 0.0;
    Object maybe_error(&scope, convertIntToDouble(t, self, &value));
    if (!maybe_error.isNoneType()) return *maybe_error;
    return t->runtime()->newFloat(value);
  });
}

RawObject IntBuiltins::dunderInvert(Thread* thread, Frame* frame, word nargs) {
  return intUnaryOp(thread, frame, nargs, [](Thread* t, const Int& self) {
    return t->runtime()->intInvert(t, self);
  });
}

RawObject IntBuiltins::dunderFloordiv(Thread* thread, Frame* frame,
                                      word nargs) {
  return intBinaryOp(
      thread, frame, nargs, [](Thread* t, const Int& left, const Int& right) {
        HandleScope scope(t);
        Object quotient(&scope, NoneType::object());
        if (!t->runtime()->intDivideModulo(t, left, right, &quotient,
                                           nullptr)) {
          return t->raiseWithFmt(LayoutId::kZeroDivisionError,
                                 "integer division or modulo by zero");
        }
        return *quotient;
      });
}

static RawObject formatIntCodePoint(Thread* thread, const Int& value,
                                    FormatSpec* format) {
  if (value.isLargeInt()) {
    return thread->raiseWithFmt(LayoutId::kOverflowError,
                                "Python int too large to convert to C long");
  }
  word value_word = value.asWord();
  if (value_word < 0 || value_word > kMaxUnicode) {
    static_assert(kMaxUnicode == 0x10ffff, "unexpected max unicode value");
    return thread->raiseWithFmt(LayoutId::kOverflowError,
                                "%%c arg not in range(0x110000)");
  }
  HandleScope scope(thread);
  Str code_point(&scope,
                 SmallStr::fromCodePoint(static_cast<int32_t>(value_word)));
  if (format->precision >= 0) {
    return thread->raiseWithFmt(
        LayoutId::kValueError,
        "Precision not allowed in integer format specifier");
  }
  if (format->positive_sign != '\0') {
    return thread->raiseWithFmt(
        LayoutId::kValueError,
        "Sign not allowed with integer format specifier 'c'");
  }
  if (format->alternate) {
    return thread->raiseWithFmt(
        LayoutId::kValueError,
        "Alternate form (#) not allowed with integer format specifier 'c'");
  }
  return formatStr(thread, code_point, format);
}

RawObject IntBuiltins::dunderFormat(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfInt(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kInt);
  }
  Int self(&scope, intUnderlying(thread, self_obj));

  Object spec_obj(&scope, args.get(1));
  if (!runtime->isInstanceOfStr(*spec_obj)) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "__format__() argument 1 must be str, not %T",
                                &spec_obj);
  }
  Str spec(&scope, strUnderlying(thread, spec_obj));

  if (spec.charLength() == 0) {
    return formatIntDecimalSimple(thread, self);
  }

  FormatSpec format;
  Object possible_error(&scope,
                        parseFormatSpec(thread, spec,
                                        /*default_type=*/'d',
                                        /*default_align=*/'>', &format));
  if (!possible_error.isNoneType()) {
    DCHECK(possible_error.isErrorException(), "expected exception");
    return *possible_error;
  }

  switch (format.type) {
    case 'b':
      return formatIntBinary(thread, self, &format);
    case 'c':
      return formatIntCodePoint(thread, self, &format);
    case 'd':
      return formatIntDecimal(thread, self, &format);
    case 'n':
      UNIMPLEMENTED("print with locale thousands separator");
    case 'o':
      return formatIntOctal(thread, self, &format);
    case 'x':
      return formatIntHexadecimalLowerCase(thread, self, &format);
    case 'X':
      return formatIntHexadecimalUpperCase(thread, self, &format);
    case '%':
    case 'e':
    case 'E':
    case 'f':
    case 'F':
    case 'g':
    case 'G':
      // TODO(matthiasb): convert to float and call formatFloat().
      UNIMPLEMENTED("print int as float");
    default:
      return raiseUnknownFormatError(thread, format.type, self_obj);
  }
}

RawObject IntBuiltins::dunderLe(Thread* thread, Frame* frame, word nargs) {
  return intBinaryOp(
      thread, frame, nargs,
      [](Thread*, const Int& left, const Int& right) -> RawObject {
        return Bool::fromBool(left.compare(*right) <= 0);
      });
}

static RawObject toBytesImpl(Thread* thread, const Object& self_obj,
                             const Object& length_obj,
                             const Object& byteorder_obj, bool is_signed) {
  HandleScope scope;
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfInt(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kInt);
  }
  Int self(&scope, intUnderlying(thread, self_obj));
  if (!runtime->isInstanceOfInt(*length_obj)) {
    return thread->raiseWithFmt(
        LayoutId::kTypeError,
        "length argument cannot be interpreted as an integer");
  }
  Int length_int(&scope, intUnderlying(thread, length_obj));
  OptInt<word> l = length_int.asInt<word>();
  if (l.error != CastError::None) {
    return thread->raiseWithFmt(LayoutId::kOverflowError,
                                "Python int too large to convert to C word");
  }
  word length = l.value;
  if (length < 0) {
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "length argument must be non-negative");
  }

  if (!runtime->isInstanceOfStr(*byteorder_obj)) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "to_bytes() argument 2 must be str, not int");
  }
  Str byteorder(&scope, *byteorder_obj);
  endian endianness;
  if (byteorder.equals(runtime->symbols()->Little())) {
    endianness = endian::little;
  } else if (byteorder.equals(runtime->symbols()->Big())) {
    endianness = endian::big;
  } else {
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "byteorder must be either 'little' or 'big'");
  }

  if (!is_signed && self.isNegative()) {
    return thread->raiseWithFmt(LayoutId::kOverflowError,
                                "can't convert negative int to unsigned");
  }

  // Check for overflow.
  word num_digits = self.numDigits();
  uword high_digit = self.digitAt(num_digits - 1);
  word bit_length =
      num_digits * kBitsPerWord - Utils::numRedundantSignBits(high_digit);
  if (bit_length > length * kBitsPerByte + !is_signed) {
    return thread->raiseWithFmt(LayoutId::kOverflowError,
                                "int too big to convert");
  }

  return runtime->intToBytes(thread, self, length, endianness);
}

RawObject IntBuiltins::toBytes(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  Object length(&scope, args.get(1));
  Object byteorder(&scope, args.get(2));
  if (!args.get(3).isBool()) {
    return thread->raiseWithFmt(LayoutId::kTypeError, "signed must be bool");
  }
  return toBytesImpl(thread, self, length, byteorder,
                     Bool::cast(args.get(3)).value());
}

RawObject IntBuiltins::dunderLt(Thread* thread, Frame* frame, word nargs) {
  return intBinaryOp(
      thread, frame, nargs,
      [](Thread*, const Int& left, const Int& right) -> RawObject {
        return Bool::fromBool(left.compare(*right) < 0);
      });
}

RawObject IntBuiltins::dunderGe(Thread* thread, Frame* frame, word nargs) {
  return intBinaryOp(
      thread, frame, nargs,
      [](Thread*, const Int& left, const Int& right) -> RawObject {
        return Bool::fromBool(left.compare(*right) >= 0);
      });
}

RawObject IntBuiltins::dunderGt(Thread* thread, Frame* frame, word nargs) {
  return intBinaryOp(
      thread, frame, nargs,
      [](Thread*, const Int& left, const Int& right) -> RawObject {
        return Bool::fromBool(left.compare(*right) > 0);
      });
}

RawObject IntBuiltins::dunderMod(Thread* thread, Frame* frame, word nargs) {
  return intBinaryOp(
      thread, frame, nargs, [](Thread* t, const Int& left, const Int& right) {
        HandleScope scope(t);
        Object remainder(&scope, NoneType::object());
        if (!t->runtime()->intDivideModulo(t, left, right, nullptr,
                                           &remainder)) {
          return t->raiseWithFmt(LayoutId::kZeroDivisionError,
                                 "integer division or modulo by zero");
        }
        return *remainder;
      });
}

RawObject IntBuiltins::dunderMul(Thread* thread, Frame* frame, word nargs) {
  return intBinaryOp(thread, frame, nargs,
                     [](Thread* t, const Int& left, const Int& right) {
                       return t->runtime()->intMultiply(t, left, right);
                     });
}

RawObject IntBuiltins::dunderNe(Thread* thread, Frame* frame, word nargs) {
  return intBinaryOp(
      thread, frame, nargs,
      [](Thread*, const Int& left, const Int& right) -> RawObject {
        return Bool::fromBool(left.compare(*right) != 0);
      });
}

RawObject IntBuiltins::dunderNeg(Thread* thread, Frame* frame, word nargs) {
  return intUnaryOp(thread, frame, nargs, [](Thread* t, const Int& self) {
    return t->runtime()->intNegate(t, self);
  });
}

RawObject IntBuiltins::dunderRshift(Thread* thread, Frame* frame, word nargs) {
  return intBinaryOp(
      thread, frame, nargs, [](Thread* t, const Int& left, const Int& right) {
        if (right.isNegative()) {
          return t->raiseWithFmt(LayoutId::kValueError, "negative shift count");
        }
        return t->runtime()->intBinaryRshift(t, left, right);
      });
}

RawObject IntBuiltins::dunderSub(Thread* thread, Frame* frame, word nargs) {
  return intBinaryOp(thread, frame, nargs,
                     [](Thread* t, const Int& left, const Int& right) {
                       return t->runtime()->intSubtract(t, left, right);
                     });
}

RawObject IntBuiltins::dunderTrueDiv(Thread* thread, Frame* frame, word nargs) {
  return intBinaryOp(
      thread, frame, nargs, [](Thread* t, const Int& left, const Int& right) {
        if (right.isZero()) {
          return t->raiseWithFmt(LayoutId::kZeroDivisionError,
                                 "division by zero");
        }
        if (left.isLargeInt() || right.isLargeInt()) {
          UNIMPLEMENTED("true division of LargeInts");  // TODO(T40072578)
        }
        return t->runtime()->newFloat(static_cast<double>(left.asWord()) /
                                      right.asWord());
      });
}

RawObject IntBuiltins::dunderXor(Thread* thread, Frame* frame, word nargs) {
  return intBinaryOp(thread, frame, nargs,
                     [](Thread* t, const Int& left, const Int& right) {
                       return t->runtime()->intBinaryXor(t, left, right);
                     });
}

RawObject IntBuiltins::dunderOr(Thread* thread, Frame* frame, word nargs) {
  return intBinaryOp(thread, frame, nargs,
                     [](Thread* t, const Int& left, const Int& right) {
                       return t->runtime()->intBinaryOr(t, left, right);
                     });
}

RawObject IntBuiltins::dunderLshift(Thread* thread, Frame* frame, word nargs) {
  return intBinaryOp(
      thread, frame, nargs, [](Thread* t, const Int& left, const Int& right) {
        if (right.isNegative()) {
          return t->raiseWithFmt(LayoutId::kValueError, "negative shift count");
        }
        return t->runtime()->intBinaryLshift(t, left, right);
      });
}

RawObject IntBuiltins::dunderRepr(Thread* thread, Frame* frame, word nargs) {
  return intUnaryOp(thread, frame, nargs, [](Thread* t, const Int& self) {
    return formatIntDecimalSimple(t, self);
  });
}

RawObject BoolBuiltins::dunderNew(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object type_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfType(*type_obj)) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "bool.__new__(X): X is not a type object");
  }
  Type type(&scope, *type_obj);

  // Since bool can't be subclassed, only need to check if the type is exactly
  // bool.
  Layout layout(&scope, type.instanceLayout());
  if (layout.id() != LayoutId::kBool) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "bool.__new__(X): X is not bool");
  }

  return Interpreter::isTrue(thread, args.get(1));
}

const BuiltinMethod BoolBuiltins::kBuiltinMethods[] = {
    {SymbolId::kDunderNew, dunderNew},
    {SymbolId::kSentinelId, nullptr},
};

enum RoundingDirection {
  RoundDown = -1,
  NoRounding = 0,
  RoundUp = 1,
};

// Convert a large int to double.  Returns true and sets `result` if the
// conversion was successful, false if the integer is too big to fit the
// double range.  If `rounding_direction` is not nullptr, it will be set to a
// value indicating what rounding occured.
static inline bool convertLargeIntToDouble(const LargeInt& large_int,
                                           double* result,
                                           RoundingDirection* rounding) {
  // The following algorithm looks at the highest n bits of the integer and puts
  // them into the mantissa of the floating point number. It extracts two
  // extra bits to account for the highest bit not being explicitly encoded
  // in floating point and the lowest bit to decide whether we should round
  // up or down.

  // Extract the highest two digits of the numbers magnitude.
  word num_digits = large_int.numDigits();
  DCHECK(num_digits > 1, "must have more than 1 digit");
  uword high_digit = large_int.digitAt(num_digits - 1);
  uword second_highest_digit = large_int.digitAt(num_digits - 2);
  bool is_negative = large_int.isNegative();
  uword carry_to_second_highest = 0;
  if (is_negative) {
    // The magnitude of a negative value is `~value + 1`. We compute the
    // complement of the highest two digits and possibly add a carry.
    carry_to_second_highest = 1;
    for (word i = num_digits - 3; i >= 0; i--) {
      // Any `digit != 0` will have a zero bit so we won't have a carry.
      if (large_int.digitAt(i) != 0) {
        carry_to_second_highest = 0;
        break;
      }
    }
    second_highest_digit = ~second_highest_digit + carry_to_second_highest;
    uword carry_to_highest = second_highest_digit == 0 ? 1 : 0;
    high_digit = ~high_digit + carry_to_highest;
    // A negative number has the highest bit set so incrementing the complement
    // cannot overflow.
    DCHECK(carry_to_highest == 0 || high_digit != 0,
           "highest digit cannot overflow");
  }

  // Determine the exponent bits.
  int high_bit = Utils::highestBit(high_digit);
  uword exponent_bits = kBitsPerDouble - kDoubleMantissaBits - 1;
  uword exponent_bias = (1 << (exponent_bits - 1)) - 1;
  uword exponent =
      (num_digits - 1) * kBitsPerWord + high_bit - 1 + exponent_bias;

  // Extract mantissa bits including the high bit which is implicit in the
  // float representation and one extra bit to help determine if we need to
  // round up.
  // We also keep track if the bits shifted out on the right side are zero.
  int shift = high_bit - (kDoubleMantissaBits + 2);
  int shift_right = Utils::maximum(shift, 0);
  int shift_left = -Utils::minimum(shift, 0);
  uword value_as_word = (high_digit >> shift_right) << shift_left;
  bool lesser_significant_bits_zero;
  if (shift_left > 0) {
    int lower_shift_right = kBitsPerWord - shift_left;
    value_as_word |= second_highest_digit >> lower_shift_right;
    lesser_significant_bits_zero = (second_highest_digit << shift_left) == 0;
  } else {
    lesser_significant_bits_zero =
        second_highest_digit == 0 &&
        (shift_right == 0 || (high_digit << (kBitsPerWord - shift_right)) == 0);
  }

  // Returns true if all digits (in the numbers magnitude) below the 2 highest
  // digits are zero.
  auto lower_bits_zero = [&]() -> bool {
    if (!lesser_significant_bits_zero) return false;
    // Already scanned the digits in the negative case and can look at carry.
    if (is_negative) return carry_to_second_highest != 0;
    for (word i = num_digits - 3; i >= 0; i--) {
      if (large_int.digitAt(i) != 0) return false;
    }
    return true;
  };

  // We need to round down if the least significant bit is zero, we need to
  // round up if the least significant and any other bit is one. If the
  // least significant bit is one and all other bits are zero then we look at
  // second least significant bit to round towards an even number.
  if ((value_as_word & 0x3) == 0x3 ||
      ((value_as_word & 1) && !lower_bits_zero())) {
    value_as_word++;
    // This may have triggered an overflow, so we need to add 1 to the exponent.
    if (value_as_word == (uword{1} << (kDoubleMantissaBits + 2))) {
      exponent++;
    }
    if (rounding != nullptr) {
      *rounding = RoundUp;
    }
  } else if (rounding != nullptr) {
    *rounding =
        !(value_as_word & 1) && lower_bits_zero() ? NoRounding : RoundDown;
  }
  value_as_word >>= 1;

  // Check for overflow.
  // The biggest exponent is used to mark special numbers like NAN or INF.
  uword max_exponent = (1 << exponent_bits) - 1;
  if (exponent > max_exponent - 1) {
    return false;
  }

  // Mask out implicit bit, combine mantissa, exponent and sign.
  value_as_word &= (uword{1} << kDoubleMantissaBits) - 1;
  value_as_word |= exponent << kDoubleMantissaBits;
  value_as_word |= uword{is_negative} << (kDoubleMantissaBits + exponent_bits);
  *result = bit_cast<double>(value_as_word);
  return true;
}

RawObject convertIntToDouble(Thread* thread, const Int& value, double* result) {
  if (value.numDigits() == 1) {
    *result = static_cast<double>(value.asWord());
    return NoneType::object();
  }
  HandleScope scope(thread);
  LargeInt large_int(&scope, *value);
  if (!convertLargeIntToDouble(large_int, result, nullptr)) {
    return thread->raiseWithFmt(LayoutId::kOverflowError,
                                "int too large to convert to float");
  }
  return NoneType::object();
}

bool compareDoubleWithInt(Thread* thread, double left, const Int& right,
                          CompareOp op) {
  DCHECK(op == GE || op == GT || op == LE || op == LT, "needs inequality op");
  bool compare_equal = op == LE || op == GE;
  bool compare_less = op == LT || op == LE;
  bool compare_greater = !compare_less;
  if (!std::isfinite(left)) {
    if (std::isnan(left)) return false;
    DCHECK(std::isinf(left), "remaining case must be infinity");
    return compare_less == (left < 0);
  }

  word num_digits = right.numDigits();
  if (num_digits == 1) {
    word right_word = right.asWord();
    double right_double = static_cast<double>(right_word);
    if (left < right_double) return compare_less;
    if (left > right_double) return compare_greater;
    // TODO(matthiasb): We could also detect the rounding direction by
    // performing bit operations on `right_word` which is more complicated but
    // may be faster; benchmark.
    word right_double_word = static_cast<word>(right_double);
    if (right_double_word == right_word) return compare_equal;
    return compare_less == (right_double_word < right_word);
  }

  // Shortcut for differing signs.
  if ((left < 0) != right.isNegative()) {
    DCHECK((compare_less == (left < 0)) == (compare_greater == (left > 0)),
           "conditions must be exclusive");
    return compare_less == (left < 0);
  }

  double right_double;
  RoundingDirection rounding;
  HandleScope scope(thread);
  LargeInt large_int(&scope, *right);
  if (!convertLargeIntToDouble(large_int, &right_double, &rounding)) {
    return compare_less != (left < 0);
  }
  if (left < right_double) return compare_less;
  if (left > right_double) return compare_greater;
  if (rounding == NoRounding) return compare_equal;
  return compare_less == (rounding == RoundDown);
}

bool doubleEqualsInt(Thread* thread, double left, const Int& right) {
  // This is basically the same code as `doubleCompareWithInt` but can take some
  // shortcuts because we don't care about the lesser/greater situations.
  word num_digits = right.numDigits();
  if (num_digits == 1) {
    word right_word = right.asWord();
    double right_double = static_cast<double>(right_word);
    if (left != right_double) return false;
    // Check whether any rounding occured when converting to floating-point.
    // TODO(matthiasb): We can also check this via bit operations on
    // `right_word` which is more complicated but may be faster; should run
    // some benchmarks.
    return static_cast<word>(right_double) == right_word;
  }

  if (!std::isfinite(left)) {
    return false;
  }
  double right_double;
  RoundingDirection rounding;
  HandleScope scope(thread);
  LargeInt large_int(&scope, *right);
  if (!convertLargeIntToDouble(large_int, &right_double, &rounding)) {
    return false;
  }
  return rounding == NoRounding && left == right_double;
}

RawObject intFromIndex(Thread* thread, const Object& obj) {
  Runtime* runtime = thread->runtime();
  if (runtime->isInstanceOfInt(*obj)) {
    return *obj;
  }
  HandleScope scope(thread);
  Object result(&scope, thread->invokeMethod1(obj, SymbolId::kDunderIndex));
  if (result.isError()) {
    if (result.isErrorNotFound()) {
      return thread->raiseWithFmt(
          LayoutId::kTypeError,
          "'%T' object cannot be interpreted as an integer", &obj);
    }
    return *result;
  }
  if (runtime->isInstanceOfInt(*result)) {
    return *result;
  }
  return thread->raiseWithFmt(LayoutId::kTypeError,
                              "__index__ returned non-int (type %T)", &result);
}

RawObject intUnderlying(Thread* thread, const Object& obj) {
  DCHECK(thread->runtime()->isInstanceOfInt(*obj),
         "cannot get a base int value from a non-int");
  if (obj.isInt()) return *obj;
  HandleScope scope(thread);
  UserIntBase user_int(&scope, *obj);
  return user_int.value();
}

}  // namespace python
