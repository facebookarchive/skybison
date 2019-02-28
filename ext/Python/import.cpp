#include "cpython-func.h"
#include "imp-module.h"
#include "runtime.h"

namespace python {

PY_EXPORT PyObject* PyImport_GetModuleDict(void) {
  Thread* thread = Thread::currentThread();
  return ApiHandle::borrowedReference(thread, thread->runtime()->modules());
}

PY_EXPORT PyObject* PyImport_ImportModuleLevelObject(PyObject* /* e */,
                                                     PyObject* /* s */,
                                                     PyObject* /* s */,
                                                     PyObject* /* t */,
                                                     int /* l */) {
  UNIMPLEMENTED("PyImport_ImportModuleLevelObject");
}

PY_EXPORT PyObject* PyImport_AddModule(const char* name) {
  PyObject* pyname = PyUnicode_FromString(name);
  if (!pyname) return nullptr;
  PyObject* module = PyImport_AddModuleObject(pyname);
  Py_DECREF(pyname);
  return module;
}

PY_EXPORT PyObject* PyImport_AddModuleObject(PyObject* name) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Dict modules_dict(&scope, runtime->modules());
  Object name_obj(&scope, ApiHandle::fromPyObject(name)->asObject());
  Object module(&scope, runtime->dictAt(modules_dict, name_obj));
  if (module.isModule()) {
    return ApiHandle::borrowedReference(thread, *module);
  }

  Module new_module(&scope, runtime->newModule(name_obj));
  runtime->addModule(new_module);
  return ApiHandle::borrowedReference(thread, *new_module);
}

PY_EXPORT int PyImport_AppendInittab(const char* /* e */,
                                     PyObject* (*/* initfunc */)(void)) {
  UNIMPLEMENTED("PyImport_AppendInittab");
}

PY_EXPORT void PyImport_Cleanup() { UNIMPLEMENTED("PyImport_Cleanup"); }

PY_EXPORT PyObject* PyImport_ExecCodeModule(const char* /* e */,
                                            PyObject* /* o */) {
  UNIMPLEMENTED("PyImport_ExecCodeModule");
}

PY_EXPORT PyObject* PyImport_ExecCodeModuleEx(const char* /* e */,
                                              PyObject* /* o */,
                                              const char* /* e */) {
  UNIMPLEMENTED("PyImport_ExecCodeModuleEx");
}

PY_EXPORT PyObject* PyImport_ExecCodeModuleObject(PyObject* /* e */,
                                                  PyObject* /* o */,
                                                  PyObject* /* e */,
                                                  PyObject* /* e */) {
  UNIMPLEMENTED("PyImport_ExecCodeModuleObject");
}

PY_EXPORT PyObject* PyImport_ExecCodeModuleWithPathnames(const char* /* e */,
                                                         PyObject* /* o */,
                                                         const char* /* e */,
                                                         const char* /* e */) {
  UNIMPLEMENTED("PyImport_ExecCodeModuleWithPathnames");
}

PY_EXPORT long PyImport_GetMagicNumber() {
  UNIMPLEMENTED("PyImport_GetMagicNumber");
}

PY_EXPORT const char* PyImport_GetMagicTag() {
  UNIMPLEMENTED("PyImport_GetMagicTag");
}

PY_EXPORT PyObject* PyImport_Import(PyObject* /* e */) {
  UNIMPLEMENTED("PyImport_Import");
}

PY_EXPORT int PyImport_ImportFrozenModule(const char* /* e */) {
  UNIMPLEMENTED("PyImport_ImportFrozenModule");
}

PY_EXPORT int PyImport_ImportFrozenModuleObject(PyObject* /* e */) {
  UNIMPLEMENTED("PyImport_ImportFrozenModuleObject");
}

PY_EXPORT PyObject* PyImport_ImportModule(const char* /* e */) {
  UNIMPLEMENTED("PyImport_ImportModule");
}

PY_EXPORT PyObject* PyImport_ImportModuleLevel(const char* /* e */,
                                               PyObject* /* s */,
                                               PyObject* /* s */,
                                               PyObject* /* t */, int /* l */) {
  UNIMPLEMENTED("PyImport_ImportModuleLevel");
}

PY_EXPORT PyObject* PyImport_ImportModuleNoBlock(const char* /* e */) {
  UNIMPLEMENTED("PyImport_ImportModuleNoBlock");
}

PY_EXPORT PyObject* PyImport_ReloadModule(PyObject* /* m */) {
  UNIMPLEMENTED("PyImport_ReloadModule");
}

PY_EXPORT void _PyImport_AcquireLock() {
  importAcquireLock(Thread::currentThread());
}

PY_EXPORT int _PyImport_ReleaseLock() {
  return importReleaseLock(Thread::currentThread()) ? 1 : -1;
}

}  // namespace python
