#include "runtime.h"

namespace python {

typedef enum { PyGILState_LOCKED, PyGILState_UNLOCKED } PyGILState_STATE;

PY_EXPORT int PyGILState_Check() { UNIMPLEMENTED("PyGILState_Check"); }

PY_EXPORT PyGILState_STATE PyGILState_Ensure() {
  UNIMPLEMENTED("PyGILState_Ensure");
}

PY_EXPORT PyThreadState* PyGILState_GetThisThreadState() {
  UNIMPLEMENTED("PyGILState_GetThisThreadState");
}

PY_EXPORT void PyGILState_Release(PyGILState_STATE /* e */) {
  UNIMPLEMENTED("PyGILState_Release");
}

PY_EXPORT void PyInterpreterState_Clear(PyInterpreterState* /* p */) {
  UNIMPLEMENTED("PyInterpreterState_Clear");
}

PY_EXPORT void PyInterpreterState_Delete(PyInterpreterState* /* p */) {
  UNIMPLEMENTED("PyInterpreterState_Delete");
}

PY_EXPORT int PyState_AddModule(PyObject* module, struct PyModuleDef* def) {
  DCHECK(module != nullptr, "module must not be null");
  CHECK(def != nullptr, "PyState_AddModule: Module Definition is NULL");
  DCHECK(def->m_name != nullptr, "def.m_name must not be null");

  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Str module_name(&scope, runtime->internStrFromCStr(def->m_name));
  if (!runtime->findModule(module_name).isNoneType()) {
    return 0;
  }
  // Below is the body of the port of _PyState_AddModule in CPython.
  if (def->m_slots != nullptr) {
    thread->raiseSystemErrorWithCStr(
        "PyState_AddModule called on module with slots");
    return -1;
  }
  // CPython adds the module into a separate module list to do an index lookup
  // rather than a dict for lookup efficiency. Given that we are avoiding the
  // use of the PyModuleDef.m_base.m_index value, just insert to the module dict
  Module module_obj(&scope, ApiHandle::fromPyObject(module)->asObject());
  module_obj.setDef(runtime->newIntFromCPtr(def));
  Str doc_key(&scope, runtime->symbols()->DunderDoc());
  Str doc_value(&scope, runtime->newStrFromCStr(def->m_doc));
  runtime->moduleAtPut(module_obj, doc_key, doc_value);
  runtime->addModule(module_obj);
  return 0;
}

PY_EXPORT PyObject* PyState_FindModule(struct PyModuleDef* module) {
  if (module->m_slots != nullptr) {
    return nullptr;
  }
  Py_ssize_t index = module->m_base.m_index;
  if (index == 0) {
    return nullptr;
  }
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Str module_name(&scope, runtime->internStrFromCStr(module->m_name));
  Object module_obj(&scope, runtime->findModule(module_name));
  if (module_obj.isNoneType()) {
    return nullptr;
  }
  return ApiHandle::borrowedReference(thread, *module_obj);
}

PY_EXPORT int PyState_RemoveModule(struct PyModuleDef* /* f */) {
  UNIMPLEMENTED("PyState_RemoveModule");
}

PY_EXPORT void PyThreadState_Clear(PyThreadState* /* e */) {
  UNIMPLEMENTED("PyThreadState_Clear");
}

PY_EXPORT void PyThreadState_Delete(PyThreadState* /* e */) {
  UNIMPLEMENTED("PyThreadState_Delete");
}

PY_EXPORT void PyThreadState_DeleteCurrent() {
  UNIMPLEMENTED("PyThreadState_DeleteCurrent");
}

PY_EXPORT PyThreadState* PyThreadState_Get() {
  UNIMPLEMENTED("PyThreadState_Get");
}

PY_EXPORT PyObject* PyThreadState_GetDict() {
  UNIMPLEMENTED("PyThreadState_GetDict");
}

PY_EXPORT PyThreadState* PyThreadState_New(PyInterpreterState* /* p */) {
  UNIMPLEMENTED("PyThreadState_New");
}

PY_EXPORT int PyThreadState_SetAsyncExc(long /* d */, PyObject* /* c */) {
  UNIMPLEMENTED("PyThreadState_SetAsyncExc");
}

PY_EXPORT PyThreadState* PyThreadState_Swap(PyThreadState* /* s */) {
  UNIMPLEMENTED("PyThreadState_Swap");
}

PY_EXPORT int _PyState_AddModule(PyObject* /* e */,
                                 struct PyModuleDef* /* f */) {
  UNIMPLEMENTED("_PyState_AddModule");
}

PY_EXPORT void _PyThreadState_Init(PyThreadState* /* e */) {
  UNIMPLEMENTED("_PyThreadState_Init");
}

PY_EXPORT PyThreadState* _PyThreadState_Prealloc(PyInterpreterState* /* p */) {
  UNIMPLEMENTED("_PyThreadState_Prealloc");
}

PY_EXPORT PyInterpreterState* PyInterpreterState_Head() {
  UNIMPLEMENTED("PyInterpreterState_Head");
}

PY_EXPORT PyInterpreterState* PyInterpreterState_Next(
    PyInterpreterState* /* p */) {
  UNIMPLEMENTED("PyInterpreterState_Next");
}

PY_EXPORT PyThreadState* PyInterpreterState_ThreadHead(
    PyInterpreterState* /* p */) {
  UNIMPLEMENTED("PyInterpreterState_ThreadHead");
}

PY_EXPORT void _PyState_ClearModules() {
  UNIMPLEMENTED("_PyState_ClearModules");
}

}  // namespace python
