#include "runtime.h"

#include <cstdarg>

#include "sys-module.h"

namespace python {

PY_EXPORT PyObject* PySys_GetObject(const char* /* e */) {
  UNIMPLEMENTED("PySys_GetObject");
}

PY_EXPORT void PySys_AddWarnOption(const wchar_t* /* s */) {
  UNIMPLEMENTED("PySys_AddWarnOption");
}

PY_EXPORT void PySys_AddWarnOptionUnicode(PyObject* /* n */) {
  UNIMPLEMENTED("PySys_AddWarnOptionUnicode");
}

PY_EXPORT void PySys_AddXOption(const wchar_t* /* s */) {
  UNIMPLEMENTED("PySys_AddXOption");
}

PY_EXPORT void PySys_FormatStderr(const char* /* t */, ...) {
  UNIMPLEMENTED("PySys_FormatStderr");
}

PY_EXPORT void PySys_FormatStdout(const char* /* t */, ...) {
  UNIMPLEMENTED("PySys_FormatStdout");
}

PY_EXPORT PyObject* PySys_GetXOptions() { UNIMPLEMENTED("PySys_GetXOptions"); }

PY_EXPORT int PySys_HasWarnOptions() { UNIMPLEMENTED("PySys_HasWarnOptions"); }

PY_EXPORT void PySys_ResetWarnOptions() {
  UNIMPLEMENTED("PySys_ResetWarnOptions");
}

PY_EXPORT void PySys_SetArgv(int /* c */, wchar_t** /* argv */) {
  UNIMPLEMENTED("PySys_SetArgv");
}

PY_EXPORT void PySys_SetArgvEx(int /* c */, wchar_t** /* argv */, int /* h */) {
  UNIMPLEMENTED("PySys_SetArgvEx");
}

PY_EXPORT int PySys_SetObject(const char* /* e */, PyObject* /* v */) {
  UNIMPLEMENTED("PySys_SetObject");
}

PY_EXPORT void PySys_SetPath(const wchar_t* /* h */) {
  UNIMPLEMENTED("PySys_SetPath");
}

PY_EXPORT void PySys_WriteStderr(const char* format, ...) {
  va_list va;
  va_start(va, format);
  writeStderrV(Thread::currentThread(), format, va);
  va_end(va);
}

PY_EXPORT void PySys_WriteStdout(const char* format, ...) {
  va_list va;
  va_start(va, format);
  writeStdoutV(Thread::currentThread(), format, va);
  va_end(va);
}

}  // namespace python
