// moduleobject.c implementation

#include "cpython-func.h"
#include "handles.h"
#include "objects.h"
#include "runtime.h"

namespace python {

PY_EXPORT int PyModule_CheckExact_Func(PyObject* obj) {
  return ApiHandle::fromPyObject(obj)->asObject()->isModule();
}

PY_EXPORT int PyModule_Check_Func(PyObject* obj) {
  if (PyModule_CheckExact_Func(obj)) {
    return true;
  }
  return ApiHandle::fromPyObject(obj)->isSubclass(Thread::currentThread(),
                                                  LayoutId::kModule);
}

PY_EXPORT PyObject* PyModule_Create2(struct PyModuleDef* def, int) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Object name(&scope, runtime->newStrFromCStr(def->m_name));
  Module module(&scope, runtime->newModule(name));
  module->setDef(runtime->newIntFromCPtr(def));

  if (def->m_doc != nullptr) {
    Object doc(&scope, runtime->newStrFromCStr(def->m_doc));
    Object key(&scope, runtime->symbols()->DunderDoc());
    runtime->moduleAtPut(module, key, doc);
  }

  ApiHandle* result = ApiHandle::newReference(thread, *module);
  if (def->m_size > 0) {
    result->setCache(std::malloc(def->m_size));
  }

  // TODO(eelizondo): Check m_slots
  // TODO(eelizondo): Set md_state
  // TODO(eelizondo): Validate m_methods
  // TODO(eelizondo): Add methods

  return result;
}

PY_EXPORT PyModuleDef* PyModule_GetDef(PyObject* pymodule) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Object module_obj(&scope, ApiHandle::fromPyObject(pymodule)->asObject());
  if (!module_obj->isModule()) {
    // TODO(cshapiro): throw a TypeError
    return nullptr;
  }
  Module module(&scope, *module_obj);
  if (!module->def()->isInt()) {
    return nullptr;
  }
  Int def(&scope, module->def());
  return static_cast<PyModuleDef*>(def->asCPtr());
}

PY_EXPORT PyObject* PyModule_GetDict(PyObject* pymodule) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);

  Module module(&scope, ApiHandle::fromPyObject(pymodule)->asObject());
  return ApiHandle::newReference(thread, module->dict());
}

PY_EXPORT PyObject* PyModule_GetNameObject(PyObject* mod) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Object module_obj(&scope, ApiHandle::fromPyObject(mod)->asObject());
  if (!module_obj->isModule()) {
    // TODO(atalaba): Allow for module subclassing
    thread->raiseTypeErrorWithCStr(
        "PyModule_GetNameObject takes a Module object");
    return nullptr;
  }
  Module module(&scope, *module_obj);
  Str key(&scope, runtime->symbols()->DunderName());
  Object name(&scope, runtime->moduleAt(module, key));
  if (!runtime->isInstanceOfStr(name)) {
    thread->raiseSystemErrorWithCStr("nameless module");
    return nullptr;
  }
  return ApiHandle::newReference(thread, name);
}

PY_EXPORT void* PyModule_GetState(PyObject* mod) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);

  ApiHandle* handle = ApiHandle::fromPyObject(mod);
  Object module_obj(&scope, handle->asObject());
  if (!module_obj->isModule()) {
    // TODO(atalaba): Support module subclassing
    PyErr_BadArgument();
    return nullptr;
  }
  return handle->cache();
}

PY_EXPORT PyObject* PyModuleDef_Init(struct PyModuleDef* /* f */) {
  UNIMPLEMENTED("PyModuleDef_Init");
}

PY_EXPORT int PyModule_AddFunctions(PyObject* /* m */, PyMethodDef* /* s */) {
  UNIMPLEMENTED("PyModule_AddFunctions");
}

PY_EXPORT int PyModule_ExecDef(PyObject* module, PyModuleDef* def) {
  // TODO(atalaba): Replace with PyModule_GetName once it's ready
  PyObject* name = PyModule_GetNameObject(module);
  if (name == nullptr) return -1;

  Thread* thread = Thread::currentThread();
  if (def->m_size >= 0) {
    ApiHandle* handle = ApiHandle::fromPyObject(module);
    if (handle->cache() == nullptr) {
      handle->setCache(std::calloc(def->m_size, 1));
      if (!handle->cache()) {
        // TODO(T36797384): Change system error with PyErr_NoMemory
        thread->raiseSystemErrorWithCStr("out of memory");
        return -1;
      }
    }
  }

  if (def->m_slots == nullptr) {
    return 0;
  }

  for (PyModuleDef_Slot* cur_slot = def->m_slots; cur_slot && cur_slot->slot;
       cur_slot++) {
    switch (cur_slot->slot) {
      // TODO(T36797384): Add PyErr_Format calls
      case Py_mod_create:
        // handled in PyModule_FromDefAndSpec2
        break;
      case Py_mod_exec: {
        typedef int (*slot_func)(PyObject*);
        slot_func thunk = reinterpret_cast<slot_func>(cur_slot->value);
        if ((*thunk)(module) != 0) {
          if (!PyErr_Occurred()) {
            thread->raiseSystemErrorWithCStr(
                "execution failed without setting exception");
          }
          return -1;
        }
        if (PyErr_Occurred()) {
          thread->raiseSystemErrorWithCStr(
              "execution of module raised exception");
          return -1;
        }
        break;
      }
      default:
        thread->raiseSystemErrorWithCStr(
            "module initialized with unknown slot");
        return -1;
    }
  }
  return 0;
}

PY_EXPORT PyObject* PyModule_FromDefAndSpec2(struct PyModuleDef* /* f */,
                                             PyObject* /* c */, int /* n */) {
  UNIMPLEMENTED("PyModule_FromDefAndSpec2");
}

PY_EXPORT const char* PyModule_GetFilename(PyObject* /* m */) {
  UNIMPLEMENTED("PyModule_GetFilename");
}

PY_EXPORT PyObject* PyModule_GetFilenameObject(PyObject* pymodule) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Object module_obj(&scope, ApiHandle::fromPyObject(pymodule)->asObject());
  if (!module_obj->isModule()) {
    // TODO(atalaba): Allow for module subclassing
    // TODO(T36797384): Replace with PyErr_BadArgument
    thread->raiseTypeErrorWithCStr(
        "PyModule_GetFilenameObject takes a Module object");
    return nullptr;
  }
  Module module(&scope, *module_obj);
  Str key(&scope, runtime->symbols()->DunderFile());
  Object filename(&scope, runtime->moduleAt(module, key));
  if (!runtime->isInstanceOfStr(filename)) {
    thread->raiseSystemErrorWithCStr("module filename missing");
    return nullptr;
  }
  return ApiHandle::newReference(thread, filename);
}

PY_EXPORT const char* PyModule_GetName(PyObject* /* m */) {
  UNIMPLEMENTED("PyModule_GetName");
}

PY_EXPORT PyObject* PyModule_New(const char* c_name) {
  DCHECK(name != nullptr, "PyModule_New takes a valid string");
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Str name(&scope, runtime->newStrFromCStr(c_name));
  return ApiHandle::newReference(thread, runtime->newModule(name));
}

PY_EXPORT PyObject* PyModule_NewObject(PyObject* name) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Object name_obj(&scope, ApiHandle::fromPyObject(name)->asObject());
  Object module_obj(&scope, thread->runtime()->newModule(name_obj));
  return ApiHandle::newReference(thread, *module_obj);
}

PY_EXPORT int PyModule_SetDocString(PyObject* m, const char* doc) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Object module_obj(&scope, ApiHandle::fromPyObject(m)->asObject());
  Object uni(&scope, runtime->newStrFromCStr(doc));
  if (!uni->isStr() || !module_obj->isModule()) {
    return -1;
  }
  Module module(&scope, *module_obj);
  Object key(&scope, runtime->symbols()->DunderDoc());
  runtime->moduleAtPut(module, key, uni);
  return 0;
}

}  // namespace python
