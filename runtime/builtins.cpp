#include "builtins.h"

#include "bytecode.h"
#include "frame.h"
#include "globals.h"
#include "interpreter.h"
#include "mro.h"
#include "objects.h"
#include "os.h"
#include "runtime.h"
#include "thread.h"
#include "trampolines-inl.h"

namespace python {

Object* builtinTypeCall(Thread* thread, Frame* caller, word nargs) {
  HandleScope scope(thread->handles());

  // Create a frame big enough to hold all of the outgoing arguments and the
  // function object for the __new__ and __init__ calls.
  Frame* frame = thread->openAndLinkFrame(nargs, 0, nargs + 1);

  Arguments args(caller, nargs);

  Runtime* runtime = thread->runtime();
  Handle<Object> name(&scope, runtime->symbols()->DunderNew());

  // First, call __new__ to allocate a new instance.

  Handle<Class> type(&scope, args.get(0));
  Handle<Function> dunder_new(
      &scope, runtime->lookupNameInMro(thread, type, name));

  Object** sp = frame->valueStackTop();
  *--sp = *dunder_new;
  for (word i = 0; i < nargs; i++) {
    *--sp = args.get(i);
  }
  frame->setValueStackTop(sp);

  Handle<Object> result(&scope, dunder_new->entry()(thread, frame, nargs));

  // Pop all of the arguments we pushed for the __new__ call.  While we will
  // push the same number of arguments again for the __init__ call below,
  // starting over from scratch keeps the addressing arithmetic simple.
  frame->setValueStackTop(sp + nargs + 1);

  // Second, call __init__ to initialize the instance.

  // top of the stack should be the new instance
  Handle<Object> init(&scope, runtime->symbols()->DunderInit());
  Handle<Function> dunder_init(
      &scope, runtime->lookupNameInMro(thread, type, init));

  sp = frame->valueStackTop();
  *--sp = *dunder_init;
  *--sp = *result;
  for (word i = 1; i < nargs; i++) {
    *--sp = args.get(i);
  }
  frame->setValueStackTop(sp);

  dunder_init->entry()(thread, frame, nargs);

  // TODO: throw a type error if the __init__ method does not return None.

  thread->popFrame();

  return *result;
}

Object* builtinTypeNew(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Handle<Class> metatype(&scope, args.get(0));
  Handle<Object> name(&scope, args.get(1));
  Handle<Class> result(&scope, runtime->newClass());
  result->setName(*name);

  // Compute MRO
  Handle<ObjectArray> parents(&scope, args.get(2));
  Handle<Object> mro(&scope, computeMro(thread, result, parents));
  if (mro->isError()) {
    return *mro;
  }
  result->setMro(*mro);

  Handle<Dictionary> dictionary(&scope, args.get(3));
  result->setDictionary(*dictionary);

  // Initialize instance layout
  Handle<Layout> layout(&scope, runtime->computeInitialLayout(thread, result));
  layout->setDescribedClass(*result);
  result->setInstanceLayout(*layout);

  // Initialize builtin base class
  result->setBuiltinBaseClass(runtime->computeBuiltinBaseClass(result));
  Handle<Class> base(&scope, result->builtinBaseClass());
  Handle<Class> list(
      &scope, thread->runtime()->classAt(IntrinsicLayoutId::kList));
  if (Boolean::cast(thread->runtime()->isSubClass(base, list))->value()) {
    result->setFlag(Class::Flag::kListSubclass);
    layout->addDelegateSlot();
  }
  return *result;
}

Object* builtinTypeInit(Thread*, Frame*, word) {
  return None::object();
}

Object* builtinObjectInit(Thread* thread, Frame*, word nargs) {
  if (nargs != 1) {
    thread->throwTypeErrorFromCString("object.__init__() takes no arguments");
    return Error::object();
  }
  return None::object();
}

Object* builtinObjectNew(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  if (nargs < 1) {
    thread->throwTypeErrorFromCString("object.__new__() takes no arguments");
    return Error::object();
  }
  HandleScope scope(thread->handles());
  Handle<Class> klass(&scope, args.get(0));
  Handle<Layout> layout(&scope, klass->instanceLayout());
  return thread->runtime()->newInstance(layout);
}

static Object* listOrDelegate(Thread* thread, const Handle<Object>& instance) {
  if (instance->isList()) {
    return *instance;
  } else {
    HandleScope scope(thread);
    Handle<Class> klass(&scope, thread->runtime()->classOf(*instance));
    if (klass->hasFlag(Class::Flag::kListSubclass)) {
      return thread->runtime()->instanceDelegate(instance);
    }
  }
  return Error::object();
}

// Boolean

Object* builtinBooleanBool(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 1) {
    return thread->throwTypeErrorFromCString("not enough arguments");
  }
  Arguments args(caller, nargs);
  if (args.get(0)->isBoolean()) {
    return args.get(0);
  }
  return thread->throwTypeErrorFromCString("unsupported type for __bool__");
}

// Dictionary

Object* builtinDictionaryEq(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(caller, nargs);
  if (args.get(0)->isDictionary() && args.get(1)->isDictionary()) {
    HandleScope scope(thread->handles());
    Runtime* runtime = thread->runtime();

    Handle<Dictionary> self(&scope, args.get(0));
    Handle<Dictionary> other(&scope, args.get(1));
    if (self->numItems() != other->numItems()) {
      return Boolean::falseObj();
    }
    Handle<ObjectArray> keys(&scope, runtime->dictionaryKeys(self));
    Handle<Object> left_key(&scope, None::object());
    Handle<Object> left(&scope, None::object());
    Handle<Object> right(&scope, None::object());
    word length = keys->length();
    for (word i = 0; i < length; i++) {
      left_key = keys->at(i);
      left = runtime->dictionaryAt(self, left_key);
      right = runtime->dictionaryAt(other, left_key);
      if (right->isError()) {
        return Boolean::falseObj();
      }
      Object* result = Interpreter::compareOperation(
          thread, caller, caller->valueStackTop(), EQ, left, right);
      if (result == Boolean::falseObj()) {
        return result;
      }
    }
    return Boolean::trueObj();
  }
  // TODO(cshapiro): handle user-defined subtypes of dictionary.
  return thread->runtime()->notImplemented();
}

// Double

Object* builtinDoubleEq(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(caller, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isDouble() && other->isDouble()) {
    Double* left = Double::cast(self);
    Double* right = Double::cast(other);
    return Boolean::fromBool(left->value() == right->value());
  } else if (self->isInteger() || other->isInteger()) {
    UNIMPLEMENTED("integer to float conversion");
  }
  return thread->runtime()->notImplemented();
}

Object* builtinDoubleGe(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(caller, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isDouble() && other->isDouble()) {
    Double* left = Double::cast(self);
    Double* right = Double::cast(other);
    return Boolean::fromBool(left->value() >= right->value());
  } else if (self->isInteger() || other->isInteger()) {
    UNIMPLEMENTED("integer to float conversion");
  }
  return thread->runtime()->notImplemented();
}

Object* builtinDoubleGt(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(caller, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isDouble() && other->isDouble()) {
    Double* left = Double::cast(self);
    Double* right = Double::cast(other);
    return Boolean::fromBool(left->value() > right->value());
  } else if (self->isInteger() || other->isInteger()) {
    UNIMPLEMENTED("integer to float conversion");
  }
  return thread->runtime()->notImplemented();
}

Object* builtinDoubleLe(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(caller, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isDouble() && other->isDouble()) {
    Double* left = Double::cast(self);
    Double* right = Double::cast(other);
    return Boolean::fromBool(left->value() <= right->value());
  } else if (self->isInteger() || other->isInteger()) {
    UNIMPLEMENTED("integer to float conversion");
  }
  return thread->runtime()->notImplemented();
}

Object* builtinDoubleLt(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(caller, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isDouble() && other->isDouble()) {
    Double* left = Double::cast(self);
    Double* right = Double::cast(other);
    return Boolean::fromBool(left->value() < right->value());
  } else if (self->isInteger() || other->isInteger()) {
    UNIMPLEMENTED("integer to float conversion");
  }
  return thread->runtime()->notImplemented();
}

Object* builtinDoubleNe(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(caller, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isDouble() && other->isDouble()) {
    Double* left = Double::cast(self);
    Double* right = Double::cast(other);
    return Boolean::fromBool(left->value() != right->value());
  } else if (self->isInteger() || other->isInteger()) {
    UNIMPLEMENTED("integer to float conversion");
  }
  return thread->runtime()->notImplemented();
}

// List

Object* builtinListNew(Thread* thread, Frame* caller, word nargs) {
  if (nargs < 1) {
    return thread->throwTypeErrorFromCString("not enough arguments");
  }
  Arguments args(caller, nargs);
  if (!args.get(0)->isClass()) {
    thread->throwTypeErrorFromCString("not a type object");
  }
  HandleScope scope(thread->handles());
  Handle<Class> type(&scope, args.get(0));
  Handle<Layout> layout(&scope, type->instanceLayout());
  if (layout->id() == IntrinsicLayoutId::kList) {
    return thread->runtime()->newList();
  }
  CHECK(layout->hasDelegateSlot(), "must have a delegate slot");
  Handle<Object> result(&scope, thread->runtime()->newInstance(layout));
  Handle<Object> delegate(&scope, thread->runtime()->newList());
  thread->runtime()->setInstanceDelegate(result, delegate);
  return *result;
}

Object* builtinListAppend(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString(
        "append() takes exactly one argument");
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Handle<Object> self(&scope, args.get(0));
  Handle<Object> list_or_error(&scope, listOrDelegate(thread, self));
  if (list_or_error->isError()) {
    return thread->throwTypeErrorFromCString(
        "append() only support list or its subclasses");
  }
  Handle<List> list(&scope, *list_or_error);
  Handle<Object> value(&scope, args.get(1));
  thread->runtime()->listAdd(list, value);
  return None::object();
}

Object* builtinListLen(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->throwTypeErrorFromCString("__len__() takes no arguments");
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Handle<Object> self(&scope, args.get(0));
  Handle<Object> list_or_error(&scope, listOrDelegate(thread, self));
  if (list_or_error->isError()) {
    return thread->throwTypeErrorFromCString(
        "__len__() only support list or its subclasses");
  }
  Handle<List> list(&scope, *list_or_error);
  return SmallInteger::fromWord(list->allocated());
}

Object* builtinListInsert(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 3) {
    return thread->throwTypeErrorFromCString(
        "insert() takes exactly two arguments");
  }
  Arguments args(frame, nargs);
  if (!args.get(1)->isInteger()) {
    return thread->throwTypeErrorFromCString(
        "index object cannot be interpreted as an integer");
  }

  HandleScope scope(thread);
  Handle<Object> self(&scope, args.get(0));
  Handle<Object> list_or_error(&scope, listOrDelegate(thread, self));
  if (list_or_error->isError()) {
    return thread->throwTypeErrorFromCString(
        "descriptor 'insert' requires a 'list' object");
  }
  Handle<List> list(&scope, *list_or_error);
  word index = SmallInteger::cast(args.get(1))->value();
  Handle<Object> value(&scope, args.get(2));
  thread->runtime()->listInsert(list, value, index);
  return None::object();
}

Object* builtinListPop(Thread* thread, Frame* frame, word nargs) {
  if (nargs > 2) {
    return thread->throwTypeErrorFromCString("pop() takes at most 1 argument");
  }
  Arguments args(frame, nargs);
  if (nargs == 2 && !args.get(1)->isSmallInteger()) {
    return thread->throwTypeErrorFromCString(
        "index object cannot be interpreted as an integer");
  }

  HandleScope scope(thread);
  Handle<Object> self(&scope, args.get(0));
  Handle<Object> list_or_error(&scope, listOrDelegate(thread, self));
  if (list_or_error->isError()) {
    return thread->throwTypeErrorFromCString(
        "descriptor 'pop' requires a 'list' object");
  }
  Handle<List> list(&scope, *list_or_error);
  word index = list->allocated() - 1;
  if (nargs == 2) {
    word last_index = index;
    index = SmallInteger::cast(args.get(1))->value();
    index = index < 0 ? last_index + index : index;
    // Pop out of bounds
    if (index > last_index) {
      // TODO(T27365047): Throw an IndexError exception
      UNIMPLEMENTED("Throw an IndexError for an out of range list index.");
    }
  }
  // Pop empty, or negative out of bounds
  if (index < 0) {
    // TODO(T27365047): Throw an IndexError exception
    UNIMPLEMENTED("Throw an IndexError for an out of range list index.");
  }

  return thread->runtime()->listPop(list, index);
}

Object* builtinListRemove(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString(
        "remove() takes exactly one argument");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Handle<Object> self(&scope, args.get(0));
  Handle<Object> value(&scope, args.get(1));
  Handle<Object> list_or_error(&scope, listOrDelegate(thread, self));
  if (list_or_error->isError()) {
    return thread->throwTypeErrorFromCString(
        "descriptor 'remove' requires a 'list' object");
  }
  Handle<List> list(&scope, *list_or_error);
  for (word i = 0; i < list->allocated(); i++) {
    Handle<Object> item(&scope, list->at(i));
    if (Boolean::cast(
            Interpreter::compareOperation(
                thread, frame, frame->valueStackTop(), EQ, item, value))
            ->value()) {
      thread->runtime()->listPop(list, i);
      return None::object();
    }
  }
  return thread->throwValueErrorFromCString("list.remove(x) x not in list");
}

// Descriptor
Object* functionDescriptorGet(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 3) {
    return thread->throwTypeErrorFromCString("__get__ needs 3 arguments");
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Handle<Object> self(&scope, args.get(0));
  Handle<Object> instance(&scope, args.get(1));
  if (instance->isNone()) {
    return *self;
  }
  return thread->runtime()->newBoundMethod(self, instance);
}

Object* classmethodDescriptorGet(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 3) {
    return thread->throwTypeErrorFromCString("__get__ needs 3 arguments");
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Handle<Object> self(&scope, args.get(0));
  Handle<Object> owner(&scope, args.get(2));

  Handle<Object> method(&scope, ClassMethod::cast(*self)->function());
  return thread->runtime()->newBoundMethod(method, owner);
}

Object* staticmethodDescriptorGet(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 3) {
    return thread->throwTypeErrorFromCString("__get__ needs 3 arguments");
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Handle<Object> self(&scope, args.get(0));

  return StaticMethod::cast(*self)->function();
}

// ClassMethod
Object* builtinClassMethodNew(Thread* thread, Frame*, word) {
  return thread->runtime()->newClassMethod();
}

Object* builtinClassMethodInit(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString(
        "classmethod expected 1 arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Handle<ClassMethod> classmethod(&scope, args.get(0));
  Handle<Object> arg(&scope, args.get(1));
  classmethod->setFunction(*arg);
  return *classmethod;
}

// SmallInteger

Object* builtinSmallIntegerBool(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 1) {
    return thread->throwTypeErrorFromCString("not enough arguments");
  }
  Arguments args(caller, nargs);
  if (args.get(0)->isSmallInteger()) {
    return Boolean::fromBool(args.get(0) != SmallInteger::fromWord(0));
  }
  return thread->throwTypeErrorFromCString("unsupported type for __bool__");
}

Object* builtinSmallIntegerEq(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(caller, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isSmallInteger() && other->isSmallInteger()) {
    return Boolean::fromBool(self == other);
  }
  return thread->runtime()->notImplemented();
}

Object* builtinSmallIntegerInvert(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 1) {
    return thread->throwTypeErrorFromCString("not enough arguments");
  }
  Arguments args(caller, nargs);
  if (args.get(0)->isSmallInteger()) {
    SmallInteger* tos = SmallInteger::cast(args.get(0));
    return SmallInteger::fromWord(-(tos->value() + 1));
  }
  return thread->throwTypeErrorFromCString("unsupported type for __invert__");
}

Object* builtinSmallIntegerLe(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(caller, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isSmallInteger() && other->isSmallInteger()) {
    SmallInteger* left = SmallInteger::cast(self);
    SmallInteger* right = SmallInteger::cast(other);
    return Boolean::fromBool(left->value() <= right->value());
  }
  return thread->runtime()->notImplemented();
}

Object* builtinSmallIntegerLt(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(caller, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isSmallInteger() && other->isSmallInteger()) {
    SmallInteger* left = SmallInteger::cast(self);
    SmallInteger* right = SmallInteger::cast(other);
    return Boolean::fromBool(left->value() < right->value());
  }
  return thread->runtime()->notImplemented();
}

Object* builtinSmallIntegerGe(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(caller, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isSmallInteger() && other->isSmallInteger()) {
    SmallInteger* left = SmallInteger::cast(self);
    SmallInteger* right = SmallInteger::cast(other);
    return Boolean::fromBool(left->value() >= right->value());
  }
  return thread->runtime()->notImplemented();
}

Object* builtinSmallIntegerGt(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(caller, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isSmallInteger() && other->isSmallInteger()) {
    SmallInteger* left = SmallInteger::cast(self);
    SmallInteger* right = SmallInteger::cast(other);
    return Boolean::fromBool(left->value() > right->value());
  }
  return thread->runtime()->notImplemented();
}

Object* builtinSmallIntegerNe(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(caller, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isSmallInteger() && other->isSmallInteger()) {
    return Boolean::fromBool(self != other);
  }
  return thread->runtime()->notImplemented();
}

Object* builtinSmallIntegerNeg(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 1) {
    return thread->throwTypeErrorFromCString("not enough arguments");
  }
  Arguments args(caller, nargs);
  SmallInteger* tos = SmallInteger::cast(args.get(0));
  return SmallInteger::fromWord(-tos->value());
}

Object* builtinSmallIntegerPos(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 1) {
    return thread->throwTypeErrorFromCString("not enough arguments");
  }
  return SmallInteger::cast(*caller->valueStackTop());
}

// StaticMethod
Object* builtinStaticMethodNew(Thread* thread, Frame*, word) {
  return thread->runtime()->newStaticMethod();
}

Object* builtinStaticMethodInit(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString(
        "staticmethod expected 1 arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Handle<StaticMethod> staticmethod(&scope, args.get(0));
  Handle<Object> arg(&scope, args.get(1));
  staticmethod->setFunction(*arg);
  return *staticmethod;
}

// String

Object* builtinStringEq(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(caller, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isString() && other->isString()) {
    return Boolean::fromBool(String::cast(self)->compare(other) == 0);
  }
  // TODO(cshapiro): handle user-defined subtypes of string.
  return thread->runtime()->notImplemented();
}

Object* builtinStringGe(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(caller, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isString() && other->isString()) {
    return Boolean::fromBool(String::cast(self)->compare(other) >= 0);
  }
  // TODO(cshapiro): handle user-defined subtypes of string.
  return thread->runtime()->notImplemented();
}

Object* builtinStringGt(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(caller, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isString() && other->isString()) {
    return Boolean::fromBool(String::cast(self)->compare(other) > 0);
  }
  // TODO(cshapiro): handle user-defined subtypes of string.
  return thread->runtime()->notImplemented();
}

Object* builtinStringLe(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(caller, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isString() && other->isString()) {
    return Boolean::fromBool(String::cast(self)->compare(other) <= 0);
  }
  // TODO(cshapiro): handle user-defined subtypes of string.
  return thread->runtime()->notImplemented();
}

Object* builtinStringLt(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(caller, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isString() && other->isString()) {
    return Boolean::fromBool(String::cast(self)->compare(other) < 0);
  }
  // TODO(cshapiro): handle user-defined subtypes of string.
  return thread->runtime()->notImplemented();
}

Object* builtinStringNe(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(caller, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isString() && other->isString()) {
    return Boolean::fromBool(String::cast(self)->compare(other) != 0);
  }
  // TODO(cshapiro): handle user-defined subtypes of string.
  return thread->runtime()->notImplemented();
}

// Super

Object* builtinSuperNew(Thread* thread, Frame*, word) {
  return thread->runtime()->newSuper();
}

Object* builtinSuperInit(Thread* thread, Frame* frame, word nargs) {
  // only support idiomatic usage for now
  // super(type, obj) -> bound super object; requires isinstance(obj, type)
  // super(type, type2) -> bound super object; requires issubclass(type2, type)
  if (nargs != 3) {
    return thread->throwTypeErrorFromCString("super() expected 2 arguments");
  }
  Arguments args(frame, nargs);
  if (!args.get(1)->isClass()) {
    return thread->throwTypeErrorFromCString("super() argument 1 must be type");
  }
  HandleScope scope(thread);
  Handle<Super> super(&scope, args.get(0));
  Handle<Class> klass(&scope, args.get(1));
  Handle<Object> obj(&scope, args.get(2));
  super->setType(*klass);
  super->setObject(*obj);
  Handle<Object> obj_type(&scope, None::object());
  if (obj->isClass()) {
    Handle<Class> obj_klass(&scope, *obj);
    if (Boolean::cast(thread->runtime()->isSubClass(obj_klass, klass))
            ->value()) {
      obj_type = *obj;
    }
  } else {
    Handle<Class> obj_klass(&scope, thread->runtime()->classOf(*obj));
    if (Boolean::cast(thread->runtime()->isSubClass(obj_klass, klass))
            ->value()) {
      obj_type = *obj_klass;
    }
    // Fill in the __class__ case
  }
  if (obj_type->isNone()) {
    return thread->throwTypeErrorFromCString(
        "obj must be an instance or subtype of type");
  }
  super->setObjectType(*obj_type);
  return *super;
}

// Tuple

Object* builtinTupleEq(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(caller, nargs);
  if (args.get(0)->isObjectArray() && args.get(1)->isObjectArray()) {
    HandleScope scope(thread->handles());
    Handle<ObjectArray> self(&scope, args.get(0));
    Handle<ObjectArray> other(&scope, args.get(1));
    if (self->length() != other->length()) {
      return Boolean::falseObj();
    }
    Handle<Object> left(&scope, None::object());
    Handle<Object> right(&scope, None::object());
    word length = self->length();
    for (word i = 0; i < length; i++) {
      left = self->at(i);
      right = other->at(i);
      Object* result = Interpreter::compareOperation(
          thread, caller, caller->valueStackTop(), EQ, left, right);
      if (result == Boolean::falseObj()) {
        return result;
      }
    }
    return Boolean::trueObj();
  }
  // TODO(cshapiro): handle user-defined subtypes of tuple.
  return thread->runtime()->notImplemented();
}

} // namespace python
