#include "runtime.h"

namespace python {

PY_EXPORT wchar_t* Py_GetExecPrefix() { UNIMPLEMENTED("Py_GetExecPrefix"); }

PY_EXPORT wchar_t* Py_GetPath() { UNIMPLEMENTED("Py_GetPath"); }

PY_EXPORT wchar_t* Py_GetPrefix() { UNIMPLEMENTED("Py_GetPrefix"); }

PY_EXPORT wchar_t* Py_GetProgramFullPath() {
  UNIMPLEMENTED("Py_GetProgramFullPath");
}

PY_EXPORT void Py_SetPath(const wchar_t* /* h */) {
  UNIMPLEMENTED("Py_SetPath");
}

}  // namespace python
