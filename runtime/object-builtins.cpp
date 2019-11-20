#include "object-builtins.h"

#include <cinttypes>

#include "dict-builtins.h"
#include "frame.h"
#include "globals.h"
#include "ic.h"
#include "module-builtins.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"
#include "type-builtins.h"

namespace py {

RawObject objectRaiseAttributeError(Thread* thread, const Object& object,
                                    const Object& name) {
  return thread->raiseWithFmt(LayoutId::kAttributeError,
                              "'%T' object has no attribute '%S'", &object,
                              &name);
}

RawObject instanceDelAttr(Thread* thread, const Instance& instance,
                          const Object& name) {
  HandleScope scope(thread);

  // Remove the reference to the attribute value from the instance
  Runtime* runtime = thread->runtime();
  Layout layout(&scope, runtime->layoutAt(instance.layoutId()));
  AttributeInfo info;
  if (!runtime->layoutFindAttribute(thread, layout, name, &info)) {
    if (layout.hasDictOverflow()) {
      word offset = SmallInt::cast(layout.overflowAttributes()).value();
      Object overflow_dict_obj(&scope, instance.instanceVariableAt(offset));
      if (!overflow_dict_obj.isNoneType()) {
        Dict overflow_dict(&scope, *overflow_dict_obj);
        Object result(&scope, dictRemoveByStr(thread, overflow_dict, name));
        if (result.isError()) return *result;
        return NoneType::object();
      }
    }
    return Error::notFound();
  }

  if (info.isReadOnly()) {
    return thread->raiseWithFmt(LayoutId::kAttributeError,
                                "'%S' attribute is read-only", &name);
  }

  // Make the attribute invisible
  Object new_layout(&scope,
                    runtime->layoutDeleteAttribute(thread, layout, name));
  DCHECK(!new_layout.isError(), "should always find attribute here");
  LayoutId new_layout_id = Layout::cast(*new_layout).id();
  instance.setHeader(instance.header().withLayoutId(new_layout_id));

  if (info.isInObject()) {
    instance.instanceVariableAtPut(info.offset(), NoneType::object());
  } else {
    Tuple overflow(&scope, instance.instanceVariableAt(
                               Layout::cast(*new_layout).overflowOffset()));
    overflow.atPut(info.offset(), NoneType::object());
  }

  return NoneType::object();
}

RawObject instanceGetAttributeSetLocation(Thread* thread,
                                          const Instance& instance,
                                          const Object& name,
                                          Object* location_out) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Layout layout(&scope, runtime->layoutAt(instance.layoutId()));
  AttributeInfo info;
  if (runtime->layoutFindAttribute(thread, layout, name, &info)) {
    if (info.isInObject()) {
      if (location_out != nullptr) {
        *location_out = RawSmallInt::fromWord(info.offset());
      }
      return instance.instanceVariableAt(info.offset());
    }
    Tuple overflow(&scope,
                   instance.instanceVariableAt(layout.overflowOffset()));
    if (location_out != nullptr) {
      *location_out = RawSmallInt::fromWord(-info.offset() - 1);
    }
    return overflow.at(info.offset());
  }
  if (layout.hasDictOverflow()) {
    word offset = SmallInt::cast(layout.overflowAttributes()).value();
    Object overflow_dict_obj(&scope, instance.instanceVariableAt(offset));
    if (!overflow_dict_obj.isNoneType()) {
      Dict overflow_dict(&scope, *overflow_dict_obj);
      return dictAtByStr(thread, overflow_dict, name);
    }
  }
  return Error::notFound();
}

RawObject instanceGetAttribute(Thread* thread, const Instance& instance,
                               const Object& name) {
  return instanceGetAttributeSetLocation(thread, instance, name, nullptr);
}

static RawObject instanceSetAttrSetLocation(Thread* thread,
                                            const Instance& instance,
                                            const Object& name,
                                            const Object& value,
                                            Object* location_out) {
  HandleScope scope(thread);

  // If the attribute doesn't exist we'll need to transition the layout
  bool has_new_layout_id = false;
  Runtime* runtime = thread->runtime();
  Layout layout(&scope, runtime->layoutAt(instance.layoutId()));
  AttributeInfo info;
  if (!runtime->layoutFindAttribute(thread, layout, name, &info)) {
    if (layout.isSealed()) {
      return thread->raiseWithFmt(
          LayoutId::kAttributeError,
          "Cannot set attribute '%S' on sealed class '%T'", &name, &instance);
    }

    if (layout.hasDictOverflow()) {
      word offset = SmallInt::cast(layout.overflowAttributes()).value();
      Object overflow_dict_obj(&scope, instance.instanceVariableAt(offset));
      if (overflow_dict_obj.isNoneType()) {
        overflow_dict_obj = runtime->newDict();
        instance.instanceVariableAtPut(offset, *overflow_dict_obj);
      }
      Dict overflow_dict(&scope, *overflow_dict_obj);
      dictAtPutByStr(thread, overflow_dict, name, value);
      return NoneType::object();
    }

    // Transition the layout
    layout = runtime->layoutAddAttribute(thread, layout, name, 0);
    has_new_layout_id = true;

    bool found = runtime->layoutFindAttribute(thread, layout, name, &info);
    DCHECK(found, "couldn't find attribute on new layout");
  }

  if (info.isReadOnly()) {
    return thread->raiseWithFmt(LayoutId::kAttributeError,
                                "'%T.%S' attribute is read-only", &instance,
                                &name);
  }

  DCHECK(!thread->runtime()->isInstanceOfType(*instance),
         "must not cache type attributes");
  // Store the attribute
  if (info.isInObject()) {
    instance.instanceVariableAtPut(info.offset(), *value);
    if (location_out != nullptr) {
      *location_out = SmallInt::fromWord(info.offset());
    }
  } else {
    // Build the new overflow array
    Tuple overflow(&scope,
                   instance.instanceVariableAt(layout.overflowOffset()));
    word len = overflow.length();
    if (info.offset() < len) {
      overflow.atPut(info.offset(), *value);
    } else {
      MutableTuple new_overflow(&scope,
                                runtime->newMutableTuple(info.offset() + 1));
      new_overflow.replaceFromWith(0, *overflow, len);
      new_overflow.atPut(info.offset(), *value);
      instance.instanceVariableAtPut(layout.overflowOffset(),
                                     new_overflow.becomeImmutable());
    }
    if (location_out != nullptr) {
      *location_out = SmallInt::fromWord(-info.offset() - 1);
    }
  }

  if (has_new_layout_id) {
    instance.setHeader(instance.header().withLayoutId(layout.id()));
  }

  return NoneType::object();
}

RawObject instanceSetAttr(Thread* thread, const Instance& instance,
                          const Object& name, const Object& value) {
  return instanceSetAttrSetLocation(thread, instance, name, value, nullptr);
}

RawObject objectGetAttributeSetLocation(Thread* thread, const Object& object,
                                        const Object& name,
                                        Object* location_out) {
  // Look for the attribute in the class
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Type type(&scope, runtime->typeOf(*object));
  Object type_attr(&scope, typeLookupInMro(thread, type, name));
  if (!type_attr.isError()) {
    // TODO(T56252621): Remove this once property gets cached.
    if (type_attr.isProperty()) {
      Object getter(&scope, Property::cast(*type_attr).getter());
      DCHECK(!object.isNoneType(), "object cannot be NoneType");
      if (!getter.isNoneType()) {
        if (location_out != nullptr) {
          *location_out = *type_attr;
        }
        return Interpreter::callFunction1(thread, thread->currentFrame(),
                                          getter, object);
      }
    }
    Type type_attr_type(&scope, runtime->typeOf(*type_attr));
    if (typeIsDataDescriptor(thread, type_attr_type)) {
      return Interpreter::callDescriptorGet(thread, thread->currentFrame(),
                                            type_attr, object, type);
    }
  }

  // No data descriptor found on the class, look at the instance.
  if (object.isInstance()) {
    Instance instance(&scope, *object);
    Object result(&scope, instanceGetAttributeSetLocation(thread, instance,
                                                          name, location_out));
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
                             const Object& name) {
  return objectGetAttributeSetLocation(thread, object, name, nullptr);
}

RawObject objectNew(Thread* thread, const Type& type) {
  HandleScope scope(thread);
  if (!type.hasFlag(Type::Flag::kIsAbstract)) {
    Layout layout(&scope, type.instanceLayout());
    LayoutId id = layout.id();
    Runtime* runtime = thread->runtime();
    if (!isInstanceLayout(id)) {
      Object type_name(&scope, type.name());
      return thread->raiseWithFmt(
          LayoutId::kTypeError,
          "object.__new__(%S) is not safe. Use %S.__new__()", &type_name,
          &type_name);
    }
    return runtime->newInstance(layout);
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

RawObject objectSetAttrSetLocation(Thread* thread, const Object& object,
                                   const Object& name, const Object& value,
                                   Object* location_out) {
  Runtime* runtime = thread->runtime();
  // Check for a data descriptor
  HandleScope scope(thread);
  Type type(&scope, runtime->typeOf(*object));
  Object type_attr(&scope, typeLookupInMro(thread, type, name));
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
  if (object.isInstance()) {
    Instance instance(&scope, *object);
    return instanceSetAttrSetLocation(thread, instance, name, value,
                                      location_out);
  }
  return objectRaiseAttributeError(thread, object, name);
}

RawObject objectSetAttr(Thread* thread, const Object& object,
                        const Object& name, const Object& value) {
  return objectSetAttrSetLocation(thread, object, name, value, nullptr);
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

  Tuple parameter_names(&scope, runtime->newTuple(2));
  parameter_names.atPut(0, runtime->symbols()->Self());
  parameter_names.atPut(1, runtime->symbols()->Name());
  Object name(&scope, runtime->symbols()->at(SymbolId::kDunderGetattribute));
  Code code(&scope,
            runtime->newBuiltinCode(
                /*argcount=*/2, /*posonlyargcount=*/2, /*kwonlyargcount=*/0,
                /*flags=*/0, dunderGetattribute, parameter_names, name));
  Object qualname(
      &scope, Runtime::internStrFromCStr(thread, "object.__getattribute__"));
  Object module_obj(&scope, NoneType::object());
  Function dunder_getattribute(
      &scope, runtime->newFunctionWithCode(thread, qualname, code, module_obj));

  typeAtPutById(thread, new_type, SymbolId::kDunderGetattribute,
                dunder_getattribute);
}

RawObject ObjectBuiltins::dunderGetattribute(Thread* thread, Frame* frame,
                                             word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Object name(&scope, args.get(1));
  name = attributeName(thread, name);
  if (name.isErrorException()) return *name;
  Object result(&scope, objectGetAttribute(thread, self, name));
  if (result.isErrorNotFound()) {
    return objectRaiseAttributeError(thread, self, name);
  }
  return *result;
}

RawObject ObjectBuiltins::dunderHash(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  return SmallInt::fromWord(thread->runtime()->hash(args.get(0)));
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
  Object type_obj(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfType(*type_obj)) {
    return thread->raiseRequiresType(type_obj, SymbolId::kType);
  }
  Type type(&scope, args.get(0));
  return objectNew(thread, type);
}

RawObject ObjectBuiltins::dunderSetattr(Thread* thread, Frame* frame,
                                        word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Object name(&scope, args.get(1));
  name = attributeName(thread, name);
  if (name.isErrorException()) return *name;
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

}  // namespace py
