// moduleobject.c implementation
#include "cpython-data.h"
#include "cpython-func.h"

#include "api-handle.h"
#include "builtins-module.h"
#include "function-builtins.h"
#include "function-utils.h"
#include "handles.h"
#include "module-builtins.h"
#include "objects.h"
#include "os.h"
#include "runtime.h"
#include "trampolines.h"

namespace py {

using ExtensionModuleInitFunc = PyObject* (*)();
extern struct _inittab _PyImport_Inittab[];
struct _inittab* PyImport_Inittab = _PyImport_Inittab;

PY_EXPORT int PyModule_CheckExact_Func(PyObject* obj) {
  return ApiHandle::fromPyObject(obj)->asObject().isModule();
}

PY_EXPORT int PyModule_Check_Func(PyObject* obj) {
  return Thread::current()->runtime()->isInstanceOfModule(
      ApiHandle::fromPyObject(obj)->asObject());
}

static void moduleDefInit(PyModuleDef* def) {
  if (def->m_base.m_index != 0) return;
  def->m_base.m_index = Runtime::nextModuleIndex();
}

PY_EXPORT PyObject* PyModule_Create2(PyModuleDef* def, int) {
  moduleDefInit(def);
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object module_name(&scope, Runtime::internStrFromCStr(thread, def->m_name));
  Module module(&scope, runtime->newModule(module_name));
  module.setDef(runtime->newIntFromCPtr(def));

  if (def->m_methods != nullptr) {
    Object function_name(&scope, NoneType::object());
    Object function(&scope, NoneType::object());
    for (PyMethodDef* method = def->m_methods; method->ml_name != nullptr;
         method++) {
      if (method->ml_flags & METH_CLASS || method->ml_flags & METH_STATIC) {
        thread->raiseWithFmt(
            LayoutId::kValueError,
            "module functions cannot set METH_CLASS or METH_STATIC");
        return nullptr;
      }
      function_name = Runtime::internStrFromCStr(thread, method->ml_name);
      function =
          newCFunction(thread, method, function_name, module, module_name);
      moduleAtPut(thread, module, function_name, function);
    }
  }

  if (def->m_doc != nullptr) {
    Object doc(&scope, runtime->newStrFromCStr(def->m_doc));
    moduleAtPutById(thread, module, ID(__doc__), doc);
  }

  Py_ssize_t m_size = def->m_size;
  void* state = nullptr;
  if (m_size > 0) {
    state = std::calloc(1, m_size);
    if (state == nullptr) {
      PyErr_NoMemory();
      return nullptr;
    }
  }
  module.setState(runtime->newIntFromCPtr(state));

  // TODO(eelizondo): Check m_slots
  // TODO(eelizondo): Set md_state
  return ApiHandle::newReference(runtime, *module);
}

PY_EXPORT PyModuleDef* PyModule_GetDef(PyObject* pymodule) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object module_obj(&scope, ApiHandle::fromPyObject(pymodule)->asObject());
  if (!thread->runtime()->isInstanceOfModule(*module_obj)) {
    thread->raiseBadArgument();
    return nullptr;
  }
  Module module(&scope, *module_obj);
  Int def(&scope, module.def());
  return static_cast<PyModuleDef*>(def.asCPtr());
}

PY_EXPORT PyObject* PyModule_GetDict(PyObject* pymodule) {
  // Return the module_proxy object. Note that this is not a `PyDict` instance
  // so contrary to cpython it will not work with `PyDict_xxx` functions. It
  // does work with PyEval_EvalCode.
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object module_obj(&scope, ApiHandle::fromPyObject(pymodule)->asObject());
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfModule(*module_obj)) {
    thread->raiseBadArgument();
    return nullptr;
  }
  Module module(&scope, *module_obj);
  return ApiHandle::borrowedReference(runtime, module.moduleProxy());
}

PY_EXPORT PyObject* PyModule_GetNameObject(PyObject* mod) {
  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Object module_obj(&scope, ApiHandle::fromPyObject(mod)->asObject());
  if (!runtime->isInstanceOfModule(*module_obj)) {
    thread->raiseBadArgument();
    return nullptr;
  }
  Module module(&scope, *module_obj);
  Object name(&scope, moduleAtById(thread, module, ID(__name__)));
  if (!runtime->isInstanceOfStr(*name)) {
    thread->raiseWithFmt(LayoutId::kSystemError, "nameless module");
    return nullptr;
  }
  return ApiHandle::newReference(runtime, *name);
}

PY_EXPORT void* PyModule_GetState(PyObject* mod) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  ApiHandle* handle = ApiHandle::fromPyObject(mod);
  Object module_obj(&scope, handle->asObject());
  if (!thread->runtime()->isInstanceOfModule(*module_obj)) {
    thread->raiseBadArgument();
    return nullptr;
  }
  Module module(&scope, *module_obj);
  return Int::cast(module.state()).asCPtr();
}

PY_EXPORT PyObject* PyModuleDef_Init(PyModuleDef* def) {
  moduleDefInit(def);
  return reinterpret_cast<PyObject*>(def);
}

PY_EXPORT int PyModule_AddFunctions(PyObject* /* m */, PyMethodDef* /* s */) {
  UNIMPLEMENTED("PyModule_AddFunctions");
}

word moduleExecDef(Thread* thread, const Module& module, PyModuleDef* def) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Object name_obj(&scope, moduleAtById(thread, module, ID(__name__)));
  if (!runtime->isInstanceOfStr(*name_obj)) {
    thread->raiseWithFmt(LayoutId::kSystemError, "nameless module");
    return -1;
  }

  ApiHandle* handle = ApiHandle::borrowedReference(runtime, *module);
  if (def->m_size >= 0) {
    if (handle->cache(runtime) == nullptr) {
      DCHECK(handle->isBorrowedNoImmediate(), "handle must be marked borrowed");
      handle->setCache(runtime, std::calloc(def->m_size, 1));
      if (!handle->cache(runtime)) {
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

PY_EXPORT int PyModule_ExecDef(PyObject* pymodule, PyModuleDef* def) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object module_obj(&scope, ApiHandle::fromPyObject(pymodule)->asObject());
  if (!thread->runtime()->isInstanceOfModule(*module_obj)) {
    return -1;
  }
  Module module(&scope, *module_obj);
  return moduleExecDef(thread, module, def);
}

PY_EXPORT PyObject* PyModule_FromDefAndSpec2(PyModuleDef* /* f */,
                                             PyObject* /* c */, int /* n */) {
  UNIMPLEMENTED("PyModule_FromDefAndSpec2");
}

PY_EXPORT const char* PyModule_GetFilename(PyObject* /* m */) {
  UNIMPLEMENTED("PyModule_GetFilename");
}

PY_EXPORT PyObject* PyModule_GetFilenameObject(PyObject* pymodule) {
  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Object module_obj(&scope, ApiHandle::fromPyObject(pymodule)->asObject());
  if (!runtime->isInstanceOfModule(*module_obj)) {
    thread->raiseBadArgument();
    return nullptr;
  }
  Module module(&scope, *module_obj);
  Object filename(&scope, moduleAtById(thread, module, ID(__file__)));
  if (!runtime->isInstanceOfStr(*filename)) {
    thread->raiseWithFmt(LayoutId::kSystemError, "module filename missing");
    return nullptr;
  }
  return ApiHandle::newReference(runtime, *filename);
}

PY_EXPORT const char* PyModule_GetName(PyObject* pymodule) {
  PyObject* name = PyModule_GetNameObject(pymodule);
  if (name == nullptr) return nullptr;
  const char* result = PyUnicode_AsUTF8(name);
  Py_DECREF(name);
  return result;
}

PY_EXPORT PyObject* PyModule_New(const char* c_name) {
  DCHECK(c_name != nullptr, "PyModule_New takes a valid string");
  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Str name(&scope, runtime->newStrFromCStr(c_name));
  return ApiHandle::newReference(runtime, runtime->newModule(name));
}

PY_EXPORT PyObject* PyModule_NewObject(PyObject* name) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object name_obj(&scope, ApiHandle::fromPyObject(name)->asObject());
  Runtime* runtime = thread->runtime();
  return ApiHandle::newReference(runtime, runtime->newModule(name_obj));
}

PY_EXPORT int PyModule_SetDocString(PyObject* m, const char* doc) {
  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Object module(&scope, ApiHandle::fromPyObject(m)->asObject());
  Object uni(&scope, runtime->newStrFromCStr(doc));
  if (!uni.isStr()) {
    return -1;
  }
  Object name(&scope, runtime->symbols()->at(ID(__doc__)));
  if (thread->invokeMethod3(module, ID(__setattr__), name, uni)
          .isErrorException()) {
    return -1;
  }
  return 0;
}

PY_EXPORT PyTypeObject* PyModule_Type_Ptr() {
  Runtime* runtime = Thread::current()->runtime();
  return reinterpret_cast<PyTypeObject*>(ApiHandle::borrowedReference(
      runtime, runtime->typeAt(LayoutId::kModule)));
}

static RawObject initializeModule(Thread* thread, ExtensionModuleInitFunc init,
                                  const Str& name) {
  PyObject* module_or_def = (*init)();
  if (module_or_def == nullptr) {
    if (!thread->hasPendingException()) {
      return thread->raiseWithFmt(
          LayoutId::kSystemError,
          "Initialization of '%S' failed without raising", &name);
    }
    return Error::exception();
  }
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object module_obj(&scope, ApiHandle::fromPyObject(module_or_def)->asObject());
  if (!runtime->isInstanceOfModule(*module_obj)) {
    // TODO(T39542987): Enable multi-phase module initialization
    UNIMPLEMENTED("Multi-phase module initialization");
  }

  Module module(&scope, *module_obj);
  PyModuleDef* def =
      static_cast<PyModuleDef*>(Int::cast(module.def()).asCPtr());
  if (_PyState_AddModule(module_or_def, def) < 0) {
    return Error::exception();
  }
  return *module;
}

RawObject moduleLoadDynamicExtension(Thread* thread, const Str& name,
                                     const Str& path) {
  unique_c_ptr<char> path_cstr(path.toCStr());
  const char* error_msg;
  void* handle =
      OS::openSharedObject(path_cstr.get(), OS::kRtldNow, &error_msg);
  if (handle == nullptr) {
    return thread->raiseWithFmt(
        LayoutId::kImportError, "dlerror: '%s' importing: '%S' from '%S'",
        error_msg != nullptr ? error_msg : "", &name, &path);
  }

  // Resolve PyInit_xxx symbol.
  unique_c_ptr<char> name_cstr(name.toCStr());
  char init_name[256];
  std::snprintf(init_name, sizeof(init_name), "PyInit_%s", name_cstr.get());
  ExtensionModuleInitFunc init = reinterpret_cast<ExtensionModuleInitFunc>(
      OS::sharedObjectSymbolAddress(handle, init_name, /*error_msg=*/nullptr));
  if (init == nullptr) {
    return thread->raiseWithFmt(LayoutId::kImportError,
                                "dlsym error: dynamic module '%S' does not "
                                "define export function: '%s'",
                                &name, init_name);
  }

  return initializeModule(thread, init, name);
}

static word inittabIndex(const Str& name) {
  for (word i = 0; PyImport_Inittab[i].name != nullptr; i++) {
    if (name.equalsCStr(PyImport_Inittab[i].name)) {
      return i;
    }
  }
  return -1;
}

bool isBuiltinExtensionModule(const Str& name) {
  return inittabIndex(name) >= 0;
}

RawObject moduleInitBuiltinExtension(Thread* thread, const Str& name) {
  word index = inittabIndex(name);
  if (index < 0) return Error::notFound();
  return initializeModule(thread, PyImport_Inittab[index].initfunc, name);
}

PY_EXPORT int PyImport_AppendInittab(const char* name,
                                     PyObject* (*initfunc)(void)) {
  word old_inittab_length = 0;
  while (PyImport_Inittab[old_inittab_length].name != nullptr) {
    old_inittab_length++;
  }
  // The copy needs 2 more slots than the non-null entries:
  // 1 for the sentinel at the end, and 1 for the inittab we're adding.
  word new_inittab_length = old_inittab_length + 2;
  struct _inittab* inittab_copy = static_cast<struct _inittab*>(
      std::malloc(sizeof(struct _inittab) * (new_inittab_length)));
  if (inittab_copy == nullptr) {
    return -1;
  }
  int i = 0;
  for (; i < old_inittab_length; i++) {
    inittab_copy[i] = PyImport_Inittab[i];
  }
  inittab_copy[i++] = {name, initfunc};
  inittab_copy[i++] = {nullptr, nullptr};
  DCHECK(i == new_inittab_length,
         "length mismatch when populating new inittab");

  // Only attempt to deallocate PyImport_Inittab if it was heap-allocated.
  struct _inittab* old_inittab_pointer = PyImport_Inittab;
  PyImport_Inittab = inittab_copy;
  if (old_inittab_pointer != _PyImport_Inittab) {
    std::free(old_inittab_pointer);
  }
  return 0;
}

}  // namespace py
