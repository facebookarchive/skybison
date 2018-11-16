#include "runtime.h"

namespace python {

extern "C" void PyErr_SetString(PyObject*, const char* message) {
  // TODO(T32875119): Set the correct exception type.
  Thread::currentThread()->throwSystemErrorFromCStr(message);
}

extern "C" PyObject* PyErr_Occurred(void) {
  Thread* thread = Thread::currentThread();
  if (thread->pendingException()->isNone()) {
    return nullptr;
  }
  return ApiHandle::fromObject(thread->pendingException())->asPyObject();
}

extern "C" PyObject* PyErr_Format(PyObject*, const char*, ...) {
  UNIMPLEMENTED("PyErr_Format");
}

extern "C" void PyErr_Clear(void) { UNIMPLEMENTED("PyErr_Clear"); }

extern "C" int PyErr_BadArgument(void) { UNIMPLEMENTED("PyErr_BadArgument"); }

extern "C" PyObject* PyErr_NoMemory(void) { UNIMPLEMENTED("PyErr_NoMemory"); }

extern "C" PyObject* _PyErr_FormatFromCause(PyObject*, const char*, ...) {
  UNIMPLEMENTED("_PyErr_FormatFromCause");
}

}  // namespace python
