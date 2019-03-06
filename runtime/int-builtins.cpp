#include "int-builtins.h"

#include <cerrno>
#include <cinttypes>
#include <climits>
#include <cmath>

#include "bytes-builtins.h"
#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "thread.h"
#include "trampolines-inl.h"

namespace python {

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
    {SymbolId::kDunderNew, dunderNew},
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

void IntBuiltins::postInitialize(Runtime* runtime, const Type& new_type) {
  new_type.setBuiltinBase(LayoutId::kInt);
  runtime->typeAddNativeFunctionKw(new_type, SymbolId::kFromBytes,
                                   nativeTrampoline<fromBytes>,
                                   nativeTrampolineKw<fromBytesKw>);
}

RawObject IntBuiltins::asInt(const Int& value) {
  if (value.isBool()) {
    return RawSmallInt::fromWord(RawBool::cast(*value)->value() ? 1 : 0);
  }
  return *value;
}

RawObject IntBuiltins::dunderNew(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  Arguments args(frame, nargs);
  HandleScope scope(thread);

  Object type_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfType(*type_obj)) {
    return thread->raiseTypeErrorWithCStr(
        "int.__new__(X): X is not a type object");
  }

  Type type(&scope, *type_obj);
  if (type.builtinBase() != LayoutId::kInt) {
    return thread->raiseTypeErrorWithCStr(
        "int.__new__(X): X is not a subtype of int");
  }

  Layout layout(&scope, type.instanceLayout());
  if (layout.id() != LayoutId::kInt) {
    // TODO(dulinr): Implement __new__ with subtypes of int.
    UNIMPLEMENTED("int.__new__(<subtype of int>, ...)");
  }

  Object arg(&scope, args.get(1));
  if (!arg.isStr()) {
    // TODO(dulinr): Handle non-string types.
    UNIMPLEMENTED("int(<non-string>)");
  }

  // No base argument, use 10 as the base.
  if (args.get(2).isUnboundValue()) {
    return intFromString(thread, *arg, 10);
  }

  // The third argument is the base of the integer represented in the string.
  Object base(&scope, args.get(2));
  if (!base.isInt()) {
    // TODO(dulinr): Call __index__ on base to convert it.
    UNIMPLEMENTED("Can't handle non-integer base");
  }
  if (runtime->isInstanceOfBytes(*arg)) {
    // TODO(T41277914): Int from bytes
    UNIMPLEMENTED("int.__new__(bytes)");
  }
  if (runtime->isInstanceOfByteArray(*arg)) {
    // TODO(T41277959): Int from bytearray
    UNIMPLEMENTED("int.__new__(bytearray)");
  }
  return intFromString(thread, *arg, RawInt::cast(*base)->asWord());
}

RawObject IntBuiltins::intFromString(Thread* thread, RawObject arg_raw,
                                     int base) {
  if (!(base == 0 || (base >= 2 && base <= 36))) {
    return thread->raiseValueErrorWithCStr(
        "Invalid base, must be between 2 and 36, or 0");
  }
  HandleScope scope(thread);
  Object arg(&scope, arg_raw);
  if (arg.isInt()) {
    return *arg;
  }

  CHECK(arg.isStr(), "not string type");
  Str s(&scope, *arg);
  if (s.length() == 0) {
    return thread->raiseValueErrorWithCStr("invalid literal");
  }
  char* c_str = s.toCStr();  // for strtol()
  char* end_ptr;
  errno = 0;
  long res = std::strtol(c_str, &end_ptr, base);
  int saved_errno = errno;
  bool is_complete = (*end_ptr == '\0');
  free(c_str);
  if (!is_complete || (res == 0 && saved_errno == EINVAL)) {
    return thread->raiseValueErrorWithCStr("invalid literal");
  }
  if ((res == LONG_MAX || res == LONG_MIN) && saved_errno == ERANGE) {
    return thread->raiseValueErrorWithCStr("invalid literal (range)");
  }
  if (!SmallInt::isValid(res)) {
    return thread->raiseValueErrorWithCStr("unsupported type");
  }
  return SmallInt::fromWord(res);
}

static RawObject raiseRequiresInt(Thread* thread, Frame* frame) {
  HandleScope scope(thread);
  Function function(&scope, frame->function());
  Object message(&scope, NoneType::object());
  Runtime* runtime = thread->runtime();
  Str function_name(&scope, function.name());
  unique_c_ptr<char> function_name_cstr(function_name.toCStr());
  message = runtime->newStrFromFormat("'%s' requires a 'int' object",
                                      function_name_cstr.get());
  return thread->raiseTypeError(*message);
}

static RawObject intBinaryOp(Thread* thread, Frame* frame, word nargs,
                             RawObject (*op)(Thread* thread, const Int& left,
                                             const Int& right)) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfInt(*self_obj)) {
    return raiseRequiresInt(thread, frame);
  }
  Object other_obj(&scope, args.get(1));
  if (!runtime->isInstanceOfInt(*other_obj)) {
    return runtime->notImplemented();
  }
  Int self(&scope, *self_obj);
  Int other(&scope, *other_obj);
  return op(thread, self, other);
}

static RawObject intUnaryOp(Thread* thread, Frame* frame, word nargs,
                            RawObject (*op)(Thread* thread, const Int& self)) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfInt(*self_obj)) {
    return raiseRequiresInt(thread, frame);
  }
  Int self(&scope, *self_obj);
  return op(thread, self);
}

RawObject IntBuiltins::dunderInt(Thread* thread, Frame* frame, word nargs) {
  return intUnaryOp(thread, frame, nargs,
                    [](Thread*, const Int& self) { return asInt(self); });
}

void SmallIntBuiltins::postInitialize(Runtime* runtime, const Type& new_type) {
  new_type.setBuiltinBase(kSuperType);
  // We want to lookup the class of an immediate type by using the 5-bit tag
  // value as an index into the class table.  Replicate the class object for
  // SmallInt to all locations that decode to a SmallInt tag.
  for (word i = 1; i < 16; i++) {
    DCHECK(
        runtime->layoutAt(static_cast<LayoutId>(i << 1)) == NoneType::object(),
        "list collision");
    runtime->layoutAtPut(static_cast<LayoutId>(i << 1), *new_type);
  }
}

RawObject IntBuiltins::bitLength(Thread* t, Frame* frame, word nargs) {
  return intUnaryOp(t, frame, nargs, [](Thread* thread, const Int& self) {
    return thread->runtime()->newInt(self.bitLength());
  });
}

RawObject IntBuiltins::dunderAbs(Thread* t, Frame* frame, word nargs) {
  return intUnaryOp(t, frame, nargs, [](Thread* thread, const Int& self) {
    return self.isNegative() ? thread->runtime()->intNegate(thread, self)
                             : asInt(self);
  });
}

RawObject IntBuiltins::dunderAdd(Thread* t, Frame* frame, word nargs) {
  return intBinaryOp(t, frame, nargs,
                     [](Thread* thread, const Int& left, const Int& right) {
                       return thread->runtime()->intAdd(thread, left, right);
                     });
}

RawObject IntBuiltins::dunderAnd(Thread* t, Frame* frame, word nargs) {
  return intBinaryOp(
      t, frame, nargs, [](Thread* thread, const Int& left, const Int& right) {
        return thread->runtime()->intBinaryAnd(thread, left, right);
      });
}

RawObject IntBuiltins::dunderBool(Thread* t, Frame* frame, word nargs) {
  return intUnaryOp(t, frame, nargs, [](Thread*, const Int& self) -> RawObject {
    if (self.isBool()) return *self;
    if (self.isSmallInt()) {
      return Bool::fromBool(SmallInt::cast(*self)->value() != 0);
    }
    DCHECK(self.isLargeInt(), "remaining case should be LargeInt");
    return Bool::trueObj();
  });
}

RawObject IntBuiltins::dunderEq(Thread* t, Frame* frame, word nargs) {
  return intBinaryOp(
      t, frame, nargs,
      [](Thread*, const Int& left, const Int& right) -> RawObject {
        return Bool::fromBool(left.compare(*right) == 0);
      });
}

RawObject IntBuiltins::dunderDivmod(Thread* t, Frame* frame, word nargs) {
  return intBinaryOp(
      t, frame, nargs,
      [](Thread* thread, const Int& left, const Int& right) -> RawObject {
        HandleScope scope(thread);
        Object quotient(&scope, NoneType::object());
        Object remainder(&scope, NoneType::object());
        Runtime* runtime = thread->runtime();
        if (!runtime->intDivideModulo(thread, left, right, &quotient,
                                      &remainder)) {
          return thread->raiseZeroDivisionErrorWithCStr(
              "integer division or modulo by zero");
        }
        Tuple result(&scope, runtime->newTuple(2));
        result.atPut(0, *quotient);
        result.atPut(1, *remainder);
        return *result;
      });
}

RawObject IntBuiltins::dunderFloat(Thread* t, Frame* frame, word nargs) {
  return intUnaryOp(t, frame, nargs, [](Thread* thread, const Int& self) {
    HandleScope scope(thread);
    double value;
    Object maybe_error(&scope, convertIntToDouble(thread, self, &value));
    if (!maybe_error.isNoneType()) return *maybe_error;
    return thread->runtime()->newFloat(value);
  });
}

RawObject IntBuiltins::dunderInvert(Thread* t, Frame* frame, word nargs) {
  return intUnaryOp(t, frame, nargs, [](Thread* thread, const Int& self) {
    return thread->runtime()->intInvert(thread, self);
  });
}

RawObject IntBuiltins::dunderFloordiv(Thread* t, Frame* frame, word nargs) {
  return intBinaryOp(
      t, frame, nargs,
      [](Thread* thread, const Int& left, const Int& right) -> RawObject {
        HandleScope scope(thread);
        Object quotient(&scope, NoneType::object());
        if (!thread->runtime()->intDivideModulo(thread, left, right, &quotient,
                                                nullptr)) {
          return thread->raiseZeroDivisionErrorWithCStr(
              "integer division or modulo by zero");
        }
        return *quotient;
      });
}

RawObject IntBuiltins::dunderLe(Thread* t, Frame* frame, word nargs) {
  return intBinaryOp(
      t, frame, nargs,
      [](Thread*, const Int& left, const Int& right) -> RawObject {
        return Bool::fromBool(left.compare(*right) <= 0);
      });
}

static RawObject toBytesImpl(Thread* thread, Frame* frame,
                             const Object& self_obj, const Object& length_obj,
                             const Object& byteorder_obj, bool is_signed) {
  HandleScope scope;
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfInt(*self_obj)) {
    return raiseRequiresInt(thread, frame);
  }
  Int self(&scope, *self_obj);

  if (!runtime->isInstanceOfInt(*length_obj)) {
    return thread->raiseTypeErrorWithCStr(
        "length argument cannot be interpreted as an integer");
  }
  Int length_int(&scope, *length_obj);
  OptInt<word> l = length_int.asInt<word>();
  if (l.error != CastError::None) {
    return thread->raiseOverflowErrorWithCStr(
        "Python int too large to convert to C word");
  }
  word length = l.value;
  if (length < 0) {
    return thread->raiseValueErrorWithCStr(
        "length argument must be non-negative");
  }

  if (!runtime->isInstanceOfStr(*byteorder_obj)) {
    return thread->raiseTypeErrorWithCStr(
        "to_bytes() argument 2 must be str, not int");
  }
  Str byteorder(&scope, *byteorder_obj);
  endian endianness;
  if (byteorder.equals(runtime->symbols()->Little())) {
    endianness = endian::little;
  } else if (byteorder.equals(runtime->symbols()->Big())) {
    endianness = endian::big;
  } else {
    return thread->raiseValueErrorWithCStr(
        "byteorder must be either 'little' or 'big'");
  }

  if (!is_signed && self.isNegative()) {
    return thread->raiseOverflowErrorWithCStr(
        "can't convert negative int to unsigned");
  }

  // Check for overflow.
  word num_digits = self.numDigits();
  uword high_digit = self.digitAt(num_digits - 1);
  word bit_length =
      num_digits * kBitsPerWord - Utils::numRedundantSignBits(high_digit);
  if (bit_length > length * kBitsPerByte + !is_signed) {
    return thread->raiseOverflowErrorWithCStr("int too big to convert");
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
    return thread->raiseTypeErrorWithCStr("signed must be bool");
  }
  return toBytesImpl(thread, frame, self, length, byteorder,
                     RawBool::cast(args.get(3)).value());
}

RawObject IntBuiltins::dunderTrueDiv(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  Arguments args(frame, nargs);
  RawObject self = args.get(0);
  RawObject other = args.get(1);
  if (!self->isSmallInt()) {
    return thread->raiseTypeErrorWithCStr(
        "__truediv__() must be called with int instance as first argument");
  }
  word left = RawSmallInt::cast(self)->value();
  if (other->isFloat()) {
    double right = RawFloat::cast(other)->value();
    if (right == 0.0) {
      return thread->raiseZeroDivisionErrorWithCStr("float division by zero");
    }
    return runtime->newFloat(left / right);
  }
  if (other->isBool()) {
    other = IntBuiltins::intFromBool(other);
  }
  if (other->isInt()) {
    word right = RawInt::cast(other)->asWord();
    if (right == 0) {
      return thread->raiseZeroDivisionErrorWithCStr("division by zero");
    }
    return runtime->newFloat(left / static_cast<double>(right));
  }
  return runtime->notImplemented();
}

RawObject IntBuiltins::dunderLt(Thread* t, Frame* frame, word nargs) {
  return intBinaryOp(
      t, frame, nargs,
      [](Thread*, const Int& left, const Int& right) -> RawObject {
        return Bool::fromBool(left.compare(*right) < 0);
      });
}

RawObject IntBuiltins::dunderGe(Thread* t, Frame* frame, word nargs) {
  return intBinaryOp(
      t, frame, nargs,
      [](Thread*, const Int& left, const Int& right) -> RawObject {
        return Bool::fromBool(left.compare(*right) >= 0);
      });
}

RawObject IntBuiltins::dunderGt(Thread* t, Frame* frame, word nargs) {
  return intBinaryOp(
      t, frame, nargs,
      [](Thread*, const Int& left, const Int& right) -> RawObject {
        return Bool::fromBool(left.compare(*right) > 0);
      });
}

RawObject IntBuiltins::dunderMod(Thread* t, Frame* frame, word nargs) {
  return intBinaryOp(
      t, frame, nargs,
      [](Thread* thread, const Int& left, const Int& right) -> RawObject {
        HandleScope scope(thread);
        Object remainder(&scope, NoneType::object());
        if (!thread->runtime()->intDivideModulo(thread, left, right, nullptr,
                                                &remainder)) {
          return thread->raiseZeroDivisionErrorWithCStr(
              "integer division or modulo by zero");
        }
        return *remainder;
      });
}

RawObject IntBuiltins::dunderMul(Thread* t, Frame* frame, word nargs) {
  return intBinaryOp(
      t, frame, nargs, [](Thread* thread, const Int& left, const Int& right) {
        return thread->runtime()->intMultiply(thread, left, right);
      });
}

RawObject IntBuiltins::dunderNe(Thread* t, Frame* frame, word nargs) {
  return intBinaryOp(
      t, frame, nargs,
      [](Thread*, const Int& left, const Int& right) -> RawObject {
        return Bool::fromBool(left.compare(*right) != 0);
      });
}

RawObject IntBuiltins::dunderNeg(Thread* t, Frame* frame, word nargs) {
  return intUnaryOp(t, frame, nargs, [](Thread* thread, const Int& self) {
    return thread->runtime()->intNegate(thread, self);
  });
}

RawObject IntBuiltins::dunderRshift(Thread* t, Frame* frame, word nargs) {
  return intBinaryOp(
      t, frame, nargs, [](Thread* thread, const Int& left, const Int& right) {
        if (right.isNegative()) {
          return thread->raiseValueErrorWithCStr("negative shift count");
        }
        return thread->runtime()->intBinaryRshift(thread, left, right);
      });
}

RawObject IntBuiltins::dunderSub(Thread* t, Frame* frame, word nargs) {
  return intBinaryOp(
      t, frame, nargs, [](Thread* thread, const Int& left, const Int& right) {
        return thread->runtime()->intSubtract(thread, left, right);
      });
}

RawObject IntBuiltins::dunderXor(Thread* t, Frame* frame, word nargs) {
  return intBinaryOp(
      t, frame, nargs, [](Thread* thread, const Int& left, const Int& right) {
        return thread->runtime()->intBinaryXor(thread, left, right);
      });
}

// TODO(T39167211): Merge with IntBuiltins::fromBytesKw / IntBuiltins::fromBytes
// once argument parsing is automated.
static RawObject fromBytesImpl(Thread* thread, const Object& bytes_obj,
                               const Object& byteorder_obj, bool is_signed) {
  HandleScope scope(thread);
  Object maybe_bytes(&scope, *bytes_obj);
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfBytes(*maybe_bytes)) {
    maybe_bytes = callDunderBytes(thread, bytes_obj);
    if (maybe_bytes.isNoneType()) {
      maybe_bytes = bytesFromIterable(thread, bytes_obj);
    }
    if (maybe_bytes.isError()) return *maybe_bytes;
  }
  Bytes bytes(&scope, *maybe_bytes);

  if (!runtime->isInstanceOfStr(*byteorder_obj)) {
    return thread->raiseTypeErrorWithCStr(
        "from_bytes() must be called with str instance as second argument");
  }
  Str byteorder(&scope, *byteorder_obj);
  endian endianness;
  if (byteorder.equals(runtime->symbols()->Little())) {
    endianness = endian::little;
  } else if (byteorder.equals(runtime->symbols()->Big())) {
    endianness = endian::big;
  } else {
    return thread->raiseValueErrorWithCStr(
        "from_bytes() byteorder argument must be 'little' or 'big'");
  }

  return runtime->bytesToInt(thread, bytes, endianness, is_signed);
}

RawObject IntBuiltins::fromBytes(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("expected 2 arguments");
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object bytes(&scope, args.get(0));
  Object byteorder(&scope, args.get(1));
  return fromBytesImpl(thread, bytes, byteorder, false);
}

RawObject IntBuiltins::fromBytesKw(Thread* thread, Frame* frame, word nargs) {
  KwArguments args(frame, nargs);
  if (args.numArgs() > 2) {
    return thread->raiseTypeError(thread->runtime()->newStrFromFormat(
        "from_bytes() takes at most 2 positional arguments (%ld given)",
        args.numArgs()));
  }

  HandleScope scope(thread);
  word num_known_keywords = 0;
  Runtime* runtime = thread->runtime();
  Object bytes(&scope, args.getKw(runtime->symbols()->Bytes()));
  if (args.numArgs() > 0) {
    if (!bytes.isError()) {
      return thread->raiseTypeErrorWithCStr(
          "argument for from_bytes() given by name ('bytes') and position (1)");
    }
    bytes = args.get(0);
  } else {
    if (bytes.isError()) {
      return thread->raiseTypeErrorWithCStr(
          "from_bytes() missing required argument 'bytes' (pos 1)");
    }
    ++num_known_keywords;
  }

  Object byteorder(&scope, args.getKw(runtime->symbols()->Byteorder()));
  if (args.numArgs() > 1) {
    if (!byteorder.isError()) {
      return thread->raiseTypeErrorWithCStr(
          "argument for from_bytes() given by name ('byteorder') and position "
          "(2)");
    }
    byteorder = args.get(1);
  } else {
    if (byteorder.isError()) {
      return thread->raiseTypeErrorWithCStr(
          "from_bytes() missing required argument 'byteorder' (pos 2)");
    }
    ++num_known_keywords;
  }

  bool is_signed = false;
  Object signed_arg(&scope, args.getKw(runtime->symbols()->Signed()));
  if (!signed_arg.isError()) {
    ++num_known_keywords;
    Object is_true(&scope, Interpreter::isTrue(thread, frame, signed_arg));
    if (is_true.isError()) return *is_true;
    is_signed = is_true == Bool::trueObj();
  }

  if (args.numKeywords() != num_known_keywords) {
    return thread->raiseTypeErrorWithCStr(
        "from_bytes() called with invalid keyword arguments");
  }

  return fromBytesImpl(thread, bytes, byteorder, is_signed);
}

inline RawObject IntBuiltins::intFromBool(RawObject bool_obj) {
  return SmallInt::fromWord(bool_obj == Bool::trueObj() ? 1 : 0);
}

RawObject IntBuiltins::dunderOr(Thread* t, Frame* frame, word nargs) {
  return intBinaryOp(
      t, frame, nargs, [](Thread* thread, const Int& left, const Int& right) {
        return thread->runtime()->intBinaryOr(thread, left, right);
      });
}

RawObject IntBuiltins::dunderLshift(Thread* t, Frame* frame, word nargs) {
  return intBinaryOp(
      t, frame, nargs, [](Thread* thread, const Int& left, const Int& right) {
        if (right.isNegative()) {
          return thread->raiseValueErrorWithCStr("negative shift count");
        }
        return thread->runtime()->intBinaryLshift(thread, left, right);
      });
}

// Returns the quotient of a double word number and a single word.
// Assumes the result will fit in a single uword: `dividend_high < divisor`.
static uword dwordUDiv(uword dividend_low, uword dividend_high, uword divisor,
                       uword* remainder) {
  // TODO(matthiasb): Future optimization idea:
  // This whole function is a single `divq` instruction on x86_64, we could use
  // inline assembly for it (there doesn't seem to be a builtin).

  // The code is based on Hacker's Delight chapter 9-4 Unsigned Long Division.
  DCHECK(divisor != 0, "division by zero");
  DCHECK(dividend_high < divisor, "overflow");

  // Performs some arithmetic with no more than half the bits of a `uword`.
  int half_bits = kBitsPerWord / 2;
  uword half_mask = (static_cast<uword>(1) << half_bits) - 1;

  // Normalize divisor by shifting the highest bit left as much as possible.
  static_assert(sizeof(divisor) == sizeof(long), "choose right builtin");
  int s = __builtin_clzl(divisor);
  uword divisor_n = divisor << s;
  uword divisor_n_high_half = divisor_n >> half_bits;
  uword divisor_n_low_half = divisor_n & half_mask;

  // Normalize dividend by shifting it by the same amount as the divisor.
  uword dividend_high_n =
      (s == 0) ? dividend_high
               : (dividend_high << s) | (dividend_low >> (kBitsPerWord - s));
  uword dividend_low_n = dividend_low << s;
  uword dividend_low_n_high_half = dividend_low_n >> half_bits;
  uword dividend_low_n_low_half = dividend_low_n & half_mask;

  uword quot_high_half = dividend_high_n / divisor_n_high_half;
  uword remainder_high_half = dividend_high_n % divisor_n_high_half;
  while (quot_high_half > half_mask ||
         quot_high_half * divisor_n_low_half >
             ((remainder_high_half << half_bits) | dividend_low_n_high_half)) {
    quot_high_half--;
    remainder_high_half += divisor_n_high_half;
    if (remainder_high_half > half_mask) break;
  }

  uword dividend_middle =
      ((dividend_high_n << half_bits) | dividend_low_n_high_half) -
      quot_high_half * divisor_n;

  uword quot_low_half = dividend_middle / divisor_n_high_half;
  uword remainder_low_half = dividend_middle % divisor_n_high_half;
  while (quot_low_half > half_mask ||
         quot_low_half * divisor_n_low_half >
             ((remainder_low_half << half_bits) | dividend_low_n_low_half)) {
    quot_low_half--;
    remainder_low_half += divisor_n_high_half;
    if (remainder_low_half > half_mask) break;
  }

  uword result = (quot_high_half << half_bits) | quot_low_half;
  *remainder = dividend_low - result * divisor;
  return result;
}

// Divide a large integer formed by an array of int digits by a single digit and
// return the remainder. `digits_in` and `digits_out` may be the same pointer
// for in-place operation.
static uword divIntSingleDigit(uword* digits_out, const uword* digits_in,
                               word num_digits, uword divisor) {
  // TODO(matthiasb): Future optimization idea:
  // Instead of dividing by a constant, multiply with a precomputed inverse
  // (see Hackers Delight, chapter 10). The compiler doesn't catch this case
  // for double word arithmetic as in dwordUDiv.
  uword remainder = 0;
  for (word i = num_digits - 1; i >= 0; i--) {
    // Compute `remainder:digit / divisor`.
    digits_out[i] = dwordUDiv(digits_in[i], remainder, divisor, &remainder);
  }
  return remainder;
}

// Converts an uword to ascii decimal digits. The digits can only be efficiently
// produced from least to most significant without knowing the exact number of
// digits upfront. Because of this the function takes a `buf_end` argument and
// writes the digit before it. Returns a pointer to the last byte written.
static byte* uwordToDecimal(uword num, byte* buf_end) {
  byte* start = buf_end;
  do {
    *--start = '0' + num % 10;
    num /= 10;
  } while (num > 0);
  return start;
}

RawObject IntBuiltins::dunderRepr(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfInt(*self_obj)) {
    return thread->raiseTypeErrorWithCStr("'__repr__' requires a 'int' object");
  }
  Int self(&scope, *self_obj);
  if (self.numDigits() == 1) {
    word value = self.asWord();
    uword magnitude = value >= 0 ? value : -static_cast<uword>(value);
    byte buffer[kUwordDigits10 + 1];
    byte* end = buffer + sizeof(buffer);
    byte* start = uwordToDecimal(magnitude, end);
    if (value < 0) *--start = '-';
    DCHECK(start >= buffer, "buffer underflow");
    return runtime->newStrWithAll(View<byte>(start, end - start));
  }
  LargeInt large_int(&scope, *self);

  // Allocate space for intermediate results. We also convert a negative number
  // to a positive number of the same magnitude here.
  word num_digits = large_int.numDigits();
  std::unique_ptr<uword[]> temp_digits(new uword[num_digits]);
  bool negative = large_int.isNegative();
  if (!negative) {
    for (word i = 0; i < num_digits; ++i) {
      temp_digits[i] = large_int.digitAt(i);
    }
  } else {
    uword carry = 1;
    for (word i = 0; i < num_digits; ++i) {
      uword digit = large_int.digitAt(i);
      carry = __builtin_uaddl_overflow(~digit, carry, &temp_digits[i]);
    }
    // The complement of the highest bit in a negative number must be 0 so we
    // cannot overflow.
    DCHECK(carry == 0, "overflow");
  }
  word num_temp_digits = num_digits;

  // Compute an upper bound on the number of decimal digits required for a
  // number with n bits:
  //   ceil(log10(2**n - 1))
  // We over-approximate this with:
  //   ceil(log10(2**n - 1))
  //   == ceil(log2(2**n - 1)/log2(10))
  //   <= 1 + n * (1/log2(10))
  //   <= 1 + n * 0.30102999566398114
  //   <= 1 + n * 309 / 1024
  // This isn't off by more than 1 digit for all one binary numbers up to
  // 1425 bits.
  word bit_length = large_int.bitLength();
  word max_chars = 1 + negative + bit_length * 309 / 1024;
  std::unique_ptr<byte[]> buffer(new byte[max_chars]);

  // The strategy here is to divide the large integer by continually dividing it
  // by `kUwordDigits10Pow`. `uwordToDecimal` can convert those remainders to
  // decimal digits.
  //
  // TODO(matthiasb): Future optimization ideas:
  // It seems cpythons algorithm is faster (for big numbers) in practive.
  // Their source claims it is (Knuth TAOCP, vol 2, section 4.4, method 1b).
  byte* end = buffer.get() + max_chars;
  byte* start = end;
  do {
    uword remainder = divIntSingleDigit(temp_digits.get(), temp_digits.get(),
                                        num_temp_digits, kUwordDigits10Pow);
    byte* new_start = uwordToDecimal(remainder, start);

    while (num_temp_digits > 0 && temp_digits[num_temp_digits - 1] == 0) {
      num_temp_digits--;
    }
    // Produce leading zeros if this wasn't the last round.
    if (num_temp_digits > 0) {
      for (word i = 0, n = kUwordDigits10 - (start - new_start); i < n; i++) {
        *--new_start = '0';
      }
    }
    start = new_start;
  } while (num_temp_digits > 0);

  if (negative) *--start = '-';

  DCHECK(start >= buffer.get(), "buffer underflow");
  return runtime->newStrWithAll(View<byte>(start, end - start));
}

RawObject BoolBuiltins::dunderNew(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object type_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfType(*type_obj)) {
    return thread->raiseTypeErrorWithCStr(
        "bool.__new__(X): X is not a type object");
  }
  Type type(&scope, *type_obj);

  // Since bool can't be subclassed, only need to check if the type is exactly
  // bool.
  Layout layout(&scope, type.instanceLayout());
  if (layout.id() != LayoutId::kBool) {
    return thread->raiseTypeErrorWithCStr("bool.__new__(X): X is not bool");
  }

  // If no arguments are given, return false.
  if (nargs == 1) {
    return Bool::falseObj();
  }

  Object arg(&scope, args.get(1));
  return Interpreter::isTrue(thread, frame, arg);
}

const BuiltinMethod BoolBuiltins::kBuiltinMethods[] = {
    {SymbolId::kDunderNew, dunderNew},
    {SymbolId::kSentinelId, nullptr},
};

RawObject asIntObject(Thread* thread, const Object& object) {
  if (object.isInt()) {
    return *object;
  }

  // TODO(T38780562): Handle Int subclasses

  // Try calling __int__
  HandleScope scope(thread);
  Frame* frame = thread->currentFrame();
  Object int_method(&scope, Interpreter::lookupMethod(thread, frame, object,
                                                      SymbolId::kDunderInt));
  if (int_method.isError()) {
    return thread->raiseTypeErrorWithCStr("an integer is required");
  }
  Object int_res(&scope,
                 Interpreter::callMethod1(thread, frame, int_method, object));
  if (int_res.isError()) return *int_res;
  if (!thread->runtime()->isInstanceOfInt(*int_res)) {
    return thread->raiseTypeErrorWithCStr("__int__ returned non-int");
  }

  // TODO(T38780562): Handle Int subclasses

  return *int_res;
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

  // We construct the IEEE754 number representation in an equally sized integer.
  static_assert(kWordSize == kDoubleSize, "expect equal word and double size");

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
  std::memcpy(result, &value_as_word, kDoubleSize);
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
    return thread->raiseOverflowErrorWithCStr(
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

}  // namespace python
