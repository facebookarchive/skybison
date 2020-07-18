#include <cwchar>

#include "runtime.h"
#include "sys-module.h"

namespace py {

PY_EXPORT wchar_t* Py_GetExecPrefix() {
  if (Runtime::moduleSearchPath()[0] == L'\0') {
    initializeRuntimePaths(Thread::current());
  }
  return Runtime::execPrefix();
}

PY_EXPORT wchar_t* Py_GetPath() {
  if (Runtime::moduleSearchPath()[0] == L'\0') {
    initializeRuntimePaths(Thread::current());
  }
  return Runtime::moduleSearchPath();
}

PY_EXPORT wchar_t* Py_GetPrefix() {
  if (Runtime::moduleSearchPath()[0] == L'\0') {
    initializeRuntimePaths(Thread::current());
  }
  return Runtime::prefix();
}

PY_EXPORT wchar_t* Py_GetProgramFullPath() {
  UNIMPLEMENTED("Py_GetProgramFullPath");
}

PY_EXPORT wchar_t* Py_GetProgramName() { return Runtime::programName(); }

PY_EXPORT wchar_t* Py_GetPythonHome() { UNIMPLEMENTED("Py_GetPythonHome"); }

PY_EXPORT void Py_SetPath(const wchar_t* path) {
  Runtime::setPrefix(L"");
  Runtime::setExecPrefix(L"");
  Runtime::setModuleSearchPath(path != nullptr ? path : L"");
}

PY_EXPORT void Py_SetProgramName(const wchar_t* name) {
  if (name != nullptr && name[0] != L'\0') {
    Runtime::setProgramName(name);
  }
}

PY_EXPORT void Py_SetPythonHome(const wchar_t* /* home */) {
  UNIMPLEMENTED("Py_SetPythonHome");
}

}  // namespace py
