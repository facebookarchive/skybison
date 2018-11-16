// moduleobject.c implementation

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
  return ApiHandle::fromPyObject(obj)->isSubClass(Thread::currentThread(),
                                                  LayoutId::kModule);
}

PY_EXPORT PyObject* PyModule_Create2(struct PyModuleDef* def, int) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  const char* c_name = def->m_name;
  Object name(&scope, runtime->newStrFromCStr(c_name));
  Module module(&scope, runtime->newModule(name));
  module->setDef(runtime->newIntFromCPtr(def));

  if (def->m_doc != nullptr) {
    Object doc(&scope, runtime->newStrFromCStr(def->m_doc));
    Object key(&scope, runtime->symbols()->DunderDoc());
    runtime->moduleAtPut(module, key, doc);
  }

  // TODO: Check m_slots
  // TODO: Set md_state
  // TODO: Validate m_methods
  // TODO: Add methods

  return ApiHandle::fromObject(*module);
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
  return ApiHandle::fromObject(module->dict());
}

PY_EXPORT PyObject* PyModule_GetNameObject(PyObject* /* m */) {
  UNIMPLEMENTED("PyModule_GetNameObject");
}

PY_EXPORT void* PyModule_GetState(PyObject* /* m */) {
  UNIMPLEMENTED("PyModule_GetState");
}

PY_EXPORT PyObject* PyModuleDef_Init(struct PyModuleDef* /* f */) {
  UNIMPLEMENTED("PyModuleDef_Init");
}

PY_EXPORT int PyModule_AddFunctions(PyObject* /* m */, PyMethodDef* /* s */) {
  UNIMPLEMENTED("PyModule_AddFunctions");
}

PY_EXPORT int PyModule_ExecDef(PyObject* /* e */, PyModuleDef* /* f */) {
  UNIMPLEMENTED("PyModule_ExecDef");
}

PY_EXPORT PyObject* PyModule_FromDefAndSpec2(struct PyModuleDef* /* f */,
                                             PyObject* /* c */, int /* n */) {
  UNIMPLEMENTED("PyModule_FromDefAndSpec2");
}

PY_EXPORT const char* PyModule_GetFilename(PyObject* /* m */) {
  UNIMPLEMENTED("PyModule_GetFilename");
}

PY_EXPORT PyObject* PyModule_GetFilenameObject(PyObject* /* m */) {
  UNIMPLEMENTED("PyModule_GetFilenameObject");
}

PY_EXPORT const char* PyModule_GetName(PyObject* /* m */) {
  UNIMPLEMENTED("PyModule_GetName");
}

PY_EXPORT PyObject* PyModule_New(const char* /* e */) {
  UNIMPLEMENTED("PyModule_New");
}

PY_EXPORT PyObject* PyModule_NewObject(PyObject* /* e */) {
  UNIMPLEMENTED("PyModule_NewObject");
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
