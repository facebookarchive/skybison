#include "runtime.h"

namespace python {

PY_EXPORT PyObject* PyImport_GetModuleDict(void) {
  UNIMPLEMENTED("PyImport_GetModuleDict");
}

PY_EXPORT PyObject* PyImport_ImportModuleLevelObject(PyObject* /* e */,
                                                     PyObject* /* s */,
                                                     PyObject* /* s */,
                                                     PyObject* /* t */,
                                                     int /* l */) {
  UNIMPLEMENTED("PyImport_ImportModuleLevelObject");
}

PY_EXPORT PyObject* PyImport_AddModule(const char* /* e */) {
  UNIMPLEMENTED("PyImport_AddModule");
}

PY_EXPORT PyObject* PyImport_AddModuleObject(PyObject* /* e */) {
  UNIMPLEMENTED("PyImport_AddModuleObject");
}

PY_EXPORT int PyImport_AppendInittab(const char* /* e */,
                                     PyObject* (*/* initfunc */)(void)) {
  UNIMPLEMENTED("PyImport_AppendInittab");
}

PY_EXPORT void PyImport_Cleanup(void) { UNIMPLEMENTED("PyImport_Cleanup"); }

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

PY_EXPORT long PyImport_GetMagicNumber(void) {
  UNIMPLEMENTED("PyImport_GetMagicNumber");
}

PY_EXPORT const char* PyImport_GetMagicTag(void) {
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

}  // namespace python
