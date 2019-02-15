#include "module-builtins.h"

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"
#include "trampolines-inl.h"

namespace python {

int execDef(Thread* thread, const Module& module, PyModuleDef* def) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Str key(&scope, runtime->symbols()->DunderName());
  Object name_obj(&scope, runtime->moduleAt(module, key));
  if (!runtime->isInstanceOfStr(*name_obj)) {
    thread->raiseSystemErrorWithCStr("nameless module");
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
            unique_c_ptr<char> name(name_str.toCStr());
            thread->raiseSystemError(thread->runtime()->newStrFromFormat(
                "execution of module %s failed without setting an exception",
                name.get()));
          }
          return -1;
        }
        if (thread->hasPendingException()) {
          unique_c_ptr<char> name(name_str.toCStr());
          thread->raiseSystemError(thread->runtime()->newStrFromFormat(
              "execution of module %s failed without setting an exception",
              name.get()));
          return -1;
        }
        break;
      }
      default: {
        unique_c_ptr<char> name(name_str.toCStr());
        thread->raiseSystemError(thread->runtime()->newStrFromFormat(
            "module %s initialized with unknown slot %i", name.get(),
            cur_slot->slot));
        return -1;
      }
    }
  }
  return 0;
}

const BuiltinAttribute ModuleBuiltins::kAttributes[] = {
    {SymbolId::kDunderName, RawModule::kNameOffset},
    {SymbolId::kDunderDict, RawModule::kDictOffset},
};

const BuiltinMethod ModuleBuiltins::kBuiltinMethods[] = {
    {SymbolId::kDunderNew, dunderNew},
};

void ModuleBuiltins::initialize(Runtime* runtime) {
  runtime->addBuiltinType(SymbolId::kModule, LayoutId::kModule,
                          LayoutId::kObject, kAttributes,
                          View<NativeMethod>(nullptr, 0), kBuiltinMethods);
}

RawObject ModuleBuiltins::dunderNew(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  if (!args.get(0)->isType()) {
    return thread->raiseTypeErrorWithCStr("not a type object");
  }
  if (RawType::cast(args.get(0)).builtinBase() != LayoutId::kModule) {
    return thread->raiseTypeErrorWithCStr("not a subtype of module");
  }
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfStr(args.get(1))) {
    return thread->raiseTypeErrorWithCStr("argument must be a str instance");
  }
  HandleScope scope(thread);
  Str name(&scope, args.get(1));
  return runtime->newModule(name);
}

}  // namespace python
