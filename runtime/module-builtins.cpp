#include "module-builtins.h"

#include "attributedict.h"
#include "builtins.h"
#include "capi.h"
#include "dict-builtins.h"
#include "frame.h"
#include "globals.h"
#include "ic.h"
#include "object-builtins.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"
#include "type-builtins.h"

namespace py {

RawObject moduleAt(const Module& module, const Object& name) {
  return attributeAt(*module, *name);
}

RawObject moduleAtById(Thread* thread, const Module& module, SymbolId id) {
  RawObject name = thread->runtime()->symbols()->at(id);
  return attributeAt(*module, name);
}

static RawObject filterPlaceholderValueCell(RawObject result) {
  if (result.isErrorNotFound()) {
    return result;
  }
  RawValueCell value_cell = ValueCell::cast(result);
  if (value_cell.isPlaceholder()) {
    return Error::notFound();
  }
  return value_cell;
}

RawObject moduleValueCellAtById(Thread* thread, const Module& module,
                                SymbolId id) {
  RawObject name = thread->runtime()->symbols()->at(id);
  return filterPlaceholderValueCell(attributeValueCellAt(*module, name));
}

RawObject moduleValueCellAt(Thread*, const Module& module, const Object& name) {
  return filterPlaceholderValueCell(attributeValueCellAt(*module, *name));
}

static RawObject moduleValueCellAtPut(Thread* thread, const Module& module,
                                      const Object& name, const Object& value) {
  HandleScope scope(thread);
  Object module_result(&scope, attributeValueCellAt(*module, *name));
  if (!module_result.isErrorNotFound() &&
      ValueCell::cast(*module_result).isPlaceholder()) {
    // A builtin entry is cached under the same name, so invalidate its caches.
    Object builtins(&scope, moduleAtById(thread, module, ID(__builtins__)));
    Module builtins_module(&scope, *module);
    if (builtins.isModuleProxy()) {
      builtins = ModuleProxy::cast(*builtins).module();
    }
    if (thread->runtime()->isInstanceOfModule(*builtins)) {
      builtins_module = *builtins;
      Object builtins_result(&scope,
                             attributeValueCellAt(*builtins_module, *name));
      if (!builtins_result.isErrorNotFound()) {
        ValueCell builtins_value_cell(&scope, *builtins_result);
        if (!builtins_value_cell.isPlaceholder()) {
          DCHECK(!builtins_value_cell.dependencyLink().isNoneType(),
                 "the builtin valuecell must have a dependent");
          icInvalidateGlobalVar(thread, builtins_value_cell);
        }
      }
    }
  }
  return attributeAtPut(thread, module, name, value);
}

RawObject moduleAtPut(Thread* thread, const Module& module, const Object& name,
                      const Object& value) {
  return moduleValueCellAtPut(thread, module, name, value);
}

RawObject moduleAtPutById(Thread* thread, const Module& module, SymbolId id,
                          const Object& value) {
  HandleScope scope(thread);
  Object name(&scope, thread->runtime()->symbols()->at(id));
  return moduleValueCellAtPut(thread, module, name, value);
}

RawObject moduleAtPutByCStr(Thread* thread, const Module& module,
                            const char* name_cstr, const Object& value) {
  HandleScope scope(thread);
  Object name(&scope, Runtime::internStrFromCStr(thread, name_cstr));
  return moduleValueCellAtPut(thread, module, name, value);
}

RawObject moduleKeys(Thread* thread, const Module& module) {
  return attributeKeys(thread, module);
}

word moduleLen(Thread* thread, const Module& module) {
  return attributeLen(thread, module);
}

RawObject moduleRaiseAttributeError(Thread* thread, const Module& module,
                                    const Object& name) {
  HandleScope scope(thread);
  Object module_name(&scope, module.name());
  if (!thread->runtime()->isInstanceOfStr(*module_name)) {
    return thread->raiseWithFmt(LayoutId::kAttributeError,
                                "module has no attribute '%S'", &name);
  }
  return thread->raiseWithFmt(LayoutId::kAttributeError,
                              "module '%S' has no attribute '%S'", &module_name,
                              &name);
}

RawObject moduleRemove(Thread* thread, const Module& module,
                       const Object& name) {
  DCHECK(Runtime::isInternedStr(thread, name), "expected interned str");
  HandleScope scope(thread);
  word index;
  Object value_cell_obj(&scope, NoneType::object());
  if (!attributeFindForRemoval(module, name, &value_cell_obj, &index)) {
    return Error::notFound();
  }
  attributeRemove(module, index);
  ValueCell value_cell(&scope, *value_cell_obj);
  icInvalidateGlobalVar(thread, value_cell);
  if (value_cell.isPlaceholder()) return Error::notFound();
  return value_cell.value();
}

RawObject moduleValues(Thread* thread, const Module& module) {
  return attributeValues(thread, module);
}

RawObject moduleGetAttribute(Thread* thread, const Module& module,
                             const Object& name) {
  return moduleGetAttributeSetLocation(thread, module, name, nullptr);
}

RawObject moduleGetAttributeSetLocation(Thread* thread, const Module& module,
                                        const Object& name,
                                        Object* location_out) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Type module_type(&scope, runtime->typeOf(*module));
  // TODO(T66579052): Skip type lookups for `module` type when `name` doesn't
  // start with "__".
  Object attr(&scope, typeLookupInMro(thread, *module_type, *name));
  if (!attr.isError()) {
    Type attr_type(&scope, runtime->typeOf(*attr));
    if (typeIsDataDescriptor(*attr_type)) {
      return Interpreter::callDescriptorGet(thread, attr, module, module_type);
    }
  }

  Object result(&scope, moduleValueCellAt(thread, module, name));
  DCHECK(result.isValueCell() || result.isErrorNotFound(),
         "result must be a value cell or not found");
  if (!result.isErrorNotFound() && !ValueCell::cast(*result).isPlaceholder()) {
    if (location_out != nullptr) {
      *location_out = *result;
    }
    return ValueCell::cast(*result).value();
  }

  if (!attr.isError()) {
    Type attr_type(&scope, runtime->typeOf(*attr));
    if (typeIsNonDataDescriptor(*attr_type)) {
      return Interpreter::callDescriptorGet(thread, attr, module, module_type);
    }
    return *attr;
  }

  Object dunder_getattr(&scope, moduleAtById(thread, module, ID(__getattr__)));
  if (!dunder_getattr.isErrorNotFound()) {
    return Interpreter::call1(thread, dunder_getattr, name);
  }

  return Error::notFound();
}

RawObject moduleSetAttr(Thread* thread, const Module& module,
                        const Object& name, const Object& value) {
  moduleAtPut(thread, module, name, value);
  return NoneType::object();
}

RawObject moduleInit(Thread* thread, const Module& module, const Object& name) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  module.setModuleProxy(runtime->newModuleProxy(module));
  if (name.isStr()) {
    module.setName(*name);
  }
  module.setDef(runtime->newIntFromCPtr(nullptr));
  module.setState(runtime->newIntFromCPtr(nullptr));
  moduleAtPutById(thread, module, ID(__name__), name);

  Object none(&scope, NoneType::object());
  moduleAtPutById(thread, module, ID(__doc__), none);
  moduleAtPutById(thread, module, ID(__package__), none);
  moduleAtPutById(thread, module, ID(__loader__), none);
  moduleAtPutById(thread, module, ID(__spec__), none);
  return NoneType::object();
}

static const BuiltinAttribute kModuleAttributes[] = {
    {ID(_module__attributes), RawModule::kAttributesOffset,
     AttributeFlags::kHidden},
    {ID(_module__attributes_remaining), RawModule::kAttributesRemainingOffset,
     AttributeFlags::kHidden},
    {ID(_module__name), RawModule::kNameOffset, AttributeFlags::kHidden},
    {ID(_module__def), RawModule::kDefOffset, AttributeFlags::kHidden},
    {ID(_module__state), RawModule::kStateOffset, AttributeFlags::kHidden},
    {ID(_module__proxy), RawModule::kModuleProxyOffset,
     AttributeFlags::kHidden},
};

void initializeModuleType(Thread* thread) {
  HandleScope scope(thread);
  Type type(&scope, addBuiltinType(thread, ID(module), LayoutId::kModule,
                                   /*superclass_id=*/LayoutId::kObject,
                                   kModuleAttributes, Module::kSize,
                                   /*basetype=*/true));
  word flags = static_cast<word>(type.flags());
  flags |= RawType::Flag::kHasCustomDict;
  type.setFlags(static_cast<Type::Flag>(flags));
  Runtime* runtime = thread->runtime();
  Object object_type(&scope, runtime->typeAt(LayoutId::kObject));
  type.setMro(runtime->newTupleWith2(type, object_type));
}

RawObject METH(module, __getattribute__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfModule(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(module));
  }
  Module self(&scope, *self_obj);
  Object name(&scope, args.get(1));
  name = attributeName(thread, name);
  if (name.isErrorException()) return *name;
  Object result(&scope, moduleGetAttribute(thread, self, name));
  if (result.isErrorNotFound()) {
    return moduleRaiseAttributeError(thread, self, name);
  }
  return *result;
}

RawObject METH(module, __new__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object cls_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfType(*cls_obj)) {
    return thread->raiseWithFmt(
        LayoutId::kTypeError, "module.__new__(X): X is not a type object (%T)",
        &cls_obj);
  }
  Type cls(&scope, *cls_obj);
  if (cls.builtinBase() != LayoutId::kModule) {
    Object cls_name(&scope, cls.name());
    return thread->raiseWithFmt(
        LayoutId::kTypeError,
        "module.__new__(%S): %S is not a subtype of module", &cls_name,
        &cls_name);
  }
  Layout layout(&scope, cls.instanceLayout());
  Module result(&scope, runtime->newInstance(layout));
  attributeDictInit(thread, result);
  result.setDef(runtime->newIntFromCPtr(nullptr));
  result.setId(runtime->reserveModuleId());
  return *result;
}

RawObject METH(module, __init__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfModule(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(module));
  }
  Module self(&scope, *self_obj);
  Object name(&scope, args.get(1));
  if (!runtime->isInstanceOfStr(*name)) {
    return thread->raiseWithFmt(
        LayoutId::kTypeError,
        "module.__init__() argument 1 must be str, not %T", &name);
  }
  return moduleInit(thread, self, name);
}

RawObject METH(module, __setattr__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfModule(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(module));
  }
  Module self(&scope, *self_obj);
  Object name(&scope, args.get(1));
  name = attributeName(thread, name);
  if (name.isErrorException()) return *name;
  Object value(&scope, args.get(2));
  return moduleSetAttr(thread, self, name, value);
}

}  // namespace py
