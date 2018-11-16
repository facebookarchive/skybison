#include "int-builtins.h"

#include <cerrno>
#include <climits>

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "thread.h"
#include "trampolines-inl.h"

namespace python {

const BuiltinMethod IntegerBuiltins::kMethods[] = {
    {SymbolId::kDunderNew, nativeTrampoline<dunderNew>},
};

void IntegerBuiltins::initialize(Runtime* runtime) {
  HandleScope scope;
  Handle<Type> type(
      &scope, runtime->addEmptyBuiltinClass(SymbolId::kInt, LayoutId::kInteger,
                                            LayoutId::kObject));
  type->setFlag(Type::Flag::kIntSubclass);
  for (uword i = 0; i < ARRAYSIZE(kMethods); i++) {
    runtime->classAddBuiltinFunction(type, kMethods[i].name,
                                     kMethods[i].address);
  }
}

Object* IntegerBuiltins::dunderNew(Thread* thread, Frame* frame, word nargs) {
  if (nargs == 0) {
    return thread->throwTypeErrorFromCString(
        "int.__new__(): not enough arguments");
  }
  if (nargs > 3) {
    return thread->throwTypeError(thread->runtime()->newStringFromFormat(
        "int() takes at most two arguments, %ld given", nargs - 1));
  }

  Runtime* runtime = thread->runtime();
  Arguments args(frame, nargs);
  HandleScope scope(thread);

  Handle<Object> type_obj(&scope, args.get(0));
  if (!runtime->hasSubClassFlag(*type_obj, Type::Flag::kClassSubclass)) {
    return thread->throwTypeErrorFromCString(
        "int.__new__(X): X is not a type object");
  }

  Handle<Type> type(&scope, *type_obj);
  if (!type->hasFlag(Type::Flag::kIntSubclass)) {
    return thread->throwTypeErrorFromCString(
        "int.__new__(X): X is not a subtype of int");
  }

  Handle<Layout> layout(&scope, type->instanceLayout());
  if (layout->id() != LayoutId::kInteger) {
    // TODO(dulinr): Implement __new__ with subtypes of int.
    UNIMPLEMENTED("int.__new__(<subtype of int>, ...)");
  }

  Handle<Object> arg(&scope, args.get(1));
  if (!arg->isString()) {
    // TODO(dulinr): Handle non-string types.
    UNIMPLEMENTED("int(<non-string>)");
  }

  // No base argument, use 10 as the base.
  if (nargs == 2) {
    return intFromString(thread, *arg, 10);
  }

  // The third argument is the base of the integer represented in the string.
  Handle<Object> base(&scope, args.get(2));
  if (!base->isInteger()) {
    // TODO(dulinr): Call __index__ on base to convert it.
    UNIMPLEMENTED("Can't handle non-integer base");
  }
  return intFromString(thread, *arg, Integer::cast(*base)->asWord());
}

Object* IntegerBuiltins::intFromString(Thread* thread, Object* arg_raw,
                                       int base) {
  if (!(base == 0 || (base >= 2 && base <= 36))) {
    return thread->throwValueErrorFromCString(
        "Invalid base, must be between 2 and 36, or 0");
  }
  HandleScope scope(thread);
  Handle<Object> arg(&scope, arg_raw);
  if (arg->isInteger()) {
    return *arg;
  }

  CHECK(arg->isString(), "not string type");
  Handle<String> s(&scope, *arg);
  if (s->length() == 0) {
    return thread->throwValueErrorFromCString("invalid literal");
  }
  char* c_string = s->toCString();  // for strtol()
  char* end_ptr;
  errno = 0;
  long res = std::strtol(c_string, &end_ptr, base);
  int saved_errno = errno;
  bool is_complete = (*end_ptr == '\0');
  free(c_string);
  if (!is_complete || (res == 0 && saved_errno == EINVAL)) {
    return thread->throwValueErrorFromCString("invalid literal");
  } else if ((res == LONG_MAX || res == LONG_MIN) && saved_errno == ERANGE) {
    return thread->throwValueErrorFromCString("invalid literal (range)");
  } else if (!SmallInteger::isValid(res)) {
    return thread->throwValueErrorFromCString("unsupported type");
  }
  return SmallInteger::fromWord(res);
}

const BuiltinMethod SmallIntegerBuiltins::kMethods[] = {
    {SymbolId::kBitLength, nativeTrampoline<bitLength>},
    {SymbolId::kDunderBool, nativeTrampoline<dunderBool>},
    {SymbolId::kDunderEq, nativeTrampoline<dunderEq>},
    {SymbolId::kDunderInvert, nativeTrampoline<dunderInvert>},
    {SymbolId::kDunderLe, nativeTrampoline<dunderLe>},
    {SymbolId::kDunderFloordiv, nativeTrampoline<dunderFloorDiv>},
    {SymbolId::kDunderLt, nativeTrampoline<dunderLt>},
    {SymbolId::kDunderGe, nativeTrampoline<dunderGe>},
    {SymbolId::kDunderGt, nativeTrampoline<dunderGt>},
    {SymbolId::kDunderMod, nativeTrampoline<dunderMod>},
    {SymbolId::kDunderMul, nativeTrampoline<dunderMul>},
    {SymbolId::kDunderNe, nativeTrampoline<dunderNe>},
    {SymbolId::kDunderNeg, nativeTrampoline<dunderNeg>},
    {SymbolId::kDunderPos, nativeTrampoline<dunderPos>},
    {SymbolId::kDunderAdd, nativeTrampoline<dunderAdd>},
    {SymbolId::kDunderSub, nativeTrampoline<dunderSub>},
    {SymbolId::kDunderXor, nativeTrampoline<dunderXor>},
};

void SmallIntegerBuiltins::initialize(Runtime* runtime) {
  HandleScope scope;
  Handle<Type> type(&scope, runtime->addEmptyBuiltinClass(
                                SymbolId::kSmallInt, LayoutId::kSmallInteger,
                                LayoutId::kInteger));
  for (uword i = 0; i < ARRAYSIZE(kMethods); i++) {
    runtime->classAddBuiltinFunction(type, kMethods[i].name,
                                     kMethods[i].address);
  }
  // We want to lookup the class of an immediate type by using the 5-bit tag
  // value as an index into the class table.  Replicate the class object for
  // SmallInteger to all locations that decode to a SmallInteger tag.
  for (word i = 1; i < 16; i++) {
    DCHECK(runtime->layoutAt(static_cast<LayoutId>(i << 1)) == None::object(),
           "list collision");
    runtime->layoutAtPut(static_cast<LayoutId>(i << 1), *type);
  }
}

Object* SmallIntegerBuiltins::bitLength(Thread* thread, Frame* frame,
                                        word nargs) {
  if (nargs != 1) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(frame, nargs);
  Object* self = args.get(0);
  if (!self->isSmallInteger()) {
    return thread->throwTypeErrorFromCString(
        "bit_length() must be called with int instance as first argument");
  }
  uword number =
      static_cast<uword>(std::abs(SmallInteger::cast(self)->value()));
  return SmallInteger::fromWord(Utils::highestBit(number));
}

Object* SmallIntegerBuiltins::dunderBool(Thread* thread, Frame* frame,
                                         word nargs) {
  if (nargs != 1) {
    return thread->throwTypeErrorFromCString("not enough arguments");
  }
  Arguments args(frame, nargs);
  if (args.get(0)->isSmallInteger()) {
    return Boolean::fromBool(args.get(0) != SmallInteger::fromWord(0));
  }
  return thread->throwTypeErrorFromCString("unsupported type for __bool__");
}

Object* SmallIntegerBuiltins::dunderEq(Thread* thread, Frame* frame,
                                       word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(frame, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isSmallInteger() && other->isSmallInteger()) {
    return Boolean::fromBool(self == other);
  }
  return thread->runtime()->notImplemented();
}

Object* SmallIntegerBuiltins::dunderInvert(Thread* thread, Frame* frame,
                                           word nargs) {
  if (nargs != 1) {
    return thread->throwTypeErrorFromCString("not enough arguments");
  }
  Arguments args(frame, nargs);
  if (args.get(0)->isSmallInteger()) {
    SmallInteger* tos = SmallInteger::cast(args.get(0));
    return SmallInteger::fromWord(-(tos->value() + 1));
  }
  return thread->throwTypeErrorFromCString("unsupported type for __invert__");
}

Object* SmallIntegerBuiltins::dunderLe(Thread* thread, Frame* frame,
                                       word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(frame, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isSmallInteger() && other->isSmallInteger()) {
    SmallInteger* left = SmallInteger::cast(self);
    SmallInteger* right = SmallInteger::cast(other);
    return Boolean::fromBool(left->value() <= right->value());
  }
  return thread->runtime()->notImplemented();
}

Object* SmallIntegerBuiltins::dunderFloorDiv(Thread* thread, Frame* caller,
                                             word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(caller, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (!self->isSmallInteger()) {
    return thread->throwTypeErrorFromCString(
        "__floordiv__() must be called with int instance as first argument");
  }
  word left = SmallInteger::cast(self)->value();
  if (other->isInteger()) {
    word right = Integer::cast(other)->asWord();
    if (right == 0) {
      UNIMPLEMENTED("ZeroDivisionError");
    }
    return thread->runtime()->newInteger(left / right);
  }
  return thread->runtime()->notImplemented();
}

Object* SmallIntegerBuiltins::dunderLt(Thread* thread, Frame* frame,
                                       word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(frame, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isSmallInteger() && other->isSmallInteger()) {
    SmallInteger* left = SmallInteger::cast(self);
    SmallInteger* right = SmallInteger::cast(other);
    return Boolean::fromBool(left->value() < right->value());
  }
  return thread->runtime()->notImplemented();
}

Object* SmallIntegerBuiltins::dunderGe(Thread* thread, Frame* frame,
                                       word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(frame, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isSmallInteger() && other->isSmallInteger()) {
    SmallInteger* left = SmallInteger::cast(self);
    SmallInteger* right = SmallInteger::cast(other);
    return Boolean::fromBool(left->value() >= right->value());
  }
  return thread->runtime()->notImplemented();
}

Object* SmallIntegerBuiltins::dunderGt(Thread* thread, Frame* frame,
                                       word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(frame, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isSmallInteger() && other->isSmallInteger()) {
    SmallInteger* left = SmallInteger::cast(self);
    SmallInteger* right = SmallInteger::cast(other);
    return Boolean::fromBool(left->value() > right->value());
  }
  return thread->runtime()->notImplemented();
}

Object* SmallIntegerBuiltins::dunderMod(Thread* thread, Frame* caller,
                                        word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(caller, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (!self->isSmallInteger()) {
    return thread->throwTypeErrorFromCString(
        "__mod__() must be called with int instance as first argument");
  }
  word left = SmallInteger::cast(self)->value();
  if (other->isInteger()) {
    word right = Integer::cast(other)->asWord();
    if (right == 0) {
      UNIMPLEMENTED("ZeroDivisionError");
    }
    return thread->runtime()->newInteger(left % right);
  }
  return thread->runtime()->notImplemented();
}

Object* SmallIntegerBuiltins::dunderMul(Thread* thread, Frame* caller,
                                        word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(caller, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (!self->isSmallInteger()) {
    return thread->throwTypeErrorFromCString(
        "__mul__() must be called with int instance as first argument");
  }
  word left = SmallInteger::cast(self)->value();
  if (other->isInteger()) {
    word right = Integer::cast(other)->asWord();
    word product = left * right;
    if (!(left == 0 || (product / left) == right)) {
      UNIMPLEMENTED("small integer overflow");
    }
    return thread->runtime()->newInteger(product);
  }
  return thread->runtime()->notImplemented();
}

Object* SmallIntegerBuiltins::dunderNe(Thread* thread, Frame* frame,
                                       word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(frame, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isSmallInteger() && other->isSmallInteger()) {
    return Boolean::fromBool(self != other);
  }
  return thread->runtime()->notImplemented();
}

Object* SmallIntegerBuiltins::dunderNeg(Thread* thread, Frame* frame,
                                        word nargs) {
  if (nargs != 1) {
    return thread->throwTypeErrorFromCString("not enough arguments");
  }
  Arguments args(frame, nargs);
  SmallInteger* tos = SmallInteger::cast(args.get(0));
  return SmallInteger::fromWord(-tos->value());
}

Object* SmallIntegerBuiltins::dunderPos(Thread* thread, Frame* frame,
                                        word nargs) {
  if (nargs != 1) {
    return thread->throwTypeErrorFromCString("not enough arguments");
  }
  Arguments args(frame, nargs);
  return SmallInteger::cast(args.get(0));
}

Object* SmallIntegerBuiltins::dunderAdd(Thread* thread, Frame* caller,
                                        word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }

  Arguments args(caller, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);

  if (!self->isSmallInteger()) {
    return thread->throwTypeErrorFromCString(
        "__add__() must be called with int instance as first argument");
  }

  word left = SmallInteger::cast(self)->value();
  if (other->isInteger()) {
    word right = Integer::cast(other)->asWord();
    return thread->runtime()->newInteger(left + right);
  }
  return thread->runtime()->notImplemented();
}

Object* SmallIntegerBuiltins::dunderSub(Thread* thread, Frame* frame,
                                        word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }

  Arguments args(frame, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);

  if (!self->isSmallInteger()) {
    return thread->throwTypeErrorFromCString(
        "__sub__() must be called with int instance as first argument");
  }

  word left = SmallInteger::cast(self)->value();
  if (other->isInteger()) {
    word right = Integer::cast(other)->asWord();
    return thread->runtime()->newInteger(left - right);
  }
  return thread->runtime()->notImplemented();
}

Object* SmallIntegerBuiltins::dunderXor(Thread* thread, Frame* frame,
                                        word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(frame, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (!self->isSmallInteger()) {
    return thread->throwTypeErrorFromCString(
        "__xor__() must be called with int instance as first argument");
  }
  word left = SmallInteger::cast(self)->value();
  if (other->isInteger()) {
    word right = Integer::cast(other)->asWord();
    return thread->runtime()->newInteger(left ^ right);
  }
  return thread->runtime()->notImplemented();
}

}  // namespace python
