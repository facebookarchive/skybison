#include "runtime.h"

namespace python {

struct PyThreadState;
typedef void (*PyOS_sighandler_t)(int);

extern "C" void Py_Initialize(void) { new Runtime; }

extern "C" int Py_FinalizeEx(void) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  delete runtime;
  return 0;
}

extern "C" PyOS_sighandler_t PyOS_getsig(int /* g */) {
  UNIMPLEMENTED("PyOS_getsig");
}

extern "C" PyOS_sighandler_t PyOS_setsig(int /* g */,
                                         PyOS_sighandler_t /* r */) {
  UNIMPLEMENTED("PyOS_setsig");
}

extern "C" int Py_AtExit(void (*/* func */)(void)) {
  UNIMPLEMENTED("Py_AtExit");
}

extern "C" void Py_EndInterpreter(PyThreadState* /* e */) {
  UNIMPLEMENTED("Py_EndInterpreter");
}

extern "C" void Py_Exit(int /* s */) { UNIMPLEMENTED("Py_Exit"); }

extern "C" void Py_FatalError(const char* /* g */) {
  UNIMPLEMENTED("Py_FatalError");
}

extern "C" void Py_Finalize(void) { UNIMPLEMENTED("Py_Finalize"); }

extern "C" void Py_InitializeEx(int /* s */) {
  UNIMPLEMENTED("Py_InitializeEx");
}

extern "C" int Py_IsInitialized(void) { UNIMPLEMENTED("Py_IsInitialized"); }

extern "C" PyThreadState* Py_NewInterpreter(void) {
  UNIMPLEMENTED("Py_NewInterpreter");
}

}  // namespace python
