#include "object-builtins.h"

#include <cinttypes>

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"
#include "trampolines-inl.h"
#include "type-builtins.h"

namespace python {

RawObject objectGetAttribute(Thread* thread, const Object& object,
                             const Object& name_str) {
  // Look for the attribute in the class
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Type type(&scope, runtime->typeOf(*object));
  Object type_attr(&scope, typeLookupNameInMro(thread, type, name_str));
  if (!type_attr.isError()) {
    Type type_attr_type(&scope, runtime->typeOf(*type_attr));
    if (typeIsDataDescriptor(thread, type_attr_type)) {
      return Interpreter::callDescriptorGet(thread, thread->currentFrame(),
                                            type_attr, object, type);
    }
  }

  // No data descriptor found on the class, look at the instance.
  if (object.isHeapObject()) {
    HeapObject instance(&scope, *object);
    Object result(&scope, runtime->instanceAt(thread, instance, name_str));
    if (!result.isError()) {
      return *result;
    }
  }

  // Nothing found in the instance, if we found a non-data descriptor via the
  // class search, use it.
  if (!type_attr.isError()) {
    Type type_attr_type(&scope, runtime->typeOf(*type_attr));
    if (typeIsNonDataDescriptor(thread, type_attr_type)) {
      return Interpreter::callDescriptorGet(thread, thread->currentFrame(),
                                            type_attr, object, type);
    }

    // If a regular attribute was found in the class, return it
    return *type_attr;
  }

  return Error::notFound();
}

RawObject objectRaiseAttributeError(Thread* thread, const Object& object,
                                    const Object& name_str) {
  return thread->raiseWithFmt(LayoutId::kAttributeError,
                              "'%T' object has no attribute '%S'", &object,
                              &name_str);
}

RawObject objectSetAttr(Thread* thread, const Object& object,
                        const Object& name_interned_str, const Object& value) {
  Runtime* runtime = thread->runtime();
  DCHECK(runtime->isInternedStr(name_interned_str), "name must be interned");
  // Check for a data descriptor
  HandleScope scope(thread);
  Type type(&scope, runtime->typeOf(*object));
  Object type_attr(&scope,
                   typeLookupNameInMro(thread, type, name_interned_str));
  if (!type_attr.isError()) {
    Type type_attr_type(&scope, runtime->typeOf(*type_attr));
    if (typeIsDataDescriptor(thread, type_attr_type)) {
      Object set_result(
          &scope, Interpreter::callDescriptorSet(thread, thread->currentFrame(),
                                                 type_attr, object, value));
      if (set_result.isError()) return *set_result;
      return NoneType::object();
    }
  }

  // No data descriptor found, store on the instance
  if (object.isHeapObject()) {
    HeapObject instance(&scope, *object);
    return runtime->instanceAtPut(thread, instance, name_interned_str, value);
  }
  return objectRaiseAttributeError(thread, object, name_interned_str);
}

const BuiltinMethod ObjectBuiltins::kBuiltinMethods[] = {
    {SymbolId::kDunderGetattribute, dunderGetattribute},
    {SymbolId::kDunderHash, dunderHash},
    {SymbolId::kDunderInit, dunderInit},
    {SymbolId::kDunderNew, dunderNew},
    {SymbolId::kDunderSizeof, dunderSizeof},
    // no sentinel needed because the iteration below is manual
};

void ObjectBuiltins::initialize(Runtime* runtime) {
  HandleScope scope;

  Layout layout(&scope, runtime->newLayout());
  layout.setId(LayoutId::kObject);
  Type object_type(&scope, runtime->newType());
  layout.setDescribedType(*object_type);
  object_type.setName(runtime->symbols()->ObjectTypename());
  Tuple mro(&scope, runtime->newTuple(1));
  mro.atPut(0, *object_type);
  object_type.setMro(*mro);
  object_type.setInstanceLayout(*layout);
  object_type.setBases(runtime->newTuple(0));
  runtime->layoutAtPut(LayoutId::kObject, *layout);

  for (uword i = 0; i < ARRAYSIZE(kBuiltinMethods); i++) {
    runtime->typeAddBuiltinFunction(object_type, kBuiltinMethods[i].name,
                                    kBuiltinMethods[i].address);
  }

  postInitialize(runtime, object_type);
}

void ObjectBuiltins::postInitialize(Runtime* runtime, const Type& new_type) {
  // Manually set argcount to avoid bootstrap problems.
  HandleScope scope;
  Dict type_dict(&scope, new_type.dict());
  Object dunder_getattribute_name(&scope,
                                  runtime->symbols()->DunderGetattribute());
  Function dunder_getattribute(
      &scope, runtime->typeDictAt(type_dict, dunder_getattribute_name));
  Code code(&scope, dunder_getattribute.code());
  code.setArgcount(2);
}

RawObject ObjectBuiltins::dunderGetattribute(Thread* thread, Frame* frame,
                                             word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Object name(&scope, args.get(1));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfStr(*name)) {
    return thread->raiseWithFmt(
        LayoutId::kTypeError, "attribute name must be string, not '%T'", &name);
  }
  Object result(&scope, objectGetAttribute(thread, self, name));
  if (result.isErrorNotFound()) {
    return objectRaiseAttributeError(thread, self, name);
  }
  return *result;
}

RawObject ObjectBuiltins::dunderHash(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  return thread->runtime()->hash(args.get(0));
}

RawObject ObjectBuiltins::dunderInit(Thread* thread, Frame* frame, word nargs) {
  // Too many arguments were given. Determine if the __new__ was not overwritten
  // or the __init__ was to throw a TypeError.
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object self(&scope, args.get(0));
  Tuple starargs(&scope, args.get(1));
  Dict kwargs(&scope, args.get(2));
  if (starargs.length() == 0 && kwargs.numItems() == 0) {
    // object.__init__ doesn't do anything except throw a TypeError if the
    // wrong number of arguments are given. It only throws if __new__ is not
    // overloaded or __init__ was overloaded, else it allows the excess
    // arguments.
    return NoneType::object();
  }
  Type type(&scope, runtime->typeOf(*self));
  if (!runtime->isMethodOverloaded(thread, type, SymbolId::kDunderNew) ||
      runtime->isMethodOverloaded(thread, type, SymbolId::kDunderInit)) {
    // Throw a TypeError if extra arguments were passed, and __new__ was not
    // overwritten by self, or __init__ was overloaded by self.
    return thread->raiseTypeErrorWithCStr(
        "object.__init__() takes no parameters");
  }
  // Else it's alright to have extra arguments.
  return NoneType::object();
}

RawObject ObjectBuiltins::dunderNew(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  if (nargs < 1) {
    return thread->raiseTypeErrorWithCStr(
        "object.__new__() takes no arguments");
  }
  HandleScope scope(thread);
  Type type(&scope, args.get(0));
  Layout layout(&scope, type.instanceLayout());
  return thread->runtime()->newInstance(layout);
}

RawObject ObjectBuiltins::dunderNewKw(Thread* thread, Frame* frame,
                                      word nargs) {
  // This should really raise an error if __init__ is not overridden (see
  // https://hlrz.com/source/xref/cpython-3.6/Objects/typeobject.c#3428)
  // However, object.__new__ should also do that as well. For now, just forward
  // to __new__.
  KwArguments args(frame, nargs);
  return dunderNew(thread, frame, nargs - args.numKeywords() - 1);
}

RawObject ObjectBuiltins::dunderSizeof(Thread* thread, Frame* frame,
                                       word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object obj(&scope, args.get(0));
  if (obj.isHeapObject()) {
    HeapObject heap_obj(&scope, *obj);
    return SmallInt::fromWord(heap_obj.size());
  }
  return SmallInt::fromWord(kPointerSize);
}

const BuiltinMethod NoneBuiltins::kBuiltinMethods[] = {
    {SymbolId::kDunderNew, dunderNew},
    {SymbolId::kDunderRepr, dunderRepr},
    {SymbolId::kSentinelId, nullptr},
};

void NoneBuiltins::postInitialize(Runtime*, const Type& new_type) {
  new_type.setBuiltinBase(LayoutId::kNoneType);
}

RawObject NoneBuiltins::dunderNew(Thread*, Frame*, word) {
  return NoneType::object();
}

RawObject NoneBuiltins::dunderRepr(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  if (!args.get(0).isNoneType()) {
    return thread->raiseTypeErrorWithCStr(
        "__repr__ expects None as first argument");
  }
  return thread->runtime()->symbols()->None();
}

}  // namespace python
