#include "object-builtins.h"

#include <cinttypes>

#include "builtins.h"
#include "descriptor-builtins.h"
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
  Layout layout(&scope, runtime->layoutOf(*instance));
  AttributeInfo info;
  if (!Runtime::layoutFindAttribute(*layout, name, &info)) {
    if (layout.hasDictOverflow()) {
      word offset = layout.dictOverflowOffset();
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
  Layout new_layout(&scope,
                    runtime->layoutDeleteAttribute(thread, layout, name, info));
  LayoutId new_layout_id = new_layout.id();
  instance.setHeader(instance.header().withLayoutId(new_layout_id));

  if (info.isInObject()) {
    instance.instanceVariableAtPut(info.offset(), NoneType::object());
  } else {
    Tuple overflow(&scope,
                   instance.instanceVariableAt(new_layout.overflowOffset()));
    overflow.atPut(info.offset(), NoneType::object());
  }

  return NoneType::object();
}

RawObject instanceGetAttributeSetLocation(Thread* thread,
                                          const Instance& instance,
                                          const Object& name,
                                          Object* location_out) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Layout layout(&scope, runtime->layoutOf(*instance));
  AttributeInfo info;
  if (Runtime::layoutFindAttribute(*layout, name, &info)) {
    if (info.isInObject()) {
      if (location_out != nullptr) {
        *location_out = SmallInt::fromWord(info.offset());
      }
      return instance.instanceVariableAt(info.offset());
    }
    word offset = info.offset();
    if (location_out != nullptr) {
      *location_out = SmallInt::fromWord(-offset - 1);
    }
    word tuple_offset = layout.overflowOffset();
    return Tuple::cast(instance.instanceVariableAt(tuple_offset)).at(offset);
  }
  if (layout.hasDictOverflow()) {
    word offset = layout.dictOverflowOffset();
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

void instanceGrowOverflow(Thread* thread, const Instance& instance,
                          word length) {
  HandleScope scope(thread);
  Layout layout(&scope, thread->runtime()->layoutOf(*instance));
  Tuple overflow(&scope, instance.instanceVariableAt(layout.overflowOffset()));
  DCHECK(overflow.length() < length, "unexpected overflow");
  MutableTuple new_overflow(&scope, thread->runtime()->newMutableTuple(length));
  new_overflow.replaceFromWith(0, *overflow, overflow.length());
  instance.instanceVariableAtPut(layout.overflowOffset(),
                                 new_overflow.becomeImmutable());
}

static RawObject instanceSetAttrSetLocation(Thread* thread,
                                            const Instance& instance,
                                            const Object& name,
                                            const Object& value,
                                            Object* location_out) {
  HandleScope scope(thread);

  // If the attribute doesn't exist we'll need to transition the layout
  Runtime* runtime = thread->runtime();
  Layout layout(&scope, runtime->layoutOf(*instance));
  AttributeInfo info;
  if (!Runtime::layoutFindAttribute(*layout, name, &info)) {
    if (!layout.hasTupleOverflow()) {
      if (layout.hasDictOverflow()) {
        word offset = layout.dictOverflowOffset();
        Object overflow_dict_obj(&scope, instance.instanceVariableAt(offset));
        if (overflow_dict_obj.isNoneType()) {
          overflow_dict_obj = runtime->newDict();
          instance.instanceVariableAtPut(offset, *overflow_dict_obj);
        }
        Dict overflow_dict(&scope, *overflow_dict_obj);
        dictAtPutByStr(thread, overflow_dict, name, value);
        return NoneType::object();
      }
      if (layout.isSealed()) {
        return thread->raiseWithFmt(
            LayoutId::kAttributeError,
            "Cannot set attribute '%S' on sealed class '%T'", &name, &instance);
      }
    }
    // Transition the layout.

    Layout new_layout(
        &scope, runtime->layoutAddAttribute(thread, layout, name, 0, &info));
    if (info.isOverflow() &&
        info.offset() >=
            Tuple::cast(instance.instanceVariableAt(layout.overflowOffset()))
                .length()) {
      instanceGrowOverflow(thread, instance, info.offset() + 1);
    }
    instance.setLayoutId(new_layout.id());
    layout = *new_layout;
  } else if (info.isReadOnly()) {
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
    Tuple::cast(instance.instanceVariableAt(layout.overflowOffset()))
        .atPut(info.offset(), *value);
    if (location_out != nullptr) {
      *location_out = SmallInt::fromWord(-info.offset() - 1);
    }
  }
  return NoneType::object();
}

RawObject instanceSetAttr(Thread* thread, const Instance& instance,
                          const Object& name, const Object& value) {
  return instanceSetAttrSetLocation(thread, instance, name, value, nullptr);
}

RawObject objectGetAttributeSetLocation(Thread* thread, const Object& object,
                                        const Object& name,
                                        Object* location_out,
                                        LoadAttrKind* kind) {
  // Look for the attribute in the class
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Type type(&scope, runtime->typeOf(*object));
  Object type_attr_location(&scope, NoneType::object());
  Object type_attr(&scope, typeLookupInMroSetLocation(thread, type, name,
                                                      &type_attr_location));
  if (!type_attr.isError()) {
    // TODO(T56252621): Remove this once property gets cached.
    if (type_attr.isProperty()) {
      Object getter(&scope, Property::cast(*type_attr).getter());
      DCHECK(!object.isNoneType(), "object cannot be NoneType");
      if (getter.isFunction()) {
        // We cache only function objects as a getter to simplify its usage.
        if (location_out != nullptr) {
          *location_out = *getter;
          *kind = LoadAttrKind::kInstanceProperty;
        }
        return Interpreter::callFunction1(thread, thread->currentFrame(),
                                          getter, object);
      }
    }
    if (type_attr.isSlotDescriptor()) {
      SlotDescriptor slot_descriptor(&scope, *type_attr);
      Object result(&scope, slotDescriptorGet(thread, slot_descriptor, object));
      if (!result.isErrorException() && location_out != nullptr) {
        // Cache slot_descriptor at success only to avoid type checking
        // afterwards. However, unbound check should be performed when
        // cache is hit.
        *location_out = SmallInt::fromWord(slot_descriptor.offset());
        *kind = LoadAttrKind::kInstanceSlotDescr;
      }
      return *result;
    }
    Type type_attr_type(&scope, runtime->typeOf(*type_attr));
    if (typeIsDataDescriptor(thread, type_attr_type)) {
      if (location_out != nullptr) {
        *location_out = *type_attr;
        *kind = LoadAttrKind::kInstanceTypeDescr;
      }
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
      if (location_out != nullptr) *kind = LoadAttrKind::kInstanceOffset;
      return *result;
    }
  }

  // Nothing found in the instance, if we found a non-data descriptor via the
  // class search, use it.
  if (!type_attr.isError()) {
    if (type_attr.isFunction()) {
      if (location_out != nullptr) {
        *location_out = *type_attr;
        *kind = LoadAttrKind::kInstanceFunction;
      }
      return runtime->newBoundMethod(type_attr, object);
    }

    Type type_attr_type(&scope, thread->runtime()->typeOf(*type_attr));
    if (!typeIsNonDataDescriptor(thread, type_attr_type)) {
      if (location_out != nullptr) {
        *location_out = *type_attr;
        *kind = LoadAttrKind::kInstanceType;
      }
      return *type_attr;
    }
    if (location_out != nullptr) {
      *location_out = *type_attr;
      *kind = LoadAttrKind::kInstanceTypeDescr;
    }
    return Interpreter::callDescriptorGet(thread, thread->currentFrame(),
                                          type_attr, object, type);
  }
  return Error::notFound();
}

RawObject objectGetAttribute(Thread* thread, const Object& object,
                             const Object& name) {
  return objectGetAttributeSetLocation(thread, object, name, nullptr, nullptr);
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
    Instance result(&scope, runtime->newInstance(layout));
    if (type.hasFlag(Type::Flag::kHasSlots)) {
      Tuple attributes(&scope, layout.inObjectAttributes());
      for (word i = 0, length = attributes.length(); i < length; i++) {
        AttributeInfo info(Tuple::cast(attributes.at(i)).at(1));
        if (info.isInitWithUnbound()) {
          DCHECK(info.isInObject(), "in-object is expected");
          result.instanceVariableAtPut(info.offset(), Unbound::object());
        }
      }
    }
    return *result;
  }
  // `type` is an abstract class and cannot be instantiated.
  Object name(&scope, type.name());
  Object comma(&scope, SmallStr::fromCStr(", "));
  Object methods(&scope, type.abstractMethods());
  Object sorted(&scope,
                thread->invokeFunction1(ID(builtins), ID(sorted), methods));
  if (sorted.isError()) return *sorted;
  Object joined(&scope, thread->invokeMethod2(comma, ID(join), sorted));
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
    if (type_attr.isSlotDescriptor()) {
      SlotDescriptor slot_descriptor(&scope, *type_attr);
      Object result(&scope,
                    slotDescriptorSet(thread, slot_descriptor, object, value));
      if (!result.isErrorException() && location_out != nullptr) {
        // Cache slot_descriptor at success only to avoid type checking
        // afterwards. Note that writes via slot_descriptor are treated equally
        // as ones to in-object instance attributes since the same cache
        // invalidation rule applies to them.
        *location_out = SmallInt::fromWord(slot_descriptor.offset());
      }
      return *result;
    }
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
  Object result(&scope, thread->invokeMethod2(object, ID(__delitem__), key));
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
  // This logic is replicated in Interpreter::binarySubscrUpdateCache for
  // optimization.
  Object result(&scope, thread->invokeMethod2(object, ID(__getitem__), key));
  if (result.isErrorNotFound()) {
    Runtime* runtime = thread->runtime();
    if (runtime->isInstanceOfType(*object)) {
      Type object_as_type(&scope, *object);
      Str dunder_class_getitem_name(
          &scope, runtime->symbols()->at(ID(__class_getitem__)));
      Object class_getitem(&scope, typeGetAttribute(thread, object_as_type,
                                                    dunder_class_getitem_name));
      if (!class_getitem.isErrorNotFound()) {
        return Interpreter::callFunction1(thread, thread->currentFrame(),
                                          class_getitem, key);
      }
    }
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "'%T' object is not subscriptable", &object);
  }
  return *result;
}

RawObject objectSetItem(Thread* thread, const Object& object, const Object& key,
                        const Object& value) {
  HandleScope scope(thread);
  // Short-cut for the common case of dict. This also helps during bootstrapping
  // as it allows us to use `objectSetItem` before `dict.__setitem__` is added.
  if (object.isDict()) {
    Dict object_dict(&scope, *object);
    RawObject hash = Interpreter::hash(thread, key);
    if (hash.isErrorException()) return hash;
    dictAtPut(thread, object_dict, key, SmallInt::cast(hash).value(), value);
    return NoneType::object();
  }
  Object result(&scope,
                thread->invokeMethod3(object, ID(__setitem__), key, value));
  if (result.isErrorNotFound()) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "'%T' object does not support item assignment",
                                &object);
  }
  return *result;
}

RawObject METH(object, __getattribute__)(Thread* thread, Frame* frame,
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

RawObject METH(object, __hash__)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  return SmallInt::fromWord(thread->runtime()->hash(args.get(0)));
}

RawObject METH(object, __init__)(Thread* thread, Frame* frame, word nargs) {
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
  if ((typeLookupInMroById(thread, type, ID(__new__)) ==
       runtime->objectDunderNew()) ||
      (typeLookupInMroById(thread, type, ID(__init__)) !=
       runtime->objectDunderInit())) {
    // Throw a TypeError if extra arguments were passed, and __new__ was not
    // overwritten by self, or __init__ was overloaded by self.
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "object.__init__() takes no parameters");
  }
  // Else it's alright to have extra arguments.
  return NoneType::object();
}

RawObject METH(object, __new__)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object type_obj(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfType(*type_obj)) {
    return thread->raiseRequiresType(type_obj, ID(type));
  }
  Type type(&scope, args.get(0));
  return objectNew(thread, type);
}

RawObject METH(object, __setattr__)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Object name(&scope, args.get(1));
  name = attributeName(thread, name);
  if (name.isErrorException()) return *name;
  Object value(&scope, args.get(2));
  return objectSetAttr(thread, self, name, value);
}

RawObject METH(object, __sizeof__)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object obj(&scope, args.get(0));
  if (obj.isHeapObject()) {
    HeapObject heap_obj(&scope, *obj);
    return SmallInt::fromWord(heap_obj.size());
  }
  return SmallInt::fromWord(kPointerSize);
}

RawObject METH(NoneType, __new__)(Thread*, Frame*, word) {
  return NoneType::object();
}

RawObject METH(NoneType, __repr__)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  if (!args.get(0).isNoneType()) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "__repr__ expects None as first argument");
  }
  return thread->runtime()->symbols()->at(ID(None));
}

static const BuiltinAttribute kInstanceProxyAttributes[] = {
    {ID(_instance), RawInstanceProxy::kInstanceOffset},
};

static void addObjectType(Thread* thread) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Layout layout(&scope, runtime->newLayout(LayoutId::kObject));
  runtime->layoutAtPut(LayoutId::kObject, *layout);
  Type type(&scope, runtime->newType());
  layout.setDescribedType(*type);
  type.setName(runtime->symbols()->at(ID(object)));
  Tuple mro(&scope, runtime->newTupleWith1(type));
  type.setMro(*mro);
  type.setInstanceLayout(*layout);
  type.setBases(runtime->emptyTuple());

  // Manually create `__getattribute__` method to avoid bootstrap problems.
  Tuple parameter_names(&scope, runtime->newTuple(2));
  parameter_names.atPut(0, runtime->symbols()->at(ID(self)));
  parameter_names.atPut(1, runtime->symbols()->at(ID(name)));
  Object name(&scope, runtime->symbols()->at(ID(__getattribute__)));
  Code code(
      &scope,
      runtime->newBuiltinCode(
          /*argcount=*/2, /*posonlyargcount=*/2, /*kwonlyargcount=*/0,
          /*flags=*/0, METH(object, __getattribute__), parameter_names, name));
  Object qualname(
      &scope, Runtime::internStrFromCStr(thread, "object.__getattribute__"));
  Object module_obj(&scope, NoneType::object());
  Function dunder_getattribute(
      &scope, runtime->newFunctionWithCode(thread, qualname, code, module_obj));
  typeAtPutById(thread, type, ID(__getattribute__), dunder_getattribute);
}

void initializeObjectTypes(Thread* thread) {
  addObjectType(thread);

  addImmediateBuiltinType(thread, ID(NoneType), LayoutId::kNoneType,
                          /*builtin_base=*/LayoutId::kNoneType,
                          /*superclass_id=*/LayoutId::kObject);

  addImmediateBuiltinType(thread, ID(NotImplementedType),
                          LayoutId::kNotImplementedType,
                          /*builtin_base=*/LayoutId::kNotImplementedType,
                          /*superclass_id=*/LayoutId::kObject);

  addImmediateBuiltinType(thread, ID(_Unbound), LayoutId::kUnbound,
                          /*builtin_base=*/LayoutId::kUnbound,
                          /*superclass_id=*/LayoutId::kObject);

  addBuiltinType(thread, ID(instance_proxy), LayoutId::kInstanceProxy,
                 /*superclass_id=*/LayoutId::kObject, kInstanceProxyAttributes);
}

}  // namespace py
