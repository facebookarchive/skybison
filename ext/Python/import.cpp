#include "runtime.h"

namespace python {

extern "C" PyObject* PyImport_GetModuleDict(void) {
  UNIMPLEMENTED("PyImport_GetModuleDict");
}

extern "C" PyObject* PyImport_ImportModuleLevelObject(PyObject* /* e */,
                                                      PyObject* /* s */,
                                                      PyObject* /* s */,
                                                      PyObject* /* t */,
                                                      int /* l */) {
  UNIMPLEMENTED("PyImport_ImportModuleLevelObject");
}

extern "C" PyObject* PyImport_AddModule(const char* /* e */) {
  UNIMPLEMENTED("PyImport_AddModule");
}

extern "C" PyObject* PyImport_AddModuleObject(PyObject* /* e */) {
  UNIMPLEMENTED("PyImport_AddModuleObject");
}

extern "C" int PyImport_AppendInittab(const char* /* e */,
                                      PyObject* (*/* initfunc */)(void)) {
  UNIMPLEMENTED("PyImport_AppendInittab");
}

extern "C" void PyImport_Cleanup(void) { UNIMPLEMENTED("PyImport_Cleanup"); }

extern "C" PyObject* PyImport_ExecCodeModule(const char* /* e */,
                                             PyObject* /* o */) {
  UNIMPLEMENTED("PyImport_ExecCodeModule");
}

extern "C" PyObject* PyImport_ExecCodeModuleEx(const char* /* e */,
                                               PyObject* /* o */,
                                               const char* /* e */) {
  UNIMPLEMENTED("PyImport_ExecCodeModuleEx");
}

extern "C" PyObject* PyImport_ExecCodeModuleObject(PyObject* /* e */,
                                                   PyObject* /* o */,
                                                   PyObject* /* e */,
                                                   PyObject* /* e */) {
  UNIMPLEMENTED("PyImport_ExecCodeModuleObject");
}

extern "C" PyObject* PyImport_ExecCodeModuleWithPathnames(const char* /* e */,
                                                          PyObject* /* o */,
                                                          const char* /* e */,
                                                          const char* /* e */) {
  UNIMPLEMENTED("PyImport_ExecCodeModuleWithPathnames");
}

extern "C" long PyImport_GetMagicNumber(void) {
  UNIMPLEMENTED("PyImport_GetMagicNumber");
}

extern "C" const char* PyImport_GetMagicTag(void) {
  UNIMPLEMENTED("PyImport_GetMagicTag");
}

extern "C" PyObject* PyImport_Import(PyObject* /* e */) {
  UNIMPLEMENTED("PyImport_Import");
}

extern "C" int PyImport_ImportFrozenModule(const char* /* e */) {
  UNIMPLEMENTED("PyImport_ImportFrozenModule");
}

extern "C" int PyImport_ImportFrozenModuleObject(PyObject* /* e */) {
  UNIMPLEMENTED("PyImport_ImportFrozenModuleObject");
}

extern "C" PyObject* PyImport_ImportModule(const char* /* e */) {
  UNIMPLEMENTED("PyImport_ImportModule");
}

extern "C" PyObject* PyImport_ImportModuleLevel(const char* /* e */,
                                                PyObject* /* s */,
                                                PyObject* /* s */,
                                                PyObject* /* t */,
                                                int /* l */) {
  UNIMPLEMENTED("PyImport_ImportModuleLevel");
}

extern "C" PyObject* PyImport_ImportModuleNoBlock(const char* /* e */) {
  UNIMPLEMENTED("PyImport_ImportModuleNoBlock");
}

extern "C" PyObject* PyImport_ReloadModule(PyObject* /* m */) {
  UNIMPLEMENTED("PyImport_ReloadModule");
}

}  // namespace python
