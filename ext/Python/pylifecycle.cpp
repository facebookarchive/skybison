#include <cstdio>

#include "runtime.h"

int Py_BytesWarningFlag = 0;
int Py_DebugFlag = 0;
int Py_DontWriteBytecodeFlag = 0;
int Py_FrozenFlag = 0;
int Py_HashRandomizationFlag = 0;
int Py_IgnoreEnvironmentFlag = 0;
int Py_InspectFlag = 0;
int Py_InteractiveFlag = 0;
int Py_IsolatedFlag = 0;
int Py_NoSiteFlag = 0;
int Py_NoUserSiteDirectory = 0;
int Py_OptimizeFlag = 0;
int Py_QuietFlag = 0;
int Py_UnbufferedStdioFlag = 0;
int Py_UseClassExceptionsFlag = 1;
int Py_VerboseFlag = 0;

namespace python {

PY_EXPORT void Py_Initialize() { new Runtime; }

PY_EXPORT int Py_FinalizeEx() {
  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  delete runtime;
  return 0;
}

PY_EXPORT PyOS_sighandler_t PyOS_getsig(int /* g */) {
  UNIMPLEMENTED("PyOS_getsig");
}

PY_EXPORT PyOS_sighandler_t PyOS_setsig(int /* g */,
                                        PyOS_sighandler_t /* r */) {
  UNIMPLEMENTED("PyOS_setsig");
}

PY_EXPORT int Py_AtExit(void (*/* func */)(void)) {
  UNIMPLEMENTED("Py_AtExit");
}

PY_EXPORT void Py_EndInterpreter(PyThreadState* /* e */) {
  UNIMPLEMENTED("Py_EndInterpreter");
}

PY_EXPORT void Py_Exit(int /* s */) { UNIMPLEMENTED("Py_Exit"); }

PY_EXPORT void Py_FatalError(const char* msg) {
  std::fprintf(stderr, "Fatal Python error: %s\n", msg);
  // TODO(T39151288): Correctly print exceptions when the current thread holds
  // the GIL
  std::abort();
}

PY_EXPORT void Py_Finalize() { UNIMPLEMENTED("Py_Finalize"); }

PY_EXPORT void Py_InitializeEx(int /* s */) {
  UNIMPLEMENTED("Py_InitializeEx");
}

PY_EXPORT int Py_IsInitialized() { UNIMPLEMENTED("Py_IsInitialized"); }

PY_EXPORT PyThreadState* Py_NewInterpreter() {
  UNIMPLEMENTED("Py_NewInterpreter");
}

PY_EXPORT wchar_t* Py_GetProgramName() { UNIMPLEMENTED("Py_GetProgramName"); }

PY_EXPORT wchar_t* Py_GetPythonHome() { UNIMPLEMENTED("Py_GetPythonHome"); }

PY_EXPORT void Py_SetProgramName(const wchar_t* /* e */) {
  UNIMPLEMENTED("Py_SetProgramName");
}

PY_EXPORT void Py_SetPythonHome(const wchar_t* /* e */) {
  UNIMPLEMENTED("Py_SetPythonHome");
}

}  // namespace python
