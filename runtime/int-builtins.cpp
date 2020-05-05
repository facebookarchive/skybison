#include "int-builtins.h"

#include <cinttypes>
#include <climits>
#include <cmath>

#include "builtins.h"
#include "bytes-builtins.h"
#include "formatter.h"
#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "str-builtins.h"
#include "thread.h"

namespace py {

void IntBuiltins::postInitialize(Runtime*, const Type& new_type) {
  new_type.setBuiltinBase(LayoutId::kInt);
}

void SmallIntBuiltins::postInitialize(Runtime* runtime, const Type& new_type) {
  runtime->setSmallIntType(new_type);
  RawObject layout = new_type.instanceLayout();
  Layout::cast(layout).setDescribedType(runtime->typeAt(kSuperType));
  // We want to lookup the class of an immediate type by using the 5-bit tag
  // value as an index into the class table.  Replicate the layout object for
  // SmallInt to all locations that decode to a SmallInt tag.
  for (word i = 2; i < (1 << Object::kImmediateTagBits); i += 2) {
    DCHECK(runtime->layoutAt(static_cast<LayoutId>(i)) == NoneType::object(),
           "list collision");
    runtime->layoutAtPut(static_cast<LayoutId>(i), layout);
  }
}

void LargeIntBuiltins::postInitialize(Runtime* runtime, const Type& new_type) {
  new_type.setBuiltinBase(kSuperType);
  runtime->setLargeIntType(new_type);
  Layout::cast(new_type.instanceLayout())
      .setDescribedType(runtime->typeAt(kSuperType));
}

word largeIntHash(RawLargeInt value) {
  const word bits_per_half = kBitsPerWord / 2;

  // The following computes `value % modulus` with
  // `modulus := kArithmeticHashModulus` with
  // a C/C++ style modulo (so -17 % m == -17). This matches cpythons hash
  // function (see `cpython/Objects/longobject.c` for details).

  // The following describes how we can compute without actually performing
  // any division/modulo operations just by bit-shifting. We want to compute
  // the modulo by a prime number for good hashing behavior and we pick a
  // Mersenne prime, because that allows us to perform some masking tricks
  // below. We use the constants as follows:
  //    hash_bits := kArithmeticHashBits := 61
  //    modulus := kArithmeticHashModulus := (1 << hash_bits) - 1
  //
  // To compute the modulo of a large int, we split it into higher bits and
  // lower bits:
  //    large_int % modulus = ((high << s) + remaining_bits) % modulus
  //     = (((high << s) % modulus) + remaining_bits % modulus) % modulus
  //
  // It turns out the upper part part of this equation
  // `((high << s) % modulus)` is just a bit rotation of `high_bits`.
  // To understand this consider splitting up a value into `high_bits` and
  // `low_bits`:
  //    low_bits := val & modulus
  //    high_bits := ((val >> hash_bits) << hash_bits)
  //    val = high_bits + low_bits
  //
  // <=> val << s = (((val << s) >> hash_bits) << hash_bits)
  //                + (val << s) & modulus
  //              = ((val >> (hash_bits - s)) << hash_bits)
  //                + (val << s) & modulus
  // <=> (val << s) % modulus
  //              = (((val >> (hash_bits - s)) << hash_bits) % modulus +
  //                 ((val << s) & modulus) % modulus) % modulus
  //              = (((val >> (hash_bits - s)) * 2**hash_bits) % modulus +
  //                 ((val << s) & modulus) % modulus
  //              = (((val << (hash_bits - s)) % modulus * 1) % modulus +
  //                 ((val << s) & modulus) % modulus
  //              = (((val << (hash_bits - s)) % modulus +
  //                 ((val << s) & modulus) % modulus
  //              = ((val << (hash_bits - s)) + ((val << s) & modulus))
  //                 % modulus
  // Which is a rotation of `s` bits in the lowest `hash_bits` in `val`.
  //
  // Note that we can choose any size `s` that is smaller than `hash_bits`,
  // meaning we can design our algorithm to compute `s` bits at a time.
  //
  // We only add a small amount of bits in each step, so the remaining modulo
  // operation can be expressed as `if (result >= modulus) result -= modulus;`.
  bool is_negative = value.isNegative();
  word num_digits = value.numDigits();

  uword result = 0;
  for (word i = num_digits - 1; i >= 0; i--) {
    uword digit = value.digitAt(i);
    // The computation is designed for positive numbers. We compute negative
    // numbers via `-(-value % p)`. We use the following equivalence so we do
    // not need to negate the large integer:
    //      -(-value % p)
    //  <=> -((~value + 1) % p)
    //  <=> -(((~value % p) + (1 % p)) % p)
    //  <=> -(((~value % p) + 1) % p)
    if (is_negative) {
      digit = ~digit;
    }

    // Rotate result, add upper half of the digit, perform modulo.
    result = ((result << bits_per_half) & kArithmeticHashModulus) |
             result >> (kArithmeticHashBits - bits_per_half);
    result += digit >> bits_per_half;
    if (result >= kArithmeticHashModulus) {
      result -= kArithmeticHashModulus;
    }

    // Rotate result, add lower half of digit, perform modulo.
    result = ((result << bits_per_half) & kArithmeticHashModulus) |
             result >> (kArithmeticHashBits - bits_per_half);
    uword low_bits = digit & ((uword{1} << bits_per_half) - 1);
    result += low_bits;
    if (result >= kArithmeticHashModulus) {
      result -= kArithmeticHashModulus;
    }
  }

  if (is_negative) {
    // We computed `result := ~value % p` so far, as described above compute
    // `-((result + 1) % p)` now.
    result++;
    if (result >= kArithmeticHashModulus) {
      result -= kArithmeticHashModulus;
    }
    result = -result;
    // cpython replaces `-1` results with -2, because -1 is used as an
    // "uninitialized hash" marker in some situations. We do not use the same
    // marker, but do the same to match behavior.
    if (result == static_cast<uword>(word{-1})) {
      result--;
    }
  } else {
    DCHECK(result != static_cast<uword>(word{-1}),
           "should only have -1 for negative numbers");
  }
  return result;
}

// Used only for UserIntBase as a heap-allocated object.
const BuiltinAttribute IntBuiltins::kAttributes[] = {
    {ID(_UserInt__value), UserIntBase::kValueOffset, AttributeFlags::kHidden},
    {SymbolId::kSentinelId, 0},
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
    return thread->raiseRequiresType(self_obj, ID(int));
  }
  if (!runtime->isInstanceOfInt(*other_obj)) {
    return NotImplementedType::object();
  }
  Int self(&scope, intUnderlying(*self_obj));
  Int other(&scope, intUnderlying(*other_obj));
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
    return thread->raiseRequiresType(self_obj, ID(int));
  }
  Int self(&scope, intUnderlying(*self_obj));
  return op(thread, self);
}

static RawObject asInt(Thread* thread, Frame* frame, word nargs) {
  return intUnaryOp(thread, frame, nargs,
                    [](Thread*, const Int& self) -> RawObject {
                      if (self.isBool()) {
                        return convertBoolToInt(*self);
                      }
                      return *self;
                    });
}

static RawObject asStr(Thread* thread, Frame* frame, word nargs) {
  return intUnaryOp(thread, frame, nargs, [](Thread* t, const Int& self) {
    return formatIntDecimalSimple(t, self);
  });
}

RawObject METH(int, __abs__)(Thread* thread, Frame* frame, word nargs) {
  return intUnaryOp(thread, frame, nargs,
                    [](Thread* t, const Int& self) -> RawObject {
                      if (self.isNegative()) {
                        return t->runtime()->intNegate(t, self);
                      }
                      if (self.isBool()) return convertBoolToInt(*self);
                      return *self;
                    });
}

RawObject METH(int, __add__)(Thread* thread, Frame* frame, word nargs) {
  return intBinaryOp(thread, frame, nargs,
                     [](Thread* t, const Int& left, const Int& right) {
                       return t->runtime()->intAdd(t, left, right);
                     });
}

RawObject METH(int, __and__)(Thread* thread, Frame* frame, word nargs) {
  return intBinaryOp(thread, frame, nargs,
                     [](Thread* t, const Int& left, const Int& right) {
                       return t->runtime()->intBinaryAnd(t, left, right);
                     });
}

RawObject METH(int, __bool__)(Thread* thread, Frame* frame, word nargs) {
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

RawObject METH(int, __ceil__)(Thread* thread, Frame* frame, word nargs) {
  return asInt(thread, frame, nargs);
}

RawObject METH(int, __eq__)(Thread* thread, Frame* frame, word nargs) {
  return intBinaryOp(
      thread, frame, nargs,
      [](Thread*, const Int& left, const Int& right) -> RawObject {
        return Bool::fromBool(left.compare(*right) == 0);
      });
}

RawObject METH(int, __divmod__)(Thread* thread, Frame* frame, word nargs) {
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
        return runtime->newTupleWith2(quotient, remainder);
      });
}

RawObject METH(int, __float__)(Thread* thread, Frame* frame, word nargs) {
  return intUnaryOp(thread, frame, nargs, [](Thread* t, const Int& self) {
    HandleScope scope(t);
    double value = 0.0;
    Object maybe_error(&scope, convertIntToDouble(t, self, &value));
    if (!maybe_error.isNoneType()) return *maybe_error;
    return t->runtime()->newFloat(value);
  });
}

RawObject METH(int, __floor__)(Thread* thread, Frame* frame, word nargs) {
  return asInt(thread, frame, nargs);
}

RawObject METH(int, __invert__)(Thread* thread, Frame* frame, word nargs) {
  return intUnaryOp(thread, frame, nargs, [](Thread* t, const Int& self) {
    return t->runtime()->intInvert(t, self);
  });
}

RawObject METH(int, __floordiv__)(Thread* thread, Frame* frame, word nargs) {
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

RawObject METH(int, __format__)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfInt(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(int));
  }
  Int self(&scope, intUnderlying(*self_obj));

  Object spec_obj(&scope, args.get(1));
  if (!runtime->isInstanceOfStr(*spec_obj)) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "__format__() argument 1 must be str, not %T",
                                &spec_obj);
  }
  Str spec(&scope, strUnderlying(*spec_obj));

  if (spec.length() == 0) {
    // We return the equivalent of `str(self)` for an empty spec.
    if (self_obj.isSmallInt() || self_obj.isLargeInt()) {
      return formatIntDecimalSimple(thread, self);
    }
    if (self_obj.isBool()) {
      return runtime->symbols()->at(Bool::cast(*self_obj).value() ? ID(True)
                                                                  : ID(False));
    }
    Object value(&scope, thread->invokeMethod1(self_obj, ID(__str__)));
    DCHECK(!value.isErrorNotFound(), "`__str__` should always exist");
    if (value.isErrorException()) return *value;
    if (!runtime->isInstanceOfStr(*value)) {
      return thread->raiseWithFmt(LayoutId::kTypeError,
                                  "__str__ returned non-string (type %T)",
                                  &value);
    }
    return *value;
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

RawObject METH(int, __hash__)(Thread* thread, Frame* frame, word nargs) {
  return intUnaryOp(thread, frame, nargs,
                    [](Thread*, const Int& self) -> RawObject {
                      return SmallInt::fromWord(intHash(*self));
                    });
}

RawObject METH(int, __index__)(Thread* thread, Frame* frame, word nargs) {
  return asInt(thread, frame, nargs);
}

RawObject METH(int, __int__)(Thread* thread, Frame* frame, word nargs) {
  return asInt(thread, frame, nargs);
}

RawObject METH(int, __le__)(Thread* thread, Frame* frame, word nargs) {
  return intBinaryOp(
      thread, frame, nargs,
      [](Thread*, const Int& left, const Int& right) -> RawObject {
        return Bool::fromBool(left.compare(*right) <= 0);
      });
}

RawObject METH(int, __lt__)(Thread* thread, Frame* frame, word nargs) {
  return intBinaryOp(
      thread, frame, nargs,
      [](Thread*, const Int& left, const Int& right) -> RawObject {
        return Bool::fromBool(left.compare(*right) < 0);
      });
}

RawObject METH(int, __ge__)(Thread* thread, Frame* frame, word nargs) {
  return intBinaryOp(
      thread, frame, nargs,
      [](Thread*, const Int& left, const Int& right) -> RawObject {
        return Bool::fromBool(left.compare(*right) >= 0);
      });
}

RawObject METH(int, __gt__)(Thread* thread, Frame* frame, word nargs) {
  return intBinaryOp(
      thread, frame, nargs,
      [](Thread*, const Int& left, const Int& right) -> RawObject {
        return Bool::fromBool(left.compare(*right) > 0);
      });
}

RawObject METH(int, __mod__)(Thread* thread, Frame* frame, word nargs) {
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

RawObject METH(int, __mul__)(Thread* thread, Frame* frame, word nargs) {
  return intBinaryOp(thread, frame, nargs,
                     [](Thread* t, const Int& left, const Int& right) {
                       return t->runtime()->intMultiply(t, left, right);
                     });
}

RawObject METH(int, __ne__)(Thread* thread, Frame* frame, word nargs) {
  return intBinaryOp(
      thread, frame, nargs,
      [](Thread*, const Int& left, const Int& right) -> RawObject {
        return Bool::fromBool(left.compare(*right) != 0);
      });
}

RawObject METH(int, __neg__)(Thread* thread, Frame* frame, word nargs) {
  return intUnaryOp(thread, frame, nargs, [](Thread* t, const Int& self) {
    return t->runtime()->intNegate(t, self);
  });
}

RawObject METH(int, __rshift__)(Thread* thread, Frame* frame, word nargs) {
  return intBinaryOp(
      thread, frame, nargs, [](Thread* t, const Int& left, const Int& right) {
        if (right.isNegative()) {
          return t->raiseWithFmt(LayoutId::kValueError, "negative shift count");
        }
        return t->runtime()->intBinaryRshift(t, left, right);
      });
}

RawObject METH(int, __str__)(Thread* thread, Frame* frame, word nargs) {
  return asStr(thread, frame, nargs);
}

RawObject METH(int, __sub__)(Thread* thread, Frame* frame, word nargs) {
  return intBinaryOp(thread, frame, nargs,
                     [](Thread* t, const Int& left, const Int& right) {
                       return t->runtime()->intSubtract(t, left, right);
                     });
}

RawObject METH(int, __truediv__)(Thread* thread, Frame* frame, word nargs) {
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

RawObject METH(int, __trunc__)(Thread* thread, Frame* frame, word nargs) {
  return asInt(thread, frame, nargs);
}

RawObject METH(int, __xor__)(Thread* thread, Frame* frame, word nargs) {
  return intBinaryOp(thread, frame, nargs,
                     [](Thread* t, const Int& left, const Int& right) {
                       return t->runtime()->intBinaryXor(t, left, right);
                     });
}

RawObject METH(int, __or__)(Thread* thread, Frame* frame, word nargs) {
  return intBinaryOp(thread, frame, nargs,
                     [](Thread* t, const Int& left, const Int& right) {
                       return t->runtime()->intBinaryOr(t, left, right);
                     });
}

RawObject METH(int, __lshift__)(Thread* thread, Frame* frame, word nargs) {
  return intBinaryOp(
      thread, frame, nargs, [](Thread* t, const Int& left, const Int& right) {
        if (right.isNegative()) {
          return t->raiseWithFmt(LayoutId::kValueError, "negative shift count");
        }
        return t->runtime()->intBinaryLshift(t, left, right);
      });
}

RawObject METH(int, __pos__)(Thread* thread, Frame* frame, word nargs) {
  return asInt(thread, frame, nargs);
}

RawObject METH(int, __repr__)(Thread* thread, Frame* frame, word nargs) {
  return asStr(thread, frame, nargs);
}

RawObject METH(int, __round__)(Thread* thread, Frame* frame, word nargs) {
  return asInt(thread, frame, nargs);
}

RawObject METH(int, bit_length)(Thread* thread, Frame* frame, word nargs) {
  return intUnaryOp(thread, frame, nargs, [](Thread* t, const Int& self) {
    return t->runtime()->newInt(self.bitLength());
  });
}

RawObject METH(int, conjugate)(Thread* thread, Frame* frame, word nargs) {
  return asInt(thread, frame, nargs);
}

static RawObject toBytesImpl(Thread* thread, const Object& self_obj,
                             const Object& length_obj,
                             const Object& byteorder_obj, bool is_signed) {
  HandleScope scope;
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfInt(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(int));
  }
  Int self(&scope, intUnderlying(*self_obj));
  if (!runtime->isInstanceOfInt(*length_obj)) {
    return thread->raiseWithFmt(
        LayoutId::kTypeError,
        "length argument cannot be interpreted as an integer");
  }
  Int length_int(&scope, intUnderlying(*length_obj));
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
  if (byteorder.equals(runtime->symbols()->at(ID(little)))) {
    endianness = endian::little;
  } else if (byteorder.equals(runtime->symbols()->at(ID(big)))) {
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

RawObject METH(int, to_bytes)(Thread* thread, Frame* frame, word nargs) {
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

RawObject METH(bool, __new__)(Thread* thread, Frame* frame, word nargs) {
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

static RawObject boolOrImpl(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  if (!self_obj.isBool()) {
    return thread->raiseRequiresType(self_obj, ID(bool));
  }
  Bool self(&scope, *self_obj);
  Object other_obj(&scope, args.get(1));
  if (other_obj.isBool()) {
    return Bool::fromBool(self.value() || Bool::cast(*other_obj).value());
  }
  if (runtime->isInstanceOfInt(*other_obj)) {
    return intBinaryOp(thread, frame, nargs,
                       [](Thread* t, const Int& left, const Int& right) {
                         return t->runtime()->intBinaryOr(t, left, right);
                       });
  }
  return NotImplementedType::object();
}

RawObject METH(bool, __or__)(Thread* thread, Frame* frame, word nargs) {
  return boolOrImpl(thread, frame, nargs);
}

RawObject METH(bool, __ror__)(Thread* thread, Frame* frame, word nargs) {
  return boolOrImpl(thread, frame, nargs);
}

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
  Object result(&scope, thread->invokeMethod1(obj, ID(__index__)));
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

}  // namespace py
