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
    {SymbolId::kBitLength, builtinTrampolineWrapper<bitLength>},
    {SymbolId::kDunderAbs, builtinTrampolineWrapper<dunderAbs>},
    {SymbolId::kDunderAdd, builtinTrampolineWrapper<dunderAdd>},
    {SymbolId::kDunderAnd, builtinTrampolineWrapper<dunderAnd>},
    {SymbolId::kDunderBool, builtinTrampolineWrapper<dunderBool>},
    {SymbolId::kDunderEq, builtinTrampolineWrapper<dunderEq>},
    {SymbolId::kDunderFloat, builtinTrampolineWrapper<dunderFloat>},
    {SymbolId::kDunderGe, builtinTrampolineWrapper<dunderGe>},
    {SymbolId::kDunderGt, builtinTrampolineWrapper<dunderGt>},
    {SymbolId::kDunderIndex, builtinTrampolineWrapper<dunderInt>},
    {SymbolId::kDunderInt, builtinTrampolineWrapper<dunderInt>},
    {SymbolId::kDunderLe, builtinTrampolineWrapper<dunderLe>},
    {SymbolId::kDunderLshift, builtinTrampolineWrapper<dunderLshift>},
    {SymbolId::kDunderLt, builtinTrampolineWrapper<dunderLt>},
    {SymbolId::kDunderMul, builtinTrampolineWrapper<dunderMul>},
    {SymbolId::kDunderNe, builtinTrampolineWrapper<dunderNe>},
    {SymbolId::kDunderNeg, builtinTrampolineWrapper<dunderNeg>},
    {SymbolId::kDunderNew, nativeTrampoline<dunderNew>},
    {SymbolId::kDunderOr, builtinTrampolineWrapper<dunderOr>},
    {SymbolId::kDunderPos, builtinTrampolineWrapper<dunderPos>},
    {SymbolId::kDunderRepr, builtinTrampolineWrapper<dunderRepr>},
    {SymbolId::kDunderRshift, builtinTrampolineWrapper<dunderRshift>},
    {SymbolId::kDunderSub, builtinTrampolineWrapper<dunderSub>},
    {SymbolId::kDunderXor, builtinTrampolineWrapper<dunderXor>},
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
  largeint_type.setBuiltinBase(LayoutId::kInt);
}

RawObject IntBuiltins::asInt(const Int& value) {
  if (value.isBool()) {
    return RawSmallInt::fromWord(RawBool::cast(*value)->value() ? 1 : 0);
  }
  return *value;
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
  if (nargs == 2) {
    return intFromString(thread, *arg, 10);
  }

  // The third argument is the base of the integer represented in the string.
  Object base(&scope, args.get(2));
  if (!base.isInt()) {
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

RawObject IntBuiltins::dunderInt(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfInt(*self_obj)) {
    return thread->raiseTypeErrorWithCStr("'__int__' requires a 'int' object");
  }
  Int self(&scope, *self_obj);
  return asInt(self);
}

const BuiltinMethod SmallIntBuiltins::kMethods[] = {
    {SymbolId::kDunderFloordiv, nativeTrampoline<dunderFloorDiv>},
    {SymbolId::kDunderInvert, nativeTrampoline<dunderInvert>},
    {SymbolId::kDunderMod, nativeTrampoline<dunderMod>},
    {SymbolId::kDunderTruediv, nativeTrampoline<dunderTrueDiv>},
};

void SmallIntBuiltins::initialize(Runtime* runtime) {
  HandleScope scope;
  Type type(&scope, runtime->addBuiltinTypeWithMethods(
                        SymbolId::kSmallInt, LayoutId::kSmallInt,
                        LayoutId::kInt, kMethods));
  type.setBuiltinBase(LayoutId::kInt);
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

RawObject IntBuiltins::bitLength(Thread* thread, Frame* frame, word nargs) {
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

RawObject IntBuiltins::dunderAbs(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfInt(*self_obj)) {
    return thread->raiseTypeErrorWithCStr("'__abs__' requires a 'int' object");
  }
  Int self(&scope, *self_obj);
  return self.isNegative() ? thread->runtime()->intNegate(thread, self)
                           : asInt(self);
}

RawObject IntBuiltins::dunderAdd(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Object other_obj(&scope, args.get(1));
  if (!thread->runtime()->isInstanceOfInt(*self_obj)) {
    return thread->raiseTypeErrorWithCStr(
        "__add__() must be called with int instance as the first argument");
  }
  if (thread->runtime()->isInstanceOfInt(*other_obj)) {
    Int self(&scope, *self_obj);
    Int other(&scope, *other_obj);
    return runtime->intAdd(thread, self, other);
  }
  // signal to binary dispatch to try another method
  return runtime->notImplemented();
}

RawObject IntBuiltins::dunderAnd(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Object other(&scope, args.get(1));
  if (!thread->runtime()->isInstanceOfInt(*self)) {
    return thread->raiseTypeErrorWithCStr(
        "__and__() must be called with int instance as the first argument");
  }
  if (thread->runtime()->isInstanceOfInt(*other)) {
    Int self_int(&scope, *self);
    Int other_int(&scope, *other);
    return runtime->intBinaryAnd(thread, self_int, other_int);
  }
  // signal to binary dispatch to try another method
  return thread->runtime()->notImplemented();
}

RawObject IntBuiltins::dunderBool(Thread* thread, Frame* frame, word nargs) {
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
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfInt(*self_obj)) {
    return thread->raiseTypeErrorWithCStr(
        "'__float__' requires a 'int' object");
  }
  Int self(&scope, *self_obj);

  double value;
  Object maybe_error(&scope, convertIntToDouble(thread, self, &value));
  if (!maybe_error.isNoneType()) return *maybe_error;
  return runtime->newFloat(value);
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
  if (!signed_arg.isError()) {
    ++num_known_keywords;
    Object is_true(&scope, Interpreter::isTrue(thread, frame, signed_arg));
    if (is_true.isError()) return *is_true;
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

RawObject IntBuiltins::dunderMul(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Object other_obj(&scope, args.get(1));
  if (!thread->runtime()->isInstanceOfInt(*self_obj)) {
    return thread->raiseTypeErrorWithCStr(
        "__mul__() must be called with int instance as the first argument");
  }
  if (thread->runtime()->isInstanceOfInt(*other_obj)) {
    Int self(&scope, *self_obj);
    Int other(&scope, *other_obj);
    return runtime->intMultiply(thread, self, other);
  }
  // signal to binary dispatch to try another method
  return thread->runtime()->notImplemented();
}

RawObject IntBuiltins::dunderNe(Thread* thread, Frame* frame, word nargs) {
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

  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfInt(*self_obj)) {
    return thread->raiseTypeErrorWithCStr("'__neg__' requires a 'int' object");
  }
  Int self(&scope, *self_obj);
  return runtime->intNegate(thread, self);
}

RawObject IntBuiltins::dunderRshift(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  Object other_obj(&scope, args.get(1));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfInt(*self_obj)) {
    return thread->raiseTypeErrorWithCStr(
        "'__rshift__' requires a 'int' object");
  }
  if (runtime->isInstanceOfInt(*other_obj)) {
    Int self(&scope, *self_obj);
    Int other(&scope, *other_obj);
    if (other.isNegative()) {
      return thread->raiseValueErrorWithCStr("negative shift count");
    }
    return runtime->intBinaryRshift(thread, self, other);
  }
  // Signal binary dispatch to try another method.
  return runtime->notImplemented();
}

RawObject IntBuiltins::dunderSub(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Object other_obj(&scope, args.get(1));
  if (!thread->runtime()->isInstanceOfInt(*self_obj)) {
    return thread->raiseTypeErrorWithCStr(
        "__sub__() must be called with int instance as the first argument");
  }
  if (thread->runtime()->isInstanceOfInt(*other_obj)) {
    Int self(&scope, *self_obj);
    Int other(&scope, *other_obj);
    return runtime->intSubtract(thread, self, other);
  }
  // signal to binary dispatch to try another method
  return thread->runtime()->notImplemented();
}

RawObject IntBuiltins::dunderXor(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Object other(&scope, args.get(1));
  if (!thread->runtime()->isInstanceOfInt(*self)) {
    return thread->raiseTypeErrorWithCStr(
        "__xor__() must be called with int instance as the first argument");
  }
  if (thread->runtime()->isInstanceOfInt(*other)) {
    Int self_int(&scope, *self);
    Int other_int(&scope, *other);
    return runtime->intBinaryXor(thread, self_int, other_int);
  }
  // signal to binary dispatch to try another method
  return thread->runtime()->notImplemented();
}

RawObject IntBuiltins::dunderPos(Thread* thread, Frame* frame, word nargs) {
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

// TODO(T39167211): Merge with IntBuiltins::fromBytesKw / IntBuiltins::fromBytes
// once argument parsing is automated.
static RawObject fromBytesImpl(Thread* thread, const Object& bytes_obj,
                               const Object& byteorder_obj, bool is_signed) {
  HandleScope scope(thread);
  Object maybe_bytes(&scope, *bytes_obj);
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfBytes(*maybe_bytes)) {
    maybe_bytes = asBytes(thread, bytes_obj);
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

RawObject IntBuiltins::dunderOr(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Object other(&scope, args.get(1));
  if (!self.isInt()) {
    return thread->raiseTypeErrorWithCStr(
        "descriptor '__or__' requires a 'int' object");
  }
  if (other.isInt()) {
    Int self_int(&scope, *self);
    Int other_int(&scope, *other);
    return runtime->intBinaryOr(thread, self_int, other_int);
  }
  // signal to binary dispatch to try another method
  return thread->runtime()->notImplemented();
}

RawObject IntBuiltins::dunderLshift(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  Object other_obj(&scope, args.get(1));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfInt(*self_obj)) {
    return thread->raiseTypeErrorWithCStr(
        "'__lshift__' requires a 'int' object");
  }
  if (runtime->isInstanceOfInt(*other_obj)) {
    Int self(&scope, *self_obj);
    Int other(&scope, *other_obj);
    if (other.isNegative()) {
      return thread->raiseValueErrorWithCStr("negative shift count");
    }
    return runtime->intBinaryLshift(thread, self, other);
  }
  // Signal binary dispatch to try another method.
  return runtime->notImplemented();
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
      temp_digits[i] = ~large_int.digitAt(i) + carry;
      carry = (temp_digits[i] == 0);
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

const BuiltinMethod BoolBuiltins::kMethods[] = {
    {SymbolId::kDunderNew, nativeTrampoline<dunderNew>},
};

void BoolBuiltins::initialize(Runtime* runtime) {
  HandleScope scope;
  Type type(&scope,
            runtime->addBuiltinTypeWithMethods(SymbolId::kBool, LayoutId::kBool,
                                               LayoutId::kInt, kMethods));
  type.setBuiltinBase(LayoutId::kInt);
}

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

RawObject convertIntToDouble(Thread* thread, const Int& value, double* result) {
  if (value.numDigits() == 1) {
    *result = static_cast<double>(value.asWord());
    return NoneType::object();
  }

  // The following algorithm looks at the highest n bits of the integer and puts
  // them into the mantissa of the floating point number. It extracts two
  // extra bits to account for the highest bit not being explicitly encoded
  // in floating point and the lowest bit to decide whether we should round
  // up or down.

  // We construct the IEEE754 number representation in an equally sized integer.
  static_assert(kWordSize == kDoubleSize, "expect equal word and double size");

  // Extract the highest two digits of the numbers magnitude.
  HandleScope scope(thread);
  LargeInt large_int(&scope, *value);
  word num_digits = large_int.numDigits();
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
  bool lesser_significand_bits_zero;
  if (shift_left > 0) {
    int lower_shift_right = kBitsPerWord - shift_left;
    value_as_word |= second_highest_digit >> lower_shift_right;
    lesser_significand_bits_zero =
        (second_highest_digit << lower_shift_right) == 0;
  } else {
    lesser_significand_bits_zero =
        second_highest_digit == 0 &&
        (shift_right == 0 || (high_digit << (kBitsPerWord - shift_right)) == 0);
  }

  // Returns true if all digits (in the numbers magnitude) below the 2 highest
  // digits are zero.
  auto lower_digits_zero = [&]() -> bool {
    // Already scanned the digits in the negative case and can look at carry.
    if (is_negative) return carry_to_second_highest != 0;
    for (word i = num_digits - 3; i >= 0; i--) {
      if (large_int.digitAt(i) != 0) return false;
    }
    return true;
  };
  // We need to round down if the least significand bit is zero, we need to
  // round up if the least significand and any other bit is one. If the
  // least significand bit is one and all other bits are zero then we look at
  // second least significand bit to round towards an even number.
  if ((value_as_word & 0x3) == 0x3 ||
      ((value_as_word & 1) &&
       (!lesser_significand_bits_zero || !lower_digits_zero()))) {
    value_as_word++;
    // This may have triggered an overflow, so we need to add 1 to the exponent.
    if (value_as_word == (uword{1} << (kDoubleMantissaBits + 2))) {
      exponent++;
    }
  }
  value_as_word >>= 1;

  // Check for overflow.
  // The biggest exponent is used to mark special numbers like NAN or INF.
  uword max_exponent = (1 << exponent_bits) - 1;
  if (exponent > max_exponent - 1) {
    return thread->raiseOverflowErrorWithCStr(
        "int too large to convert to float");
  }

  // Mask out implicit bit, combine mantissa, exponent and sign.
  value_as_word &= (uword{1} << kDoubleMantissaBits) - 1;
  value_as_word |= exponent << kDoubleMantissaBits;
  value_as_word |= uword{is_negative} << (kDoubleMantissaBits + exponent_bits);
  std::memcpy(result, &value_as_word, kDoubleSize);
  return NoneType::object();
}

}  // namespace python
