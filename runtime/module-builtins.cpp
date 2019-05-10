#include "module-builtins.h"

#include "frame.h"
#include "globals.h"
#include "object-builtins.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"
#include "trampolines-inl.h"

namespace python {

RawObject moduleGetAttribute(Thread* thread, const Module& module,
                             const Object& name_str) {
  // Note that PEP 562 adds support for data descriptors in module objects.
  // We are targeting python 3.6 for now, so we won't worry about that.

  HandleScope scope(thread);
  Object result(&scope, thread->runtime()->moduleAt(module, name_str));
  if (!result.isError()) return *result;

  // TODO(T42983855) dispatching to objectGetAttribute like this does not make
  // data properties on the type override module members.

  return objectGetAttribute(thread, module, name_str);
}

RawObject moduleSetAttr(Thread* thread, const Module& module,
                        const Object& name_str, const Object& value) {
  Runtime* runtime = thread->runtime();
  DCHECK(runtime->isInstanceOfStr(*name_str), "name must be a string");
  thread->runtime()->moduleAtPut(module, name_str, value);
  return NoneType::object();
}

int execDef(Thread* thread, const Module& module, PyModuleDef* def) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Str key(&scope, runtime->symbols()->DunderName());
  Object name_obj(&scope, runtime->moduleAt(module, key));
  if (!runtime->isInstanceOfStr(*name_obj)) {
    thread->raiseWithFmt(LayoutId::kSystemError, "nameless module");
    return -1;
  }

  ApiHandle* handle = ApiHandle::borrowedReference(thread, *module);
  if (def->m_size >= 0) {
    if (handle->cache() == nullptr) {
      handle->setCache(std::calloc(def->m_size, 1));
      if (!handle->cache()) {
        thread->raiseMemoryError();
        return -1;
      }
    }
  }

  if (def->m_slots == nullptr) {
    return 0;
  }

  Str name_str(&scope, *name_obj);
  for (PyModuleDef_Slot* cur_slot = def->m_slots;
       cur_slot != nullptr && cur_slot->slot != 0; cur_slot++) {
    switch (cur_slot->slot) {
      case Py_mod_create:
        break;
      case Py_mod_exec: {
        typedef int (*slot_func)(PyObject*);
        slot_func thunk = reinterpret_cast<slot_func>(cur_slot->value);
        if ((*thunk)(handle) != 0) {
          if (!thread->hasPendingException()) {
            thread->raiseWithFmt(
                LayoutId::kSystemError,
                "execution of module %S failed without setting an exception",
                &name_str);
          }
          return -1;
        }
        if (thread->hasPendingException()) {
          thread->clearPendingException();
          thread->raiseWithFmt(
              LayoutId::kSystemError,
              "execution of module %S failed without setting an exception",
              &name_str);
          return -1;
        }
        break;
      }
      default: {
        thread->raiseWithFmt(LayoutId::kSystemError,
                             "module %S initialized with unknown slot %d",
                             &name_str, cur_slot->slot);
        return -1;
      }
    }
  }
  return 0;
}

const BuiltinAttribute ModuleBuiltins::kAttributes[] = {
    {SymbolId::kDunderName, RawModule::kNameOffset},
    {SymbolId::kDunderDict, RawModule::kDictOffset, AttributeFlags::kReadOnly},
    {SymbolId::kSentinelId, -1},
};

const BuiltinMethod ModuleBuiltins::kBuiltinMethods[] = {
    {SymbolId::kDunderGetattribute, dunderGetattribute},
    {SymbolId::kDunderNew, dunderNew},
    {SymbolId::kDunderSetattr, dunderSetattr},
    {SymbolId::kSentinelId, nullptr},
};

RawObject ModuleBuiltins::dunderGetattribute(Thread* thread, Frame* frame,
                                             word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfModule(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kModule);
  }
  Module self(&scope, *self_obj);
  Object name(&scope, args.get(1));
  if (!runtime->isInstanceOfStr(*name)) {
    return thread->raiseWithFmt(
        LayoutId::kTypeError, "attribute name must be string, not '%T'", &name);
  }
  Object result(&scope, moduleGetAttribute(thread, self, name));
  if (result.isErrorNotFound()) {
    Object module_name(&scope, self.name());
    return thread->raiseWithFmt(LayoutId::kAttributeError,
                                "module '%S' has no attribute '%S'",
                                &module_name, &name);
  }
  return *result;
}

RawObject ModuleBuiltins::dunderNew(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  if (!args.get(0).isType()) {
    return thread->raiseWithFmt(LayoutId::kTypeError, "not a type object");
  }
  if (RawType::cast(args.get(0)).builtinBase() != LayoutId::kModule) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "not a subtype of module");
  }
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfStr(args.get(1))) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "argument must be a str instance");
  }
  HandleScope scope(thread);
  Str name(&scope, args.get(1));
  return runtime->newModule(name);
}

RawObject ModuleBuiltins::dunderSetattr(Thread* thread, Frame* frame,
                                        word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfModule(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kModule);
  }
  Module self(&scope, *self_obj);
  Object name(&scope, args.get(1));
  if (!runtime->isInstanceOfStr(*name)) {
    return thread->raiseWithFmt(
        LayoutId::kTypeError, "attribute name must be string, not '%T'", &name);
  }
  if (!name.isStr()) {
    UNIMPLEMENTED("Strict subclass of string");
  }
  Object value(&scope, args.get(2));
  return moduleSetAttr(thread, self, name, value);
}

}  // namespace python
