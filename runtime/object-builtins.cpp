#include "object-builtins.h"

#include <cinttypes>

#include "frame.h"
#include "globals.h"
#include "ic.h"
#include "module-builtins.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"
#include "type-builtins.h"

namespace python {

RawObject objectRaiseAttributeError(Thread* thread, const Object& object,
                                    const Object& name_str) {
  return thread->raiseWithFmt(LayoutId::kAttributeError,
                              "'%T' object has no attribute '%S'", &object,
                              &name_str);
}

static RawObject instanceGetAttributeSetLocation(Thread* thread,
                                                 const HeapObject& object,
                                                 const Object& name_str,
                                                 Object* location_out) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Layout layout(&scope, runtime->layoutAt(object.layoutId()));
  AttributeInfo info;
  if (runtime->layoutFindAttribute(thread, layout, name_str, &info)) {
    if (info.isInObject()) {
      if (location_out != nullptr) {
        *location_out = RawSmallInt::fromWord(info.offset());
      }
      return object.instanceVariableAt(info.offset());
    }
    Tuple overflow(&scope, object.instanceVariableAt(layout.overflowOffset()));
    if (location_out != nullptr) {
      *location_out = RawSmallInt::fromWord(-info.offset() - 1);
    }
    return overflow.at(info.offset());
  }
  if (runtime->layoutHasDictOverflow(layout)) {
    Dict overflow(&scope,
                  runtime->layoutGetOverflowDict(thread, object, layout));
    Object obj(&scope, runtime->dictAt(thread, overflow, name_str));
    if (obj.isValueCell()) {
      obj = ValueCell::cast(*obj).value();
    }
    return *obj;
  }
  return Error::notFound();
}

RawObject instanceGetAttribute(Thread* thread, const HeapObject& object,
                               const Object& name_str) {
  return instanceGetAttributeSetLocation(thread, object, name_str, nullptr);
}

static RawObject instanceSetAttrSetLocation(Thread* thread,
                                            const HeapObject& instance,
                                            const Object& name_interned,
                                            const Object& value,
                                            Object* location_out) {
  HandleScope scope(thread);

  // If the attribute doesn't exist we'll need to transition the layout
  bool has_new_layout_id = false;
  Runtime* runtime = thread->runtime();
  Layout layout(&scope, runtime->layoutAt(instance.layoutId()));
  AttributeInfo info;
  if (!runtime->layoutFindAttribute(thread, layout, name_interned, &info)) {
    if (layout.isSealed()) {
      return thread->raiseWithFmt(
          LayoutId::kAttributeError,
          "Cannot set attribute '%S' on sealed class '%T'", &name_interned,
          &instance);
    }
    // Transition the layout
    layout = runtime->layoutAddAttribute(thread, layout, name_interned, 0);
    has_new_layout_id = true;

    bool found =
        runtime->layoutFindAttribute(thread, layout, name_interned, &info);
    DCHECK(found, "couldn't find attribute on new layout");
  }

  if (info.isReadOnly()) {
    return thread->raiseWithFmt(LayoutId::kAttributeError,
                                "'%T.%S' attribute is read-only", &instance,
                                &name_interned);
  }

  DCHECK(!instance.isType(), "must not cache type attributes");
  // Store the attribute
  if (info.isInObject()) {
    instance.instanceVariableAtPut(info.offset(), *value);
    if (location_out != nullptr) {
      *location_out = SmallInt::fromWord(info.offset());
    }
  } else {
    // Build the new overflow array
    Tuple old_overflow(&scope,
                       instance.instanceVariableAt(layout.overflowOffset()));
    word old_len = old_overflow.length();
    MutableTuple new_overflow(&scope, runtime->newMutableTuple(old_len + 1));
    new_overflow.replaceFromWith(0, *old_overflow, old_len);
    new_overflow.atPut(info.offset(), *value);
    instance.instanceVariableAtPut(layout.overflowOffset(),
                                   new_overflow.becomeImmutable());
    if (location_out != nullptr) {
      *location_out = SmallInt::fromWord(-info.offset() - 1);
    }
  }

  if (has_new_layout_id) {
    instance.setHeader(instance.header().withLayoutId(layout.id()));
  }

  return NoneType::object();
}

RawObject instanceSetAttr(Thread* thread, const HeapObject& instance,
                          const Object& name_interned, const Object& value) {
  return instanceSetAttrSetLocation(thread, instance, name_interned, value,
                                    nullptr);
}

RawObject objectGetAttributeSetLocation(Thread* thread, const Object& object,
                                        const Object& name_str,
                                        Object* location_out) {
  // Look for the attribute in the class
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Type type(&scope, runtime->typeOf(*object));
  Object type_attr(&scope, typeLookupInMro(thread, type, name_str));
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
    Object result(&scope, instanceGetAttributeSetLocation(
                              thread, instance, name_str, location_out));
    if (!result.isError()) {
      return *result;
    }
  }

  // Nothing found in the instance, if we found a non-data descriptor via the
  // class search, use it.
  if (!type_attr.isError()) {
    if (type_attr.isFunction()) {
      if (location_out != nullptr) *location_out = *type_attr;
      return runtime->newBoundMethod(type_attr, object);
    }

    return resolveDescriptorGet(thread, type_attr, object, type);
  }

  return Error::notFound();
}

RawObject objectGetAttribute(Thread* thread, const Object& object,
                             const Object& name_str) {
  return objectGetAttributeSetLocation(thread, object, name_str, nullptr);
}

RawObject objectSetAttrSetLocation(Thread* thread, const Object& object,
                                   const Object& name_interned_str,
                                   const Object& value, Object* location_out) {
  Runtime* runtime = thread->runtime();
  DCHECK(runtime->isInternedStr(thread, name_interned_str),
         "name must be interned");
  // Check for a data descriptor
  HandleScope scope(thread);
  Type type(&scope, runtime->typeOf(*object));
  Object type_attr(&scope, typeLookupInMro(thread, type, name_interned_str));
  if (!type_attr.isError()) {
    Type type_attr_type(&scope, runtime->typeOf(*type_attr));
    if (typeIsDataDescriptor(thread, type_attr_type)) {
      // Do not cache data descriptors.
      Object set_result(
          &scope, Interpreter::callDescriptorSet(thread, thread->currentFrame(),
                                                 type_attr, object, value));
      if (set_result.isError()) return *set_result;
      return NoneType::object();
    }
  }

  // No data descriptor found, store on the instance.
  if (object.isHeapObject()) {
    HeapObject instance(&scope, *object);
    return instanceSetAttrSetLocation(thread, instance, name_interned_str,
                                      value, location_out);
  }
  return objectRaiseAttributeError(thread, object, name_interned_str);
}

RawObject objectSetAttr(Thread* thread, const Object& object,
                        const Object& name_interned_str, const Object& value) {
  return objectSetAttrSetLocation(thread, object, name_interned_str, value,
                                  nullptr);
}

RawObject objectDelItem(Thread* thread, const Object& object,
                        const Object& key) {
  HandleScope scope(thread);
  Object result(&scope,
                thread->invokeMethod2(object, SymbolId::kDunderDelitem, key));
  if (result.isErrorNotFound()) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "'%T' object does not support item deletion",
                                &object);
  }
  return *result;
}

RawObject objectGetItem(Thread* thread, const Object& object,
                        const Object& key) {
  HandleScope scope(thread);
  Object result(&scope,
                thread->invokeMethod2(object, SymbolId::kDunderGetitem, key));
  if (result.isErrorNotFound()) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "'%T' object is not subscriptable", &object);
  }
  return *result;
}

RawObject objectSetItem(Thread* thread, const Object& object, const Object& key,
                        const Object& value) {
  HandleScope scope(thread);
  Object result(&scope, thread->invokeMethod3(object, SymbolId::kDunderSetitem,
                                              key, value));
  if (result.isErrorNotFound()) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "'%T' object does not support item assignment",
                                &object);
  }
  return *result;
}

const BuiltinMethod ObjectBuiltins::kBuiltinMethods[] = {
    {SymbolId::kDunderHash, dunderHash},
    {SymbolId::kDunderInit, dunderInit},
    {SymbolId::kDunderNew, dunderNew},
    {SymbolId::kDunderSetattr, dunderSetattr},
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
  object_type.setBases(runtime->emptyTuple());
  runtime->layoutAtPut(LayoutId::kObject, *layout);

  for (uword i = 0; i < ARRAYSIZE(kBuiltinMethods); i++) {
    runtime->typeAddBuiltinFunction(object_type, kBuiltinMethods[i].name,
                                    kBuiltinMethods[i].address);
  }

  postInitialize(runtime, object_type);
}

void ObjectBuiltins::postInitialize(Runtime* runtime, const Type& new_type) {
  // Add object as the implicit base class for new types.
  runtime->initializeImplicitBases();

  // Don't allow users to set attributes on object() instances
  new_type.sealAttributes();

  // Manually create `__getattribute__` method to avoid bootstrap problems.
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Dict type_dict(&scope, new_type.dict());

  Tuple parameter_names(&scope, runtime->newTuple(2));
  parameter_names.atPut(0, runtime->symbols()->Self());
  parameter_names.atPut(1, runtime->symbols()->Name());
  Str name(&scope, runtime->symbols()->DunderGetattribute());
  Code code(&scope,
            runtime->newBuiltinCode(
                /*argcount=*/2, /*posonlyargcount=*/2, /*kwonlyargcount=*/0,
                /*flags=*/0, dunderGetattribute, parameter_names, name));
  Object qualname(
      &scope, runtime->internStrFromCStr(thread, "object.__getattribute__"));
  Object globals(&scope, NoneType::object());
  Function dunder_getattribute(
      &scope, runtime->newFunctionWithCode(thread, qualname, code, globals));

  runtime->typeDictAtPutByStr(thread, type_dict, name, dunder_getattribute);
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
  if ((typeLookupInMroById(thread, type, SymbolId::kDunderNew) ==
       runtime->objectDunderNew()) ||
      (typeLookupInMroById(thread, type, SymbolId::kDunderInit) !=
       runtime->objectDunderInit())) {
    // Throw a TypeError if extra arguments were passed, and __new__ was not
    // overwritten by self, or __init__ was overloaded by self.
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "object.__init__() takes no parameters");
  }
  // Else it's alright to have extra arguments.
  return NoneType::object();
}

RawObject ObjectBuiltins::dunderNew(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Type type(&scope, args.get(0));
  if (!type.hasFlag(Type::Flag::kIsAbstract)) {
    Layout layout(&scope, type.instanceLayout());
    return thread->runtime()->newInstance(layout);
  }
  // `type` is an abstract class and cannot be instantiated.
  Object name(&scope, type.name());
  Object comma(&scope, SmallStr::fromCStr(", "));
  Object methods(&scope, type.abstractMethods());
  Object sorted(&scope, thread->invokeFunction1(SymbolId::kBuiltins,
                                                SymbolId::kSorted, methods));
  if (sorted.isError()) return *sorted;
  Object joined(&scope, thread->invokeMethod2(comma, SymbolId::kJoin, sorted));
  if (joined.isError()) return *joined;
  return thread->raiseWithFmt(
      LayoutId::kTypeError,
      "Can't instantiate abstract class %S with abstract methods %S", &name,
      &joined);
}

RawObject ObjectBuiltins::dunderSetattr(Thread* thread, Frame* frame,
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
  if (!name.isStr()) {
    UNIMPLEMENTED("Strict subclass of string");
  }
  name = runtime->internStr(thread, name);
  Object value(&scope, args.get(2));
  return objectSetAttr(thread, self, name, value);
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
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "__repr__ expects None as first argument");
  }
  return thread->runtime()->symbols()->None();
}

}  // namespace python
