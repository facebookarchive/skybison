#include "runtime.h"

namespace python {

extern "C" wchar_t* Py_GetExecPrefix(void) {
  UNIMPLEMENTED("Py_GetExecPrefix");
}

extern "C" wchar_t* Py_GetPath(void) { UNIMPLEMENTED("Py_GetPath"); }

extern "C" wchar_t* Py_GetPrefix(void) { UNIMPLEMENTED("Py_GetPrefix"); }

extern "C" wchar_t* Py_GetProgramFullPath(void) {
  UNIMPLEMENTED("Py_GetProgramFullPath");
}

extern "C" wchar_t* Py_GetProgramName(void) {
  UNIMPLEMENTED("Py_GetProgramName");
}

extern "C" wchar_t* Py_GetPythonHome(void) {
  UNIMPLEMENTED("Py_GetPythonHome");
}

extern "C" void Py_SetPath(const wchar_t* /* h */) {
  UNIMPLEMENTED("Py_SetPath");
}

extern "C" void Py_SetProgramName(const wchar_t* /* e */) {
  UNIMPLEMENTED("Py_SetProgramName");
}

extern "C" void Py_SetPythonHome(const wchar_t* /* e */) {
  UNIMPLEMENTED("Py_SetPythonHome");
}

}  // namespace python
