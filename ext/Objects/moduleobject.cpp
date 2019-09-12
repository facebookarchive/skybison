// moduleobject.c implementation

#include "builtins-module.h"
#include "capi-handles.h"
#include "cpython-data.h"
#include "cpython-func.h"
#include "function-builtins.h"
#include "function-utils.h"
#include "handles.h"
#include "module-builtins.h"
#include "objects.h"
#include "runtime.h"
#include "trampolines.h"

namespace python {

PY_EXPORT int PyModule_CheckExact_Func(PyObject* obj) {
  return ApiHandle::fromPyObject(obj)->asObject().isModule();
}

PY_EXPORT int PyModule_Check_Func(PyObject* obj) {
  return Thread::current()->runtime()->isInstanceOfModule(
      ApiHandle::fromPyObject(obj)->asObject());
}

static PyObject* moduleDefInit(Thread* thread, struct PyModuleDef* def) {
  if (def->m_base.m_index == 0) {
    def->m_base.m_index = thread->runtime()->nextModuleIndex();
  }
  return reinterpret_cast<PyObject*>(def);
}

PY_EXPORT PyObject* PyModule_Create2(struct PyModuleDef* def, int) {
  Thread* thread = Thread::current();
  if (moduleDefInit(thread, def) == nullptr) {
    return nullptr;
  }
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object name(&scope, runtime->newStrFromCStr(def->m_name));
  Module module(&scope, runtime->newModule(name));
  module.setDef(runtime->newIntFromCPtr(def));

  if (def->m_methods != nullptr) {
    for (PyMethodDef* fdef = def->m_methods; fdef->ml_name != nullptr; fdef++) {
      if (fdef->ml_flags & METH_CLASS || fdef->ml_flags & METH_STATIC) {
        thread->raiseWithFmt(
            LayoutId::kValueError,
            "module functions cannot set METH_CLASS or METH_STATIC");
        return nullptr;
      }
      Function function(
          &scope, functionFromModuleMethodDef(
                      thread, fdef->ml_name, bit_cast<void*>(fdef->ml_meth),
                      fdef->ml_doc, methodTypeFromMethodFlags(fdef->ml_flags)));

      function.setModule(*module);
      Object function_name(&scope, function.name());
      moduleAtPut(thread, module, function_name, function);
    }
  }

  if (def->m_doc != nullptr) {
    Object doc(&scope, runtime->newStrFromCStr(def->m_doc));
    Object key(&scope, runtime->symbols()->DunderDoc());
    moduleAtPut(thread, module, key, doc);
  }

  ApiHandle* result = ApiHandle::newReference(thread, *module);
  if (def->m_size > 0) {
    result->setCache(std::malloc(def->m_size));
  }

  // TODO(eelizondo): Check m_slots
  // TODO(eelizondo): Set md_state

  return result;
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
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  Module module(&scope, ApiHandle::fromPyObject(pymodule)->asObject());
  return ApiHandle::newReference(thread, module.dict());
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
  Str key(&scope, runtime->symbols()->DunderName());
  Object name(&scope, moduleAt(thread, module, key));
  if (!runtime->isInstanceOfStr(*name)) {
    thread->raiseWithFmt(LayoutId::kSystemError, "nameless module");
    return nullptr;
  }
  return ApiHandle::newReference(thread, *name);
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
  return handle->cache();
}

PY_EXPORT PyObject* PyModuleDef_Init(struct PyModuleDef* def) {
  return moduleDefInit(Thread::current(), def);
}

PY_EXPORT int PyModule_AddFunctions(PyObject* /* m */, PyMethodDef* /* s */) {
  UNIMPLEMENTED("PyModule_AddFunctions");
}

PY_EXPORT int PyModule_ExecDef(PyObject* pymodule, PyModuleDef* def) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object module_obj(&scope, ApiHandle::fromPyObject(pymodule)->asObject());
  if (!thread->runtime()->isInstanceOfModule(*module_obj)) {
    return -1;
  }
  Module module(&scope, *module_obj);
  return execDef(thread, module, def);
}

PY_EXPORT PyObject* PyModule_FromDefAndSpec2(struct PyModuleDef* /* f */,
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
  Str key(&scope, runtime->symbols()->DunderFile());
  Object filename(&scope, moduleAt(thread, module, key));
  if (!runtime->isInstanceOfStr(*filename)) {
    thread->raiseWithFmt(LayoutId::kSystemError, "module filename missing");
    return nullptr;
  }
  return ApiHandle::newReference(thread, *filename);
}

PY_EXPORT const char* PyModule_GetName(PyObject* pymodule) {
  PyObject* name = PyModule_GetNameObject(pymodule);
  if (name == nullptr) return nullptr;
  Py_DECREF(name);
  return PyUnicode_AsUTF8(name);
}

PY_EXPORT PyObject* PyModule_New(const char* c_name) {
  DCHECK(c_name != nullptr, "PyModule_New takes a valid string");
  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Str name(&scope, runtime->newStrFromCStr(c_name));
  return ApiHandle::newReference(thread, runtime->newModule(name));
}

PY_EXPORT PyObject* PyModule_NewObject(PyObject* name) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object name_obj(&scope, ApiHandle::fromPyObject(name)->asObject());
  Object module_obj(&scope, thread->runtime()->newModule(name_obj));
  return ApiHandle::newReference(thread, *module_obj);
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
  Object name(&scope, runtime->symbols()->DunderDoc());
  if (thread->invokeMethod3(module, SymbolId::kDunderSetattr, name, uni)
          .isErrorException()) {
    return -1;
  }
  return 0;
}

}  // namespace python
