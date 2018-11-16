#include "object-builtins.h"

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"
#include "trampolines-inl.h"

namespace python {

const BuiltinMethod ObjectBuiltins::kMethods[] = {
    {SymbolId::kDunderHash, nativeTrampoline<dunderHash>},
    {SymbolId::kDunderInit, nativeTrampoline<dunderInit>},
    {SymbolId::kDunderNew, nativeTrampoline<dunderNew>},
    {SymbolId::kDunderRepr, nativeTrampoline<dunderRepr>}};

void ObjectBuiltins::initialize(Runtime* runtime) {
  HandleScope scope;

  Handle<Layout> layout(&scope, runtime->newLayout());
  layout->setId(LayoutId::kObject);
  Handle<Type> object_type(&scope, runtime->newClass());
  layout->setDescribedClass(*object_type);
  object_type->setName(runtime->symbols()->ObjectClassname());
  Handle<ObjectArray> mro(&scope, runtime->newObjectArray(1));
  mro->atPut(0, *object_type);
  object_type->setMro(*mro);
  object_type->setInstanceLayout(*layout);
  runtime->layoutAtPut(LayoutId::kObject, *layout);

  for (uword i = 0; i < ARRAYSIZE(kMethods); i++) {
    runtime->classAddBuiltinFunction(object_type, kMethods[i].name,
                                     kMethods[i].address);
  }
}

Object* ObjectBuiltins::dunderHash(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->throwTypeErrorFromCString(
        "object.__hash__() takes no arguments");
  }
  Arguments args(frame, nargs);
  return thread->runtime()->hash(args.get(0));
}

Object* ObjectBuiltins::dunderInit(Thread* thread, Frame* frame, word nargs) {
  // object.__init__ doesn't do anything except throw a TypeError if the wrong
  // number of arguments are given. It only throws if __new__ is not overloaded
  // or __init__ was overloaded, else it allows the excess arguments.
  if (nargs == 0) {
    return thread->throwTypeErrorFromCString("__init__ needs an argument");
  }
  if (nargs == 1) {
    return None::object();
  }
  // Too many arguments were given. Determine if the __new__ was not overwritten
  // or the __init__ was to throw a TypeError.
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Handle<Object> self(&scope, args.get(0));
  Handle<Type> type(&scope, runtime->typeOf(*self));
  if (!runtime->isMethodOverloaded(thread, type, SymbolId::kDunderNew) ||
      runtime->isMethodOverloaded(thread, type, SymbolId::kDunderInit)) {
    // Throw a TypeError if extra arguments were passed, and __new__ was not
    // overwritten by self, or __init__ was overloaded by self.
    return thread->throwTypeErrorFromCString(
        "object.__init__() takes no parameters");
  }
  // Else it's alright to have extra arguments.
  return None::object();
}

Object* ObjectBuiltins::dunderNew(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  if (nargs < 1) {
    return thread->throwTypeErrorFromCString(
        "object.__new__() takes no arguments");
  }
  HandleScope scope(thread);
  Handle<Type> type(&scope, args.get(0));
  Handle<Layout> layout(&scope, type->instanceLayout());
  return thread->runtime()->newInstance(layout);
}

Object* ObjectBuiltins::dunderRepr(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->throwTypeErrorFromCString("expected 0 arguments");
  }
  Arguments args(frame, nargs);

  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Handle<Object> self(&scope, args.get(0));

  // TODO(T31727304): Get the module and qualified subname. For now settle for
  // the class name.
  Handle<Type> type(&scope, runtime->typeOf(*self));
  Handle<String> type_name(&scope, type->name());
  char* c_string = type_name->toCString();
  Object* str = thread->runtime()->newStringFromFormat(
      "<%s object at %p>", c_string, static_cast<void*>(*self));
  free(c_string);
  return str;
}

const BuiltinMethod NoneBuiltins::kMethods[] = {
    {SymbolId::kDunderNew, nativeTrampoline<dunderNew>},
};

void NoneBuiltins::initialize(Runtime* runtime) {
  HandleScope scope;
  Handle<Type> type(
      &scope, runtime->addEmptyBuiltinClass(
                  SymbolId::kNoneType, LayoutId::kNoneType, LayoutId::kObject));
  for (uword i = 0; i < ARRAYSIZE(kMethods); i++) {
    runtime->classAddBuiltinFunction(type, kMethods[i].name,
                                     kMethods[i].address);
  }
}

Object* NoneBuiltins::dunderNew(Thread* thread, Frame*, word nargs) {
  if (nargs > 1) {
    return thread->throwTypeErrorFromCString("None.__new__ takes no arguments");
  }
  return None::object();
}

}  // namespace python
