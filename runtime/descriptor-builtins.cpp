#include "descriptor-builtins.h"

#include "builtins.h"
#include "frame.h"
#include "globals.h"
#include "object-builtins.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"
#include "type-builtins.h"

namespace py {

static const BuiltinAttribute kClassMethodAttributes[] = {
    {ID(__func__), RawClassMethod::kFunctionOffset, AttributeFlags::kReadOnly},
};

static const BuiltinAttribute kPropertyAttributes[] = {
    {ID(fget), RawProperty::kGetterOffset, AttributeFlags::kReadOnly},
    {ID(fset), RawProperty::kSetterOffset, AttributeFlags::kReadOnly},
    {ID(fdel), RawProperty::kDeleterOffset, AttributeFlags::kReadOnly},
    {ID(__doc__), RawProperty::kDocOffset},
};

static const BuiltinAttribute kSlotDescriptorAttributes[] = {
    {ID(__objclass__), RawSlotDescriptor::kTypeOffset,
     AttributeFlags::kReadOnly},
    {ID(__name__), RawSlotDescriptor::kNameOffset, AttributeFlags::kReadOnly},
    {ID(_slot_descriptor__offset), RawSlotDescriptor::kOffsetOffset,
     AttributeFlags::kHidden},
};

static const BuiltinAttribute kStaticMethodAttributes[] = {
    {ID(__func__), RawStaticMethod::kFunctionOffset, AttributeFlags::kReadOnly},
};

void initializeDescriptorTypes(Thread* thread) {
  addBuiltinType(thread, ID(classmethod), LayoutId::kClassMethod,
                 /*superclass_id=*/LayoutId::kObject, kClassMethodAttributes,
                 ClassMethod::kSize, /*basetype=*/true);

  addBuiltinType(thread, ID(property), LayoutId::kProperty,
                 /*superclass_id=*/LayoutId::kObject, kPropertyAttributes,
                 Property::kSize, /*basetype=*/true);

  addBuiltinType(thread, ID(slot_descriptor), LayoutId::kSlotDescriptor,
                 /*superclass_id=*/LayoutId::kObject, kSlotDescriptorAttributes,
                 SlotDescriptor::kSize, /*basetype=*/false);

  addBuiltinType(thread, ID(staticmethod), LayoutId::kStaticMethod,
                 /*superclass_id=*/LayoutId::kObject, kStaticMethodAttributes,
                 StaticMethod::kSize, /*basetype=*/true);
}

// classmethod

RawObject METH(classmethod, __new__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object type_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfType(*type_obj)) {
    return thread->raiseWithFmt(LayoutId::kTypeError, "not a type object");
  }
  Type type(&scope, *type_obj);
  if (type.builtinBase() != LayoutId::kClassMethod) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "not a subtype of classmethod");
  }
  Layout layout(&scope, type.instanceLayout());
  ClassMethod result(&scope, runtime->newInstance(layout));
  return *result;
}

RawObject METH(classmethod, __init__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfClassMethod(*self)) {
    return thread->raiseRequiresType(self, ID(classmethod));
  }
  ClassMethod classmethod(&scope, *self);
  Object arg(&scope, args.get(1));
  classmethod.setFunction(*arg);
  return NoneType::object();
}

RawObject METH(classmethod, __get__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object self(&scope, args.get(0));
  if (!runtime->isInstanceOfClassMethod(*self)) {
    return thread->raiseRequiresType(self, ID(classmethod));
  }
  Object owner(&scope, args.get(2));

  ClassMethod classmethod(&scope, *self);
  Object method(&scope, classmethod.function());
  return runtime->newBoundMethod(method, owner);
}

// slot_descriptor

static RawObject slotDescriptorRaiseTypeError(
    Thread* thread, const SlotDescriptor& slot_descriptor,
    const Object& instance_obj) {
  HandleScope scope(thread);
  Str slot_descriptor_name(&scope, slot_descriptor.name());
  Type slot_descriptor_type(&scope, slot_descriptor.type());
  Str slot_descriptor_type_name(&scope, slot_descriptor_type.name());
  return thread->raiseWithFmt(LayoutId::kTypeError,
                              "descriptor '%S' for '%S' objects "
                              "doesn't apply to '%T' object",
                              &slot_descriptor_name, &slot_descriptor_type_name,
                              &instance_obj);
}

RawObject slotDescriptorGet(Thread* thread,
                            const SlotDescriptor& slot_descriptor,
                            const Object& instance_obj) {
  HandleScope scope(thread);
  Type instance_type(&scope,
                     thread->runtime()->typeAt(instance_obj.layoutId()));
  if (!typeIsSubclass(*instance_type, slot_descriptor.type())) {
    return slotDescriptorRaiseTypeError(thread, slot_descriptor, instance_obj);
  }
  DCHECK(instance_type.hasFlag(Type::Flag::kHasSlots),
         "instance type is expected to set kHasSlots");
  Instance instance(&scope, *instance_obj);
  word offset = slot_descriptor.offset();
  DCHECK_BOUND(offset, instance.size() - kPointerSize);
  Object attribute_value(&scope, instance.instanceVariableAt(offset));
  if (attribute_value.isUnbound()) {
    Object attribute_name(&scope, slot_descriptor.name());
    return objectRaiseAttributeError(thread, instance, attribute_name);
  }
  return *attribute_value;
}

RawObject METH(slot_descriptor, __delete__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  SlotDescriptor slot_descriptor(&scope, args.get(0));
  Object instance_obj(&scope, args.get(1));
  Object existing_value(
      &scope, slotDescriptorGet(thread, slot_descriptor, instance_obj));
  if (existing_value.isErrorException()) {
    return *existing_value;
  }
  Instance instance(&scope, *instance_obj);
  instance.instanceVariableAtPut(slot_descriptor.offset(), Unbound::object());
  return NoneType::object();
}

RawObject METH(slot_descriptor, __get__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  SlotDescriptor slot_descriptor(&scope, args.get(0));
  Object instance(&scope, args.get(1));
  return slotDescriptorGet(thread, slot_descriptor, instance);
}

RawObject slotDescriptorSet(Thread* thread,
                            const SlotDescriptor& slot_descriptor,
                            const Object& instance_obj, const Object& value) {
  HandleScope scope(thread);
  Type instance_type(&scope,
                     thread->runtime()->typeAt(instance_obj.layoutId()));
  if (!typeIsSubclass(*instance_type, slot_descriptor.type())) {
    return slotDescriptorRaiseTypeError(thread, slot_descriptor, instance_obj);
  }
  DCHECK(instance_type.hasFlag(Type::Flag::kHasSlots),
         "instance type is expected to set kHasSlots");
  Instance instance(&scope, *instance_obj);
  word offset = slot_descriptor.offset();
  DCHECK_BOUND(offset, instance.size() - kPointerSize);
  instance.instanceVariableAtPut(offset, *value);
  return NoneType::object();
}

RawObject METH(slot_descriptor, __set__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  SlotDescriptor slot_descriptor(&scope, args.get(0));
  Object instance(&scope, args.get(1));
  Object value(&scope, args.get(2));
  return slotDescriptorSet(thread, slot_descriptor, instance, value);
}

// staticmethod

RawObject METH(staticmethod, __get__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfStaticMethod(*self)) {
    return thread->raiseRequiresType(self, ID(staticmethod));
  }
  StaticMethod staticmethod(&scope, *self);
  return staticmethod.function();
}

RawObject METH(staticmethod, __new__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object type_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfType(*type_obj)) {
    return thread->raiseWithFmt(LayoutId::kTypeError, "not a type object");
  }
  Type type(&scope, *type_obj);
  if (type.builtinBase() != LayoutId::kStaticMethod) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "not a subtype of staticmethod");
  }
  Layout layout(&scope, type.instanceLayout());
  StaticMethod result(&scope, runtime->newInstance(layout));
  return *result;
}

RawObject METH(staticmethod, __init__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  StaticMethod staticmethod(&scope, args.get(0));
  Object arg(&scope, args.get(1));
  staticmethod.setFunction(*arg);
  return NoneType::object();
}

// property

RawObject METH(property, __delete__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfProperty(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(property));
  }
  Property self(&scope, *self_obj);
  Object deleter(&scope, self.deleter());
  if (deleter.isNoneType()) {
    return thread->raiseWithFmt(LayoutId::kAttributeError,
                                "can't delete attribute");
  }
  Object instance(&scope, args.get(1));
  return Interpreter::call1(thread, deleter, instance);
}

RawObject METH(property, __get__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfProperty(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(property));
  }
  Property self(&scope, *self_obj);
  Object getter(&scope, self.getter());
  if (getter.isNoneType()) {
    return thread->raiseWithFmt(LayoutId::kAttributeError,
                                "unreadable attribute");
  }
  Object instance(&scope, args.get(1));
  if (instance.isNoneType()) {
    return *self;
  }
  return Interpreter::call1(thread, getter, instance);
}

RawObject METH(property, __init__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfProperty(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(property));
  }
  Property self(&scope, *self_obj);
  self.setGetter(args.get(1));
  self.setSetter(args.get(2));
  self.setDeleter(args.get(3));
  // TODO(T42363565) Do something with the doc argument.
  return NoneType::object();
}

RawObject METH(property, __new__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object type_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfType(*type_obj)) {
    return thread->raiseWithFmt(LayoutId::kTypeError, "not a type object");
  }
  Type type(&scope, *type_obj);
  if (type.builtinBase() != LayoutId::kProperty) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "not a subtype of property");
  }
  Layout layout(&scope, type.instanceLayout());
  Property result(&scope, runtime->newInstance(layout));
  return *result;
}

RawObject METH(property, __set__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfProperty(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(property));
  }
  Property self(&scope, *self_obj);
  Object setter(&scope, self.setter());
  if (setter.isNoneType()) {
    return thread->raiseWithFmt(LayoutId::kAttributeError,
                                "can't set attribute");
  }
  Object obj(&scope, args.get(1));
  Object value(&scope, args.get(2));
  return Interpreter::call2(thread, setter, obj, value);
}

RawObject METH(property, deleter)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfProperty(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(property));
  }
  Property self(&scope, *self_obj);
  Object getter(&scope, self.getter());
  Object setter(&scope, self.setter());
  Object deleter(&scope, args.get(1));
  return runtime->newProperty(getter, setter, deleter);
}

RawObject METH(property, getter)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfProperty(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(property));
  }
  Property self(&scope, *self_obj);
  Object getter(&scope, args.get(1));
  Object setter(&scope, self.setter());
  Object deleter(&scope, self.deleter());
  return runtime->newProperty(getter, setter, deleter);
}

RawObject METH(property, setter)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfProperty(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(property));
  }
  Property self(&scope, *self_obj);
  Object getter(&scope, self.getter());
  Object setter(&scope, args.get(1));
  Object deleter(&scope, self.deleter());
  return runtime->newProperty(getter, setter, deleter);
}

}  // namespace py
