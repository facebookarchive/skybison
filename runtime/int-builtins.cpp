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
};

void IntBuiltins::initialize(Runtime* runtime) {
  HandleScope scope;
  Handle<Type> type(
      &scope, runtime->addEmptyBuiltinClass(SymbolId::kInt, LayoutId::kInt,
                                            LayoutId::kObject));
  type->setFlag(Type::Flag::kIntSubclass);
  for (uword i = 0; i < ARRAYSIZE(kMethods); i++) {
    runtime->classAddBuiltinFunction(type, kMethods[i].name,
                                     kMethods[i].address);
  }
}

Object* IntBuiltins::dunderNew(Thread* thread, Frame* frame, word nargs) {
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
  if (!runtime->hasSubClassFlag(*type_obj, Type::Flag::kTypeSubclass)) {
    return thread->throwTypeErrorFromCString(
        "int.__new__(X): X is not a type object");
  }

  Handle<Type> type(&scope, *type_obj);
  if (!type->hasFlag(Type::Flag::kIntSubclass)) {
    return thread->throwTypeErrorFromCString(
        "int.__new__(X): X is not a subtype of int");
  }

  Handle<Layout> layout(&scope, type->instanceLayout());
  if (layout->id() != LayoutId::kInt) {
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
  if (!base->isInt()) {
    // TODO(dulinr): Call __index__ on base to convert it.
    UNIMPLEMENTED("Can't handle non-integer base");
  }
  return intFromString(thread, *arg, Int::cast(*base)->asWord());
}

Object* IntBuiltins::intFromString(Thread* thread, Object* arg_raw, int base) {
  if (!(base == 0 || (base >= 2 && base <= 36))) {
    return thread->throwValueErrorFromCString(
        "Invalid base, must be between 2 and 36, or 0");
  }
  HandleScope scope(thread);
  Handle<Object> arg(&scope, arg_raw);
  if (arg->isInt()) {
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
  } else if (!SmallInt::isValid(res)) {
    return thread->throwValueErrorFromCString("unsupported type");
  }
  return SmallInt::fromWord(res);
}

Object* IntBuiltins::dunderInt(Thread* thread, Frame* frame, word nargs) {
  if (nargs < 1) {
    return thread->throwTypeErrorFromCString("not enough arguments");
  }
  if (nargs > 1) {
    return thread->throwTypeError(thread->runtime()->newStringFromFormat(
        "expected 0 arguments, %ld given", nargs - 1));
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Handle<Object> arg(&scope, args.get(0));
  if (arg->isInt()) {
    return *arg;
  }
  if (thread->runtime()->hasSubClassFlag(*arg, Type::Flag::kIntSubclass)) {
    UNIMPLEMENTED("Strict subclass of int");
  }
  return thread->throwTypeErrorFromCString(
      "object cannot be interpreted as an integer");
}

const BuiltinMethod SmallIntBuiltins::kMethods[] = {
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

void SmallIntBuiltins::initialize(Runtime* runtime) {
  HandleScope scope;
  Handle<Type> type(
      &scope, runtime->addEmptyBuiltinClass(
                  SymbolId::kSmallInt, LayoutId::kSmallInt, LayoutId::kInt));
  for (uword i = 0; i < ARRAYSIZE(kMethods); i++) {
    runtime->classAddBuiltinFunction(type, kMethods[i].name,
                                     kMethods[i].address);
  }
  // We want to lookup the class of an immediate type by using the 5-bit tag
  // value as an index into the class table.  Replicate the class object for
  // SmallInt to all locations that decode to a SmallInt tag.
  for (word i = 1; i < 16; i++) {
    DCHECK(runtime->layoutAt(static_cast<LayoutId>(i << 1)) == None::object(),
           "list collision");
    runtime->layoutAtPut(static_cast<LayoutId>(i << 1), *type);
  }
}

Object* SmallIntBuiltins::bitLength(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(frame, nargs);
  Object* self = args.get(0);
  if (!self->isSmallInt()) {
    return thread->throwTypeErrorFromCString(
        "bit_length() must be called with int instance as first argument");
  }
  uword number = static_cast<uword>(std::abs(SmallInt::cast(self)->value()));
  return SmallInt::fromWord(Utils::highestBit(number));
}

Object* SmallIntBuiltins::dunderBool(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->throwTypeErrorFromCString("not enough arguments");
  }
  Arguments args(frame, nargs);
  if (args.get(0)->isSmallInt()) {
    return Bool::fromBool(args.get(0) != SmallInt::fromWord(0));
  }
  return thread->throwTypeErrorFromCString("unsupported type for __bool__");
}

Object* SmallIntBuiltins::dunderEq(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(frame, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isSmallInt() && other->isSmallInt()) {
    return Bool::fromBool(self == other);
  }
  return thread->runtime()->notImplemented();
}

Object* SmallIntBuiltins::dunderInvert(Thread* thread, Frame* frame,
                                       word nargs) {
  if (nargs != 1) {
    return thread->throwTypeErrorFromCString("not enough arguments");
  }
  Arguments args(frame, nargs);
  if (args.get(0)->isSmallInt()) {
    SmallInt* tos = SmallInt::cast(args.get(0));
    return SmallInt::fromWord(-(tos->value() + 1));
  }
  return thread->throwTypeErrorFromCString("unsupported type for __invert__");
}

Object* SmallIntBuiltins::dunderLe(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(frame, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isSmallInt() && other->isSmallInt()) {
    SmallInt* left = SmallInt::cast(self);
    SmallInt* right = SmallInt::cast(other);
    return Bool::fromBool(left->value() <= right->value());
  }
  return thread->runtime()->notImplemented();
}

Object* SmallIntBuiltins::dunderFloorDiv(Thread* thread, Frame* caller,
                                         word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(caller, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (!self->isSmallInt()) {
    return thread->throwTypeErrorFromCString(
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

Object* SmallIntBuiltins::dunderLt(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(frame, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isSmallInt() && other->isSmallInt()) {
    SmallInt* left = SmallInt::cast(self);
    SmallInt* right = SmallInt::cast(other);
    return Bool::fromBool(left->value() < right->value());
  }
  return thread->runtime()->notImplemented();
}

Object* SmallIntBuiltins::dunderGe(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(frame, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isSmallInt() && other->isSmallInt()) {
    SmallInt* left = SmallInt::cast(self);
    SmallInt* right = SmallInt::cast(other);
    return Bool::fromBool(left->value() >= right->value());
  }
  return thread->runtime()->notImplemented();
}

Object* SmallIntBuiltins::dunderGt(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(frame, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isSmallInt() && other->isSmallInt()) {
    SmallInt* left = SmallInt::cast(self);
    SmallInt* right = SmallInt::cast(other);
    return Bool::fromBool(left->value() > right->value());
  }
  return thread->runtime()->notImplemented();
}

Object* SmallIntBuiltins::dunderMod(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(caller, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (!self->isSmallInt()) {
    return thread->throwTypeErrorFromCString(
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
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(caller, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (!self->isSmallInt()) {
    return thread->throwTypeErrorFromCString(
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

Object* SmallIntBuiltins::dunderNe(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(frame, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isSmallInt() && other->isSmallInt()) {
    return Bool::fromBool(self != other);
  }
  return thread->runtime()->notImplemented();
}

Object* SmallIntBuiltins::dunderNeg(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->throwTypeErrorFromCString("not enough arguments");
  }
  Arguments args(frame, nargs);
  SmallInt* tos = SmallInt::cast(args.get(0));
  return SmallInt::fromWord(-tos->value());
}

Object* SmallIntBuiltins::dunderPos(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->throwTypeErrorFromCString("not enough arguments");
  }
  Arguments args(frame, nargs);
  return SmallInt::cast(args.get(0));
}

Object* SmallIntBuiltins::dunderAdd(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }

  Arguments args(caller, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);

  if (!self->isSmallInt()) {
    return thread->throwTypeErrorFromCString(
        "__add__() must be called with int instance as first argument");
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
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }

  Arguments args(frame, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);

  if (!self->isSmallInt()) {
    return thread->throwTypeErrorFromCString(
        "__sub__() must be called with int instance as first argument");
  }

  word left = SmallInt::cast(self)->value();
  if (other->isInt()) {
    word right = Int::cast(other)->asWord();
    return thread->runtime()->newInt(left - right);
  }
  return thread->runtime()->notImplemented();
}

Object* SmallIntBuiltins::dunderXor(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(frame, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (!self->isSmallInt()) {
    return thread->throwTypeErrorFromCString(
        "__xor__() must be called with int instance as first argument");
  }
  word left = SmallInt::cast(self)->value();
  if (other->isInt()) {
    word right = Int::cast(other)->asWord();
    return thread->runtime()->newInt(left ^ right);
  }
  return thread->runtime()->notImplemented();
}

}  // namespace python
