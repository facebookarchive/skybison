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

PY_EXPORT void Py_SetPath(const wchar_t* path) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object empty_str(&scope, Str::empty());
  Runtime::setPrefix(thread, empty_str);
  Runtime::setExecPrefix(thread, empty_str);
  if (path == nullptr) {
    Runtime::setModuleSearchPathFromWCstr(L"");
  } else {
    Runtime::setModuleSearchPathFromWCstr(path);
  }
}

}  // namespace py
