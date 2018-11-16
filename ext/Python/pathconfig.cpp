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

extern "C" void Py_SetPath(const wchar_t* /* h */) {
  UNIMPLEMENTED("Py_SetPath");
}

}  // namespace python
