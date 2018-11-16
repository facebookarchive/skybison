#include "runtime.h"

namespace python {

extern "C" PyObject* PySys_GetObject(const char* /* e */) {
  UNIMPLEMENTED("PySys_GetObject");
}

extern "C" void PySys_AddWarnOption(const wchar_t* /* s */) {
  UNIMPLEMENTED("PySys_AddWarnOption");
}

extern "C" void PySys_AddWarnOptionUnicode(PyObject* /* n */) {
  UNIMPLEMENTED("PySys_AddWarnOptionUnicode");
}

extern "C" void PySys_AddXOption(const wchar_t* /* s */) {
  UNIMPLEMENTED("PySys_AddXOption");
}

extern "C" void PySys_FormatStderr(const char* /* t */, ...) {
  UNIMPLEMENTED("PySys_FormatStderr");
}

extern "C" void PySys_FormatStdout(const char* /* t */, ...) {
  UNIMPLEMENTED("PySys_FormatStdout");
}

extern "C" PyObject* PySys_GetXOptions(void) {
  UNIMPLEMENTED("PySys_GetXOptions");
}

extern "C" int PySys_HasWarnOptions(void) {
  UNIMPLEMENTED("PySys_HasWarnOptions");
}

extern "C" void PySys_ResetWarnOptions(void) {
  UNIMPLEMENTED("PySys_ResetWarnOptions");
}

extern "C" void PySys_SetArgv(int /* c */, wchar_t** /* argv */) {
  UNIMPLEMENTED("PySys_SetArgv");
}

extern "C" void PySys_SetArgvEx(int /* c */, wchar_t** /* argv */,
                                int /* h */) {
  UNIMPLEMENTED("PySys_SetArgvEx");
}

extern "C" int PySys_SetObject(const char* /* e */, PyObject* /* v */) {
  UNIMPLEMENTED("PySys_SetObject");
}

extern "C" void PySys_SetPath(const wchar_t* /* h */) {
  UNIMPLEMENTED("PySys_SetPath");
}

extern "C" void PySys_WriteStderr(const char* /* t */, ...) {
  UNIMPLEMENTED("PySys_WriteStderr");
}

extern "C" void PySys_WriteStdout(const char* /* t */, ...) {
  UNIMPLEMENTED("PySys_WriteStdout");
}

}  // namespace python
