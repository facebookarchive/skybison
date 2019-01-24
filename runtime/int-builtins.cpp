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

const BuiltinMethod IntBuiltins::kMethods[] = {
    {SymbolId::kDunderNew, nativeTrampoline<dunderNew>},
    // For ints, __index__ and __int__ have the same behavior so they are mapped
    // to the same function
    {SymbolId::kDunderIndex, nativeTrampoline<dunderInt>},
    {SymbolId::kDunderInt, nativeTrampoline<dunderInt>},
    {SymbolId::kBitLength, nativeTrampoline<bitLength>},
    {SymbolId::kDunderAnd, nativeTrampoline<dunderAnd>},
    {SymbolId::kDunderBool, nativeTrampoline<dunderBool>},
    {SymbolId::kDunderEq, nativeTrampoline<dunderEq>},
    {SymbolId::kDunderFloat, nativeTrampoline<dunderFloat>},
    {SymbolId::kDunderGe, nativeTrampoline<dunderGe>},
    {SymbolId::kDunderGt, nativeTrampoline<dunderGt>},
    {SymbolId::kDunderLe, nativeTrampoline<dunderLe>},
    {SymbolId::kDunderLshift, nativeTrampoline<dunderLshift>},
    {SymbolId::kDunderLt, nativeTrampoline<dunderLt>},
    {SymbolId::kDunderNe, nativeTrampoline<dunderNe>},
    {SymbolId::kDunderNeg, nativeTrampoline<dunderNeg>},
    {SymbolId::kDunderOr, nativeTrampoline<dunderOr>},
    {SymbolId::kDunderPos, nativeTrampoline<dunderPos>},
    {SymbolId::kDunderXor, nativeTrampoline<dunderXor>},
};

void IntBuiltins::initialize(Runtime* runtime) {
  HandleScope scope;
  Type type(&scope,
            runtime->addBuiltinTypeWithMethods(SymbolId::kInt, LayoutId::kInt,
                                               LayoutId::kObject, kMethods));
  runtime->typeAddBuiltinFunctionKw(type, SymbolId::kToBytes,
                                    nativeTrampoline<toBytes>,
                                    nativeTrampolineKw<toBytesKw>);
  runtime->typeAddBuiltinFunctionKw(type, SymbolId::kFromBytes,
                                    nativeTrampoline<fromBytes>,
                                    nativeTrampolineKw<fromBytesKw>);

  Type largeint_type(
      &scope, runtime->addEmptyBuiltinType(
                  SymbolId::kLargeInt, LayoutId::kLargeInt, LayoutId::kInt));
  largeint_type->setBuiltinBase(LayoutId::kInt);
}

RawObject IntBuiltins::dunderNew(Thread* thread, Frame* frame, word nargs) {
  if (nargs == 0) {
    return thread->raiseTypeErrorWithCStr(
        "int.__new__(): not enough arguments");
  }
  if (nargs > 3) {
    return thread->raiseTypeError(thread->runtime()->newStrFromFormat(
        "int() takes at most two arguments, %ld given", nargs - 1));
  }

  Runtime* runtime = thread->runtime();
  Arguments args(frame, nargs);
  HandleScope scope(thread);

  Object type_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfType(*type_obj)) {
    return thread->raiseTypeErrorWithCStr(
        "int.__new__(X): X is not a type object");
  }

  Type type(&scope, *type_obj);
  if (type->builtinBase() != LayoutId::kInt) {
    return thread->raiseTypeErrorWithCStr(
        "int.__new__(X): X is not a subtype of int");
  }

  Layout layout(&scope, type->instanceLayout());
  if (layout->id() != LayoutId::kInt) {
    // TODO(dulinr): Implement __new__ with subtypes of int.
    UNIMPLEMENTED("int.__new__(<subtype of int>, ...)");
  }

  Object arg(&scope, args.get(1));
  if (!arg->isStr()) {
    // TODO(dulinr): Handle non-string types.
    UNIMPLEMENTED("int(<non-string>)");
  }

  // No base argument, use 10 as the base.
  if (nargs == 2) {
    return intFromString(thread, *arg, 10);
  }

  // The third argument is the base of the integer represented in the string.
  Object base(&scope, args.get(2));
  if (!base->isInt()) {
    // TODO(dulinr): Call __index__ on base to convert it.
    UNIMPLEMENTED("Can't handle non-integer base");
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
  if (arg->isInt()) {
    return *arg;
  }

  CHECK(arg->isStr(), "not string type");
  Str s(&scope, *arg);
  if (s->length() == 0) {
    return thread->raiseValueErrorWithCStr("invalid literal");
  }
  char* c_str = s->toCStr();  // for strtol()
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

RawObject IntBuiltins::dunderInt(Thread* thread, Frame* frame, word nargs) {
  if (nargs < 1) {
    return thread->raiseTypeErrorWithCStr("not enough arguments");
  }
  if (nargs > 1) {
    return thread->raiseTypeError(thread->runtime()->newStrFromFormat(
        "expected 0 arguments, %ld given", nargs - 1));
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object arg(&scope, args.get(0));
  if (arg->isBool()) {
    return intFromBool(*arg);
  }
  if (arg->isInt()) {
    return *arg;
  }
  if (thread->runtime()->isInstanceOfInt(*arg)) {
    UNIMPLEMENTED("Strict subclass of int");
  }
  return thread->raiseTypeErrorWithCStr(
      "object cannot be interpreted as an integer");
}

const BuiltinMethod SmallIntBuiltins::kMethods[] = {
    {SymbolId::kDunderAdd, nativeTrampoline<dunderAdd>},
    {SymbolId::kDunderFloordiv, nativeTrampoline<dunderFloorDiv>},
    {SymbolId::kDunderInvert, nativeTrampoline<dunderInvert>},
    {SymbolId::kDunderMod, nativeTrampoline<dunderMod>},
    {SymbolId::kDunderMul, nativeTrampoline<dunderMul>},
    {SymbolId::kDunderSub, nativeTrampoline<dunderSub>},
    {SymbolId::kDunderTruediv, nativeTrampoline<dunderTrueDiv>},
    {SymbolId::kDunderRepr, nativeTrampoline<dunderRepr>},
};

void SmallIntBuiltins::initialize(Runtime* runtime) {
  HandleScope scope;
  Type type(&scope, runtime->addBuiltinTypeWithMethods(
                        SymbolId::kSmallInt, LayoutId::kSmallInt,
                        LayoutId::kInt, kMethods));
  type->setBuiltinBase(LayoutId::kInt);
  // We want to lookup the class of an immediate type by using the 5-bit tag
  // value as an index into the class table.  Replicate the class object for
  // SmallInt to all locations that decode to a SmallInt tag.
  for (word i = 1; i < 16; i++) {
    DCHECK(
        runtime->layoutAt(static_cast<LayoutId>(i << 1)) == NoneType::object(),
        "list collision");
    runtime->layoutAtPut(static_cast<LayoutId>(i << 1), *type);
  }
}

// Does this LargeInt use fewer digits now than it would after being negated?
static bool negationOverflows(const LargeInt& value) {
  word num_digits = value->numDigits();
  for (word i = 0; i < num_digits - 1; i++) {
    if (value->digitAt(i) != 0) {
      return false;
    }
  }
  return value->digitAt(num_digits - 1) == uword{1} << (kBitsPerWord - 1);
}

RawObject IntBuiltins::negateLargeInt(Runtime* runtime,
                                      const Object& large_int) {
  HandleScope scope;
  LargeInt src(&scope, *large_int);

  word num_digits = src->numDigits();
  // -i needs more digits than i if negating it overflows.
  if (negationOverflows(src)) {
    LargeInt result(&scope, runtime->heap()->createLargeInt(num_digits + 1));
    for (word i = 0; i < num_digits; i++) {
      result->digitAtPut(i, src->digitAt(i));
    }
    result->digitAtPut(num_digits, 0);
    DCHECK(result->isValid(), "Invalid RawLargeInt");
    return *result;
  }

  // -i fits in a SmallInt when i == SmallInt::kMaxValue + 1.
  if (num_digits == 1 && src->digitAt(0) == RawSmallInt::kMaxValue + 1) {
    return SmallInt::fromWord(RawSmallInt::kMinValue);
  }

  LargeInt result(&scope, runtime->heap()->createLargeInt(num_digits));
  word carry = 1;
  for (word i = 0; i < num_digits; i++) {
    word digit = src->digitAt(i);
    carry = __builtin_saddl_overflow(~digit, carry, &digit);
    result->digitAtPut(i, digit);
  }
  DCHECK(carry == 0, "Carry should be zero");
  DCHECK(result->isValid(), "Invalid RawLargeInt");
  return *result;
}

RawObject IntBuiltins::bitLength(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("expected 1 argument");
  }
  Arguments args(frame, nargs);
  RawObject self = args.get(0);
  if (self->isBool()) {
    return intFromBool(self);
  }
  if (!self->isInt()) {
    return thread->raiseTypeErrorWithCStr(
        "bit_length() must be called with int instance as the first argument");
  }
  return SmallInt::fromWord(RawInt::cast(self)->bitLength());
}

RawObject IntBuiltins::dunderAnd(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("expected 1 argument");
  }
  Arguments args(frame, nargs);
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Object other(&scope, args.get(1));
  if (!thread->runtime()->isInstanceOfInt(self)) {
    return thread->raiseTypeErrorWithCStr(
        "__and__() must be called with int instance as the first argument");
  }
  if (thread->runtime()->isInstanceOfInt(other)) {
    Int self_int(&scope, *self);
    Int other_int(&scope, *other);
    return runtime->intBinaryAnd(thread, self_int, other_int);
  }
  // signal to binary dispatch to try another method
  return thread->runtime()->notImplemented();
}

RawObject IntBuiltins::dunderBool(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("expected 1 argument");
  }
  Arguments args(frame, nargs);
  if (args.get(0)->isBool()) {
    return args.get(0);
  }
  if (args.get(0)->isSmallInt()) {
    return (args.get(0) == SmallInt::fromWord(0)) ? Bool::falseObj()
                                                  : Bool::trueObj();
  }
  if (args.get(0)->isLargeInt()) {
    return Bool::trueObj();
  }
  return thread->raiseTypeErrorWithCStr("unsupported type for __bool__");
}

RawObject IntBuiltins::dunderEq(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("expected 1 argument");
  }
  Arguments args(frame, nargs);
  RawObject self = args.get(0);
  RawObject other = args.get(1);
  if (self->isBool()) {
    self = intFromBool(self);
  }
  if (other->isBool()) {
    other = intFromBool(other);
  }
  if (!self->isInt()) {
    return thread->raiseTypeErrorWithCStr(
        "__eq__() must be called with int instance as the first argument");
  }

  RawInt left = RawInt::cast(self);
  if (other->isInt()) {
    return Bool::fromBool(left->compare(RawInt::cast(other)) == 0);
  }
  return thread->runtime()->notImplemented();
}

RawObject IntBuiltins::dunderFloat(Thread* thread, Frame* frame, word nargs) {
  if (nargs < 1) {
    return thread->raiseTypeErrorWithCStr("missing self");
  }
  if (nargs > 1) {
    return thread->raiseTypeError(thread->runtime()->newStrFromFormat(
        "expected 0 arguments, got %ld", nargs - 1));
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (self->isBool()) {
    return runtime->newFloat(*self == Bool::trueObj() ? 1 : 0);
  }
  if (self->isInt()) {
    Int self_int(&scope, *self);
    return runtime->newFloat(self_int->floatValue());
  }
  if (runtime->isInstanceOfInt(*self)) {
    UNIMPLEMENTED("Strict subclass of int");
  }
  return thread->raiseTypeErrorWithCStr(
      "object cannot be interpreted as an integer");
}

RawObject SmallIntBuiltins::dunderInvert(Thread* thread, Frame* frame,
                                         word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("expected 1 argument");
  }
  Arguments args(frame, nargs);
  RawObject self = args.get(0);

  if (self->isSmallInt()) {
    RawSmallInt tos = RawSmallInt::cast(self);
    return SmallInt::fromWord(-(tos->value() + 1));
  }
  return thread->raiseTypeErrorWithCStr("unsupported type for __invert__");
}

RawObject IntBuiltins::dunderLe(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("expected 1 argument");
  }
  Arguments args(frame, nargs);
  RawObject self = args.get(0);
  RawObject other = args.get(1);
  if (self->isBool()) {
    self = intFromBool(self);
  }
  if (other->isBool()) {
    other = intFromBool(other);
  }
  if (!self->isInt()) {
    return thread->raiseTypeErrorWithCStr(
        "__le__() must be called with int instance as the first argument");
  }

  RawInt left = RawInt::cast(self);
  if (other->isInt()) {
    return Bool::fromBool(left->compare(RawInt::cast(other)) <= 0);
  }
  return thread->runtime()->notImplemented();
}

static RawObject toBytesImpl(Thread* thread, const Object& self_obj,
                             const Object& length_obj,
                             const Object& byteorder_obj, bool is_signed) {
  HandleScope scope;
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfInt(*self_obj)) {
    return thread->raiseTypeErrorWithCStr(
        "descriptor 'to_bytes' requires a 'int' object");
  }
  Int self(&scope, *self_obj);

  if (!runtime->isInstanceOfInt(length_obj)) {
    return thread->raiseTypeErrorWithCStr(
        "length argument cannot be interpreted as an integer");
  }
  Int length_int(&scope, *length_obj);
  OptInt<word> l = length_int->asInt<word>();
  if (l.error != CastError::None) {
    return thread->raiseOverflowErrorWithCStr(
        "Python int too large to convert to C word");
  }
  word length = l.value;
  if (length < 0) {
    return thread->raiseValueErrorWithCStr(
        "length argument must be non-negative");
  }

  if (!runtime->isInstanceOfStr(byteorder_obj)) {
    return thread->raiseTypeErrorWithCStr(
        "to_bytes() argument 2 must be str, not int");
  }
  Str byteorder(&scope, *byteorder_obj);
  endian endianness;
  if (byteorder->equals(runtime->symbols()->Little())) {
    endianness = endian::little;
  } else if (byteorder->equals(runtime->symbols()->Big())) {
    endianness = endian::big;
  } else {
    return thread->raiseValueErrorWithCStr(
        "byteorder must be either 'little' or 'big'");
  }

  if (!is_signed && self->isNegative()) {
    return thread->raiseOverflowErrorWithCStr(
        "can't convert negative int to unsigned");
  }

  // Check for overflow.
  word num_digits = self->numDigits();
  uword high_digit = self->digitAt(num_digits - 1);
  word bit_length =
      num_digits * kBitsPerWord - Utils::numRedundantSignBits(high_digit);
  if (bit_length > length * kBitsPerByte + !is_signed) {
    return thread->raiseOverflowErrorWithCStr("int too big to convert");
  }

  return runtime->intToBytes(thread, self, length, endianness);
}

RawObject IntBuiltins::toBytes(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 3) {
    return thread->raiseTypeErrorWithCStr("expected 2 arguments");
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  Object length(&scope, args.get(1));
  Object byteorder(&scope, args.get(2));
  return toBytesImpl(thread, self, length, byteorder, false);
}

RawObject IntBuiltins::toBytesKw(Thread* thread, Frame* frame, word nargs) {
  KwArguments args(frame, nargs);
  if (args.numArgs() < 1) {
    return thread->raiseTypeErrorWithCStr(
        "descriptor 'to_bytes' of 'int' object needs an argument");
  }
  if (args.numArgs() > 3) {
    return thread->raiseTypeError(thread->runtime()->newStrFromFormat(
        "to_bytes() takes at most 2 positional arguments (%ld given)",
        args.numArgs() - 1));
  }

  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();

  Object self(&scope, args.get(0));
  word num_known_keywords = 0;
  Object length(&scope, args.getKw(runtime->symbols()->Length()));
  if (args.numArgs() > 1) {
    if (!length.isError()) {
      return thread->raiseTypeErrorWithCStr(
          "argument for to_bytes() given by name ('length') and position (1)");
    }
    length = args.get(1);
  } else {
    if (length.isError()) {
      return thread->raiseTypeErrorWithCStr(
          "to_bytes() missing required argument 'length' (pos 1)");
    }
    ++num_known_keywords;
  }

  Object byteorder(&scope, args.getKw(runtime->symbols()->Byteorder()));
  if (args.numArgs() > 2) {
    if (!byteorder.isError()) {
      return thread->raiseTypeErrorWithCStr(
          "argument for to_bytes() given by name ('byteorder') and position "
          "(2)");
    }
    byteorder = args.get(2);
  } else {
    if (byteorder.isError()) {
      return thread->raiseTypeErrorWithCStr(
          "to_bytes() missing required argument 'byteorder' (pos 2)");
    }
    ++num_known_keywords;
  }

  bool is_signed = false;
  Object signed_arg(&scope, args.getKw(runtime->symbols()->Signed()));
  if (!signed_arg->isError()) {
    ++num_known_keywords;
    Object is_true(&scope, Interpreter::isTrue(thread, frame, signed_arg));
    if (is_true->isError()) return *is_true;
    is_signed = is_true == Bool::trueObj();
  }

  if (args.numKeywords() != num_known_keywords) {
    return thread->raiseTypeErrorWithCStr(
        "to_bytes() called with invalid keyword arguments");
  }

  return toBytesImpl(thread, self, length, byteorder, is_signed);
}

RawObject SmallIntBuiltins::dunderFloorDiv(Thread* thread, Frame* frame,
                                           word nargs) {
  Runtime* runtime = thread->runtime();
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("expected 1 argument");
  }
  Arguments args(frame, nargs);
  RawObject self = args.get(0);
  RawObject other = args.get(1);
  if (!self->isSmallInt()) {
    return thread->raiseTypeErrorWithCStr(
        "__floordiv__() must be called with int instance as first argument");
  }
  word left = RawSmallInt::cast(self)->value();
  if (other->isFloat()) {
    double right = RawFloat::cast(other)->value();
    if (right == 0.0) {
      return thread->raiseZeroDivisionErrorWithCStr("float divmod()");
    }
    return runtime->newFloat(std::floor(left / right));
  }
  if (other->isBool()) {
    other = IntBuiltins::intFromBool(other);
  }
  if (other->isInt()) {
    word right = RawInt::cast(other)->asWord();
    if (right == 0) {
      return thread->raiseZeroDivisionErrorWithCStr(
          "integer division or modulo by zero");
    }
    return runtime->newInt(left / right);
  }
  return runtime->notImplemented();
}

RawObject SmallIntBuiltins::dunderTrueDiv(Thread* thread, Frame* frame,
                                          word nargs) {
  Runtime* runtime = thread->runtime();
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("expected 1 argument");
  }
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

RawObject IntBuiltins::dunderLt(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("expected 1 argument");
  }
  Arguments args(frame, nargs);
  RawObject self = args.get(0);
  RawObject other = args.get(1);
  if (self->isBool()) {
    self = intFromBool(self);
  }
  if (other->isBool()) {
    other = intFromBool(other);
  }
  if (!self->isInt()) {
    return thread->raiseTypeErrorWithCStr(
        "__lt__() must be called with int instance as the first argument");
  }

  RawInt left = RawInt::cast(self);
  if (other->isInt()) {
    return Bool::fromBool(left->compare(RawInt::cast(other)) < 0);
  }
  return thread->runtime()->notImplemented();
}

RawObject IntBuiltins::dunderGe(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("expected 1 argument");
  }
  Arguments args(frame, nargs);
  RawObject self = args.get(0);
  RawObject other = args.get(1);
  if (self->isBool()) {
    self = intFromBool(self);
  }
  if (other->isBool()) {
    other = intFromBool(other);
  }
  if (!self->isInt()) {
    return thread->raiseTypeErrorWithCStr(
        "__ge__() must be called with int instance as the first argument");
  }

  RawInt left = RawInt::cast(self);
  if (other->isInt()) {
    return Bool::fromBool(left->compare(RawInt::cast(other)) >= 0);
  }
  return thread->runtime()->notImplemented();
}

RawObject IntBuiltins::dunderGt(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("expected 1 argument");
  }
  Arguments args(frame, nargs);
  RawObject self = args.get(0);
  RawObject other = args.get(1);
  if (self->isBool()) {
    self = intFromBool(self);
  }
  if (other->isBool()) {
    other = intFromBool(other);
  }
  if (!self->isInt()) {
    return thread->raiseTypeErrorWithCStr(
        "__gt__() must be called with int instance as the first argument");
  }

  RawInt left = RawInt::cast(self);
  if (other->isInt()) {
    return Bool::fromBool(left->compare(RawInt::cast(other)) > 0);
  }
  return thread->runtime()->notImplemented();
}

RawObject SmallIntBuiltins::dunderMod(Thread* thread, Frame* frame,
                                      word nargs) {
  Runtime* runtime = thread->runtime();
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("expected 1 argument");
  }
  Arguments args(frame, nargs);
  RawObject self = args.get(0);
  RawObject other = args.get(1);
  if (!self->isSmallInt()) {
    return thread->raiseTypeErrorWithCStr(
        "__mod__() must be called with int instance as first argument");
  }
  word left = RawSmallInt::cast(self)->value();
  if (other->isFloat()) {
    double right = RawFloat::cast(other)->value();
    if (right == 0.0) {
      return thread->raiseZeroDivisionErrorWithCStr("float modulo");
    }
    double mod = std::fmod(static_cast<double>(left), right);
    if (mod) {
      // Ensure that the remainder has the same sign as the denominator.
      if ((right < 0) != (mod < 0)) {
        mod += right;
      }
    } else {
      // Avoid signed zeros.
      mod = std::copysign(0.0, right);
    }
    return runtime->newFloat(mod);
  }
  if (other->isBool()) {
    other = IntBuiltins::intFromBool(other);
  }
  if (other->isInt()) {
    word right = RawInt::cast(other)->asWord();
    if (right == 0) {
      return thread->raiseZeroDivisionErrorWithCStr(
          "integer division or modulo by zero");
    }
    return runtime->newInt(left % right);
  }
  return runtime->notImplemented();
}

RawObject SmallIntBuiltins::dunderMul(Thread* thread, Frame* frame,
                                      word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("expected 1 argument");
  }
  Arguments args(frame, nargs);
  RawObject self = args.get(0);
  RawObject other = args.get(1);
  if (!self->isSmallInt()) {
    return thread->raiseTypeErrorWithCStr(
        "__mul__() must be called with int instance as first argument");
  }
  word left = RawSmallInt::cast(self)->value();
  if (other->isInt()) {
    word right = RawInt::cast(other)->asWord();
    word product;
    if (__builtin_mul_overflow(left, right, &product)) {
      UNIMPLEMENTED("small integer overflow");
    }
    return thread->runtime()->newInt(product);
  }
  return thread->runtime()->notImplemented();
}

RawObject IntBuiltins::dunderNe(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("expected 1 argument");
  }
  Arguments args(frame, nargs);
  RawObject self = args.get(0);
  RawObject other = args.get(1);
  if (self->isBool()) {
    self = intFromBool(self);
  }
  if (other->isBool()) {
    other = intFromBool(other);
  }
  if (!self->isInt()) {
    return thread->raiseTypeErrorWithCStr(
        "__ne__() must be called with int instance as the first argument");
  }

  RawInt left = RawInt::cast(self);
  if (other->isInt()) {
    return Bool::fromBool(left->compare(RawInt::cast(other)) != 0);
  }
  return thread->runtime()->notImplemented();
}

RawObject IntBuiltins::dunderNeg(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);

  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("expected no arguments");
  }
  Object self(&scope, args.get(0));
  if (self->isSmallInt()) {
    RawSmallInt tos = RawSmallInt::cast(*self);
    word neg = -tos->value();
    if (SmallInt::isValid(neg)) {
      return SmallInt::fromWord(neg);
    }
    return thread->runtime()->newInt(neg);
  }
  if (self->isLargeInt()) {
    return negateLargeInt(thread->runtime(), self);
  }
  if (self->isBool()) {
    return SmallInt::fromWord(*self == Bool::trueObj() ? -1 : 0);
  }
  return thread->raiseTypeErrorWithCStr(
      "__neg__() must be called with int instance as the first argument");
}

RawObject IntBuiltins::dunderXor(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("expected 1 argument");
  }
  Arguments args(frame, nargs);
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Object other(&scope, args.get(1));
  if (!thread->runtime()->isInstanceOfInt(self)) {
    return thread->raiseTypeErrorWithCStr(
        "__xor__() must be called with int instance as the first argument");
  }
  if (thread->runtime()->isInstanceOfInt(other)) {
    Int self_int(&scope, *self);
    Int other_int(&scope, *other);
    return runtime->intBinaryXor(thread, self_int, other_int);
  }
  // signal to binary dispatch to try another method
  return thread->runtime()->notImplemented();
}

RawObject IntBuiltins::dunderPos(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("expected no arguments");
  }

  Arguments args(frame, nargs);
  RawObject self = args.get(0);
  if (self->isSmallInt()) {
    return RawSmallInt::cast(self);
  }
  if (self->isLargeInt()) {
    return self;
  }
  if (self->isBool()) {
    return intFromBool(self);
  }
  return thread->raiseTypeErrorWithCStr(
      "__neg__() must be called with int instance as the first argument");
}

static RawObject asBytesObject(Thread* thread, const Object& object) {
  Runtime* runtime = thread->runtime();
  if (runtime->isInstanceOfBytes(object)) {
    return *object;
  }

  HandleScope scope(thread);
  Frame* frame = thread->currentFrame();
  Object bytes_method(
      &scope,
      Interpreter::lookupMethod(thread, frame, object, SymbolId::kDunderBytes));
  if (bytes_method->isError()) {
    return thread->raiseTypeErrorWithCStr("cannot convert object to bytes");
  }
  Object bytes_result(
      &scope, Interpreter::callMethod1(thread, frame, bytes_method, object));
  if (bytes_result->isError() || runtime->isInstanceOfBytes(*bytes_result)) {
    return *bytes_result;
  }
  // TODO(T39313322) implement missing ways to convert objects to bytes.

  return thread->raiseTypeErrorWithCStr("__bytes__ returned non-bytes");
}

// TODO(T39167211): Merge with IntBuiltins::fromBytesKw / IntBuiltins::fromBytes
// once argument parsing is automated.
static RawObject fromBytesImpl(Thread* thread, const Object& bytes_obj,
                               const Object& byteorder_obj, bool is_signed) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object maybe_bytes(&scope, asBytesObject(thread, bytes_obj));
  if (maybe_bytes->isError()) return *maybe_bytes;
  Bytes bytes(&scope, *maybe_bytes);

  if (!runtime->isInstanceOfStr(byteorder_obj)) {
    return thread->raiseTypeErrorWithCStr(
        "from_bytes() must be called with str instance as second argument");
  }
  Str byteorder(&scope, *byteorder_obj);
  endian endianness;
  if (byteorder->equals(runtime->symbols()->Little())) {
    endianness = endian::little;
  } else if (byteorder->equals(runtime->symbols()->Big())) {
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
    if (!byteorder->isError()) {
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
  if (!signed_arg->isError()) {
    ++num_known_keywords;
    Object is_true(&scope, Interpreter::isTrue(thread, frame, signed_arg));
    if (is_true->isError()) return *is_true;
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

RawObject IntBuiltins::dunderOr(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("expected 1 argument");
  }
  Arguments args(frame, nargs);
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Object other(&scope, args.get(1));
  if (!self->isInt()) {
    return thread->raiseTypeErrorWithCStr(
        "descriptor '__or__' requires a 'int' object");
  }
  if (other->isInt()) {
    Int self_int(&scope, *self);
    Int other_int(&scope, *other);
    return runtime->intBinaryOr(thread, self_int, other_int);
  }
  // signal to binary dispatch to try another method
  return thread->runtime()->notImplemented();
}

RawObject IntBuiltins::dunderLshift(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("expected 1 argument");
  }
  Arguments args(frame, nargs);
  Runtime* runtime = thread->runtime();
  HandleScope scope;
  Object self(&scope, args.get(0));
  Object arg(&scope, args.get(1));
  if (!self->isInt()) {
    return thread->raiseTypeErrorWithCStr(
        "'__lshift__' requires a 'int' object");
  }
  if (arg->isInt()) {
    Int arg_int(&scope, *arg);
    if (arg_int->isNegative()) {
      return thread->raiseValueErrorWithCStr("negative shift count");
    }
    if (arg_int->isLargeInt()) {
      return thread->raiseOverflowErrorWithCStr("shift count too large");
    }
    Int self_int(&scope, *self);
    return runtime->intBinaryLshift(thread, self_int, arg_int->asWord());
  }
  // signal to binary dispatch to try another method
  return thread->runtime()->notImplemented();
}

RawObject SmallIntBuiltins::dunderAdd(Thread* thread, Frame* frame,
                                      word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("expected 1 argument");
  }

  Arguments args(frame, nargs);
  RawObject self = args.get(0);
  RawObject other = args.get(1);

  if (!self->isSmallInt()) {
    return thread->raiseTypeErrorWithCStr(
        "__add__() must be called with int instance as the first argument");
  }

  word left = RawSmallInt::cast(self)->value();
  if (other->isInt()) {
    word right = RawInt::cast(other)->asWord();
    return thread->runtime()->newInt(left + right);
  }
  return thread->runtime()->notImplemented();
}

RawObject SmallIntBuiltins::dunderSub(Thread* thread, Frame* frame,
                                      word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("expected 1 argument");
  }

  Arguments args(frame, nargs);
  RawObject self = args.get(0);
  RawObject other = args.get(1);

  if (!self->isSmallInt()) {
    return thread->raiseTypeErrorWithCStr(
        "__sub__() must be called with int instance as the first argument");
  }

  word left = RawSmallInt::cast(self)->value();
  if (other->isInt()) {
    word right = RawInt::cast(other)->asWord();
    return thread->runtime()->newInt(left - right);
  }

  // TODO(T30610701): Handle LargeIntegers
  return thread->runtime()->notImplemented();
}

RawObject SmallIntBuiltins::dunderRepr(Thread* thread, Frame* frame,
                                       word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("expected no arguments");
  }
  Arguments args(frame, nargs);
  RawObject self = args.get(0);
  if (!self->isSmallInt()) {
    return thread->raiseTypeErrorWithCStr(
        "__repr__() must be called with int instance as first argument");
  }
  word value = RawSmallInt::cast(self)->value();
  char buffer[kWordDigits10 + 1];
  int size = std::snprintf(buffer, sizeof(buffer), "%" PRIdPTR, value);
  (void)size;
  DCHECK(size < int{sizeof(buffer)}, "buffer too small");
  return thread->runtime()->newStrFromCStr(buffer);
}

RawObject BoolBuiltins::dunderNew(Thread* thread, Frame* frame, word nargs) {
  if (nargs == 0) {
    return thread->raiseTypeErrorWithCStr(
        "bool.__new__(): not enough arguments");
  }
  if (nargs > 2) {
    return thread->raiseTypeErrorWithCStr("bool() takes at most one argument");
  }

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
  Layout layout(&scope, type->instanceLayout());
  if (layout->id() != LayoutId::kBool) {
    return thread->raiseTypeErrorWithCStr("bool.__new__(X): X is not bool");
  }

  // If no arguments are given, return false.
  if (nargs == 1) {
    return Bool::falseObj();
  }

  Object arg(&scope, args.get(1));
  return Interpreter::isTrue(thread, frame, arg);
}

const BuiltinMethod BoolBuiltins::kMethods[] = {
    {SymbolId::kDunderNew, nativeTrampoline<dunderNew>},
};

void BoolBuiltins::initialize(Runtime* runtime) {
  HandleScope scope;
  Type type(&scope,
            runtime->addBuiltinTypeWithMethods(SymbolId::kBool, LayoutId::kBool,
                                               LayoutId::kInt, kMethods));
  type->setBuiltinBase(LayoutId::kInt);
}

RawObject asIntObject(Thread* thread, const Object& object) {
  if (object->isInt()) {
    return *object;
  }

  // TODO(T38780562): Handle Int subclasses

  // Try calling __int__
  HandleScope scope(thread);
  Frame* frame = thread->currentFrame();
  Object int_method(&scope, Interpreter::lookupMethod(thread, frame, object,
                                                      SymbolId::kDunderInt));
  if (int_method->isError()) {
    return thread->raiseTypeErrorWithCStr("an integer is required");
  }
  Object int_res(&scope,
                 Interpreter::callMethod1(thread, frame, int_method, object));
  if (int_res->isError()) return *int_res;
  if (!thread->runtime()->isInstanceOfInt(int_res)) {
    return thread->raiseTypeErrorWithCStr("__int__ returned non-int");
  }

  // TODO(T38780562): Handle Int subclasses

  return *int_res;
}

}  // namespace python
