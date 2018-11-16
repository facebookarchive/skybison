#include "int-builtins.h"

#include <cerrno>
#include <climits>

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
    {SymbolId::kDunderBool, nativeTrampoline<dunderBool>},
    {SymbolId::kDunderEq, nativeTrampoline<dunderEq>},
    {SymbolId::kDunderGe, nativeTrampoline<dunderGe>},
    {SymbolId::kDunderGt, nativeTrampoline<dunderGt>},
    {SymbolId::kDunderLe, nativeTrampoline<dunderLe>},
    {SymbolId::kDunderLt, nativeTrampoline<dunderLt>},
    {SymbolId::kDunderNe, nativeTrampoline<dunderNe>},
    {SymbolId::kDunderNeg, nativeTrampoline<dunderNeg>},
    {SymbolId::kDunderPos, nativeTrampoline<dunderPos>},
};

void IntBuiltins::initialize(Runtime* runtime) {
  HandleScope scope;
  Handle<Type> type(&scope,
                    runtime->addBuiltinClass(SymbolId::kInt, LayoutId::kInt,
                                             LayoutId::kObject, kMethods));
  type->setFlag(Type::Flag::kIntSubclass);
  Handle<Type> largeint_type(
      &scope, runtime->addEmptyBuiltinClass(
                  SymbolId::kLargeInt, LayoutId::kLargeInt, LayoutId::kInt));
  largeint_type->setFlag(Type::Flag::kIntSubclass);
}

Object* IntBuiltins::dunderNew(Thread* thread, Frame* frame, word nargs) {
  if (nargs == 0) {
    return thread->throwTypeErrorFromCStr(
        "int.__new__(): not enough arguments");
  }
  if (nargs > 3) {
    return thread->throwTypeError(thread->runtime()->newStrFromFormat(
        "int() takes at most two arguments, %ld given", nargs - 1));
  }

  Runtime* runtime = thread->runtime();
  Arguments args(frame, nargs);
  HandleScope scope(thread);

  Handle<Object> type_obj(&scope, args.get(0));
  if (!runtime->hasSubClassFlag(*type_obj, Type::Flag::kTypeSubclass)) {
    return thread->throwTypeErrorFromCStr(
        "int.__new__(X): X is not a type object");
  }

  Handle<Type> type(&scope, *type_obj);
  if (!type->hasFlag(Type::Flag::kIntSubclass)) {
    return thread->throwTypeErrorFromCStr(
        "int.__new__(X): X is not a subtype of int");
  }

  Handle<Layout> layout(&scope, type->instanceLayout());
  if (layout->id() != LayoutId::kInt) {
    // TODO(dulinr): Implement __new__ with subtypes of int.
    UNIMPLEMENTED("int.__new__(<subtype of int>, ...)");
  }

  Handle<Object> arg(&scope, args.get(1));
  if (!arg->isStr()) {
    // TODO(dulinr): Handle non-string types.
    UNIMPLEMENTED("int(<non-string>)");
  }

  // No base argument, use 10 as the base.
  if (nargs == 2) {
    return intFromString(thread, *arg, 10);
  }

  // The third argument is the base of the integer represented in the string.
  Handle<Object> base(&scope, args.get(2));
  if (!base->isInt()) {
    // TODO(dulinr): Call __index__ on base to convert it.
    UNIMPLEMENTED("Can't handle non-integer base");
  }
  return intFromString(thread, *arg, Int::cast(*base)->asWord());
}

Object* IntBuiltins::intFromString(Thread* thread, Object* arg_raw, int base) {
  if (!(base == 0 || (base >= 2 && base <= 36))) {
    return thread->throwValueErrorFromCStr(
        "Invalid base, must be between 2 and 36, or 0");
  }
  HandleScope scope(thread);
  Handle<Object> arg(&scope, arg_raw);
  if (arg->isInt()) {
    return *arg;
  }

  CHECK(arg->isStr(), "not string type");
  Handle<Str> s(&scope, *arg);
  if (s->length() == 0) {
    return thread->throwValueErrorFromCStr("invalid literal");
  }
  char* c_str = s->toCStr();  // for strtol()
  char* end_ptr;
  errno = 0;
  long res = std::strtol(c_str, &end_ptr, base);
  int saved_errno = errno;
  bool is_complete = (*end_ptr == '\0');
  free(c_str);
  if (!is_complete || (res == 0 && saved_errno == EINVAL)) {
    return thread->throwValueErrorFromCStr("invalid literal");
  } else if ((res == LONG_MAX || res == LONG_MIN) && saved_errno == ERANGE) {
    return thread->throwValueErrorFromCStr("invalid literal (range)");
  } else if (!SmallInt::isValid(res)) {
    return thread->throwValueErrorFromCStr("unsupported type");
  }
  return SmallInt::fromWord(res);
}

Object* IntBuiltins::dunderInt(Thread* thread, Frame* frame, word nargs) {
  if (nargs < 1) {
    return thread->throwTypeErrorFromCStr("not enough arguments");
  }
  if (nargs > 1) {
    return thread->throwTypeError(thread->runtime()->newStrFromFormat(
        "expected 0 arguments, %ld given", nargs - 1));
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Handle<Object> arg(&scope, args.get(0));
  if (arg->isInt()) {
    return *arg;
  }
  if (arg->isBool()) {
    return SmallInt::fromWord(*arg == Bool::trueObj());
  }
  if (thread->runtime()->hasSubClassFlag(*arg, Type::Flag::kIntSubclass)) {
    UNIMPLEMENTED("Strict subclass of int");
  }
  return thread->throwTypeErrorFromCStr(
      "object cannot be interpreted as an integer");
}

const BuiltinMethod SmallIntBuiltins::kMethods[] = {
    {SymbolId::kDunderAdd, nativeTrampoline<dunderAdd>},
    {SymbolId::kDunderFloordiv, nativeTrampoline<dunderFloorDiv>},
    {SymbolId::kDunderInvert, nativeTrampoline<dunderInvert>},
    {SymbolId::kDunderMod, nativeTrampoline<dunderMod>},
    {SymbolId::kDunderMul, nativeTrampoline<dunderMul>},
    {SymbolId::kDunderSub, nativeTrampoline<dunderSub>},
    {SymbolId::kDunderXor, nativeTrampoline<dunderXor>},
};

void SmallIntBuiltins::initialize(Runtime* runtime) {
  HandleScope scope;
  Handle<Type> type(
      &scope, runtime->addBuiltinClass(SymbolId::kSmallInt, LayoutId::kSmallInt,
                                       LayoutId::kInt, kMethods));
  type->setFlag(Type::Flag::kIntSubclass);
  // We want to lookup the class of an immediate type by using the 5-bit tag
  // value as an index into the class table.  Replicate the class object for
  // SmallInt to all locations that decode to a SmallInt tag.
  for (word i = 1; i < 16; i++) {
    DCHECK(runtime->layoutAt(static_cast<LayoutId>(i << 1)) == None::object(),
           "list collision");
    runtime->layoutAtPut(static_cast<LayoutId>(i << 1), *type);
  }
}

Object* IntBuiltins::negateLargeInteger(Runtime* runtime,
                                        const Handle<Object>& large_integer) {
  HandleScope scope;
  View<uword> digits = LargeInt::cast(*large_integer)->digits();
  Handle<LargeInt> result(&scope,
                          runtime->heap()->createLargeInt(digits.length()));
  uword carry = 1;
  for (word i = 0; i < digits.length(); i++) {
    uword digit = digits.get(i) ^ kMaxUword;
    uword sum = digit + carry;
    result->digitAtPut(i, sum);
    if (sum >= digit) {
      carry = 0;
    }
  }
  DCHECK(carry == 0, "Carry should be zero");
  return *result;
}

Object* IntBuiltins::bitLength(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->throwTypeErrorFromCStr("expected 1 argument");
  }
  Arguments args(frame, nargs);
  Object* self = args.get(0);
  if (self->isBool()) {
    return intFromBool(self);
  }
  if (!self->isInt()) {
    return thread->throwTypeErrorFromCStr(
        "bit_length() must be called with int instance as the first argument");
  }
  return SmallInt::fromWord(Int::cast(self)->highestBit());
}

Object* IntBuiltins::dunderBool(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->throwTypeErrorFromCStr("expected 1 argument");
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
  return thread->throwTypeErrorFromCStr("unsupported type for __bool__");
}

Object* IntBuiltins::dunderEq(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCStr("expected 1 argument");
  }
  Arguments args(frame, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isBool()) {
    self = intFromBool(self);
  }
  if (other->isBool()) {
    other = intFromBool(other);
  }
  if (!self->isInt()) {
    return thread->throwTypeErrorFromCStr(
        "__eq__() must be called with int instance as the first argument");
  }

  Int* left = Int::cast(self);
  if (other->isInt()) {
    return Bool::fromBool(left->compare(Int::cast(other)) == 0);
  }
  return thread->runtime()->notImplemented();
}

Object* SmallIntBuiltins::dunderInvert(Thread* thread, Frame* frame,
                                       word nargs) {
  if (nargs != 1) {
    return thread->throwTypeErrorFromCStr("expected 1 argument");
  }
  Arguments args(frame, nargs);
  Object* self = args.get(0);

  if (self->isSmallInt()) {
    SmallInt* tos = SmallInt::cast(self);
    return SmallInt::fromWord(-(tos->value() + 1));
  }
  return thread->throwTypeErrorFromCStr("unsupported type for __invert__");
}

Object* IntBuiltins::dunderLe(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCStr("expected 1 argument");
  }
  Arguments args(frame, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isBool()) {
    self = intFromBool(self);
  }
  if (other->isBool()) {
    other = intFromBool(other);
  }
  if (!self->isInt()) {
    return thread->throwTypeErrorFromCStr(
        "__le__() must be called with int instance as the first argument");
  }

  Int* left = Int::cast(self);
  if (other->isInt()) {
    return Bool::fromBool(left->compare(Int::cast(other)) <= 0);
  }
  return thread->runtime()->notImplemented();
}

Object* SmallIntBuiltins::dunderFloorDiv(Thread* thread, Frame* frame,
                                         word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCStr("expected 1 argument");
  }
  Arguments args(frame, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (!self->isSmallInt()) {
    return thread->throwTypeErrorFromCStr(
        "__floordiv__() must be called with int instance as first argument");
  }
  word left = SmallInt::cast(self)->value();
  if (other->isInt()) {
    word right = Int::cast(other)->asWord();
    if (right == 0) {
      UNIMPLEMENTED("ZeroDivisionError");
    }
    return thread->runtime()->newInt(left / right);
  }
  return thread->runtime()->notImplemented();
}

Object* IntBuiltins::dunderLt(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCStr("expected 1 argument");
  }
  Arguments args(frame, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isBool()) {
    self = intFromBool(self);
  }
  if (other->isBool()) {
    other = intFromBool(other);
  }
  if (!self->isInt()) {
    return thread->throwTypeErrorFromCStr(
        "__lt__() must be called with int instance as the first argument");
  }

  Int* left = Int::cast(self);
  if (other->isInt()) {
    return Bool::fromBool(left->compare(Int::cast(other)) < 0);
  }
  return thread->runtime()->notImplemented();
}

Object* IntBuiltins::dunderGe(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCStr("expected 1 argument");
  }
  Arguments args(frame, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isBool()) {
    self = intFromBool(self);
  }
  if (other->isBool()) {
    other = intFromBool(other);
  }
  if (!self->isInt()) {
    return thread->throwTypeErrorFromCStr(
        "__ge__() must be called with int instance as the first argument");
  }

  Int* left = Int::cast(self);
  if (other->isInt()) {
    return Bool::fromBool(left->compare(Int::cast(other)) >= 0);
  }
  return thread->runtime()->notImplemented();
}

Object* IntBuiltins::dunderGt(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCStr("expected 1 argument");
  }
  Arguments args(frame, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isBool()) {
    self = intFromBool(self);
  }
  if (other->isBool()) {
    other = intFromBool(other);
  }
  if (!self->isInt()) {
    return thread->throwTypeErrorFromCStr(
        "__gt__() must be called with int instance as the first argument");
  }

  Int* left = Int::cast(self);
  if (other->isInt()) {
    return Bool::fromBool(left->compare(Int::cast(other)) > 0);
  }
  return thread->runtime()->notImplemented();
}

Object* SmallIntBuiltins::dunderMod(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCStr("expected 1 argument");
  }
  Arguments args(caller, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (!self->isSmallInt()) {
    return thread->throwTypeErrorFromCStr(
        "__mod__() must be called with int instance as first argument");
  }
  word left = SmallInt::cast(self)->value();
  if (other->isInt()) {
    word right = Int::cast(other)->asWord();
    if (right == 0) {
      UNIMPLEMENTED("ZeroDivisionError");
    }
    return thread->runtime()->newInt(left % right);
  }
  return thread->runtime()->notImplemented();
}

Object* SmallIntBuiltins::dunderMul(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCStr("expected 1 argument");
  }
  Arguments args(caller, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (!self->isSmallInt()) {
    return thread->throwTypeErrorFromCStr(
        "__mul__() must be called with int instance as first argument");
  }
  word left = SmallInt::cast(self)->value();
  if (other->isInt()) {
    word right = Int::cast(other)->asWord();
    word product = left * right;
    if (!(left == 0 || (product / left) == right)) {
      UNIMPLEMENTED("small integer overflow");
    }
    return thread->runtime()->newInt(product);
  }
  return thread->runtime()->notImplemented();
}

Object* IntBuiltins::dunderNe(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCStr("expected 1 argument");
  }
  Arguments args(frame, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isBool()) {
    self = intFromBool(self);
  }
  if (other->isBool()) {
    other = intFromBool(other);
  }
  if (!self->isInt()) {
    return thread->throwTypeErrorFromCStr(
        "__ne__() must be called with int instance as the first argument");
  }

  Int* left = Int::cast(self);
  if (other->isInt()) {
    return Bool::fromBool(left->compare(Int::cast(other)) != 0);
  }
  return thread->runtime()->notImplemented();
}

Object* IntBuiltins::dunderNeg(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);

  if (nargs != 1) {
    return thread->throwTypeErrorFromCStr("expected no arguments");
  }
  Handle<Object> self(&scope, args.get(0));
  if (self->isSmallInt()) {
    SmallInt* tos = SmallInt::cast(*self);
    word neg = -tos->value();
    if (SmallInt::isValid(neg)) {
      return SmallInt::fromWord(neg);
    }
    return thread->runtime()->newInt(neg);
  }
  if (self->isLargeInt()) {
    return negateLargeInteger(thread->runtime(), self);
  }
  if (self->isBool()) {
    return SmallInt::fromWord(*self == Bool::trueObj() ? -1 : 0);
  }
  return thread->throwTypeErrorFromCStr(
      "__neg__() must be called with int instance as the first argument");
}

Object* IntBuiltins::dunderPos(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->throwTypeErrorFromCStr("expected no arguments");
  }

  Arguments args(frame, nargs);
  Object* self = args.get(0);
  if (self->isSmallInt()) {
    return SmallInt::cast(self);
  }
  if (self->isLargeInt()) {
    return self;
  }
  if (self->isBool()) {
    return intFromBool(self);
  }
  return thread->throwTypeErrorFromCStr(
      "__neg__() must be called with int instance as the first argument");
}

inline Object* IntBuiltins::intFromBool(Object* bool_obj) {
  return SmallInt::fromWord(bool_obj == Bool::trueObj() ? 1 : 0);
}

Object* SmallIntBuiltins::dunderAdd(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCStr("expected 1 argument");
  }

  Arguments args(caller, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);

  if (!self->isSmallInt()) {
    return thread->throwTypeErrorFromCStr(
        "__add__() must be called with int instance as the first argument");
  }

  word left = SmallInt::cast(self)->value();
  if (other->isInt()) {
    word right = Int::cast(other)->asWord();
    return thread->runtime()->newInt(left + right);
  }
  return thread->runtime()->notImplemented();
}

Object* SmallIntBuiltins::dunderSub(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCStr("expected 1 argument");
  }

  Arguments args(frame, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);

  if (!self->isSmallInt()) {
    return thread->throwTypeErrorFromCStr(
        "__sub__() must be called with int instance as the first argument");
  }

  word left = SmallInt::cast(self)->value();
  if (other->isInt()) {
    word right = Int::cast(other)->asWord();
    return thread->runtime()->newInt(left - right);
  }

  // TODO(T30610701): Handle LargeIntegers
  return thread->runtime()->notImplemented();
}

Object* SmallIntBuiltins::dunderXor(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCStr("expected 1 argument");
  }
  Arguments args(frame, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (!self->isSmallInt()) {
    return thread->throwTypeErrorFromCStr(
        "__xor__() must be called with int instance as first argument");
  }
  word left = SmallInt::cast(self)->value();
  if (other->isInt()) {
    word right = Int::cast(other)->asWord();
    return thread->runtime()->newInt(left ^ right);
  }
  return thread->runtime()->notImplemented();
}

Object* BoolBuiltins::dunderNew(Thread* thread, Frame* frame, word nargs) {
  if (nargs == 0) {
    return thread->throwTypeErrorFromCStr(
        "bool.__new__(): not enough arguments");
  }
  if (nargs > 2) {
    return thread->throwTypeErrorFromCStr("bool() takes at most one argument");
  }

  Runtime* runtime = thread->runtime();
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Handle<Object> type_obj(&scope, args.get(0));
  if (!runtime->hasSubClassFlag(*type_obj, Type::Flag::kTypeSubclass)) {
    return thread->throwTypeErrorFromCStr(
        "bool.__new__(X): X is not a type object");
  }
  Handle<Type> type(&scope, *type_obj);

  // Since bool can't be subclassed, only need to check if the type is exactly
  // bool.
  Handle<Layout> layout(&scope, type->instanceLayout());
  if (layout->id() != LayoutId::kBool) {
    return thread->throwTypeErrorFromCStr("bool.__new__(X): X is not bool");
  }

  // If no arguments are given, return false.
  if (nargs == 1) {
    return Bool::falseObj();
  }

  Handle<Object> arg(&scope, args.get(1));
  // The interpreter reads the value from the frame, so add it on first.
  frame->pushValue(*arg);
  Object* result = Interpreter::isTrue(thread, frame);
  // Pop off the arg to isTrue before returning.
  frame->popValue();
  return result;
}

const BuiltinMethod BoolBuiltins::kMethods[] = {
    {SymbolId::kDunderNew, nativeTrampoline<dunderNew>},
};

void BoolBuiltins::initialize(Runtime* runtime) {
  HandleScope scope;
  Handle<Type> type(&scope,
                    runtime->addBuiltinClass(SymbolId::kBool, LayoutId::kBool,
                                             LayoutId::kInt, kMethods));
  type->setFlag(Type::Flag::kIntSubclass);
}

}  // namespace python
