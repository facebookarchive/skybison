#include "runtime.h"

namespace python {

PY_EXPORT wchar_t* Py_GetExecPrefix(void) { UNIMPLEMENTED("Py_GetExecPrefix"); }

PY_EXPORT wchar_t* Py_GetPath(void) { UNIMPLEMENTED("Py_GetPath"); }

PY_EXPORT wchar_t* Py_GetPrefix(void) { UNIMPLEMENTED("Py_GetPrefix"); }

PY_EXPORT wchar_t* Py_GetProgramFullPath(void) {
  UNIMPLEMENTED("Py_GetProgramFullPath");
}

PY_EXPORT void Py_SetPath(const wchar_t* /* h */) {
  UNIMPLEMENTED("Py_SetPath");
}

}  // namespace python
