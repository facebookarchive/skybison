#include "runtime.h"

namespace python {

extern "C" void PyErr_SetString(PyObject*, const char* message) {
  // TODO(T32875119): Set the correct exception type.
  Thread::currentThread()->throwSystemErrorFromCString(message);
}

extern "C" PyObject* PyErr_Occurred(void) {
  Thread* thread = Thread::currentThread();
  if (thread->pendingException()->isNone()) {
    return nullptr;
  }
  return ApiHandle::fromObject(thread->pendingException())->asPyObject();
}

extern "C" int _PyErr_ExceptionMessageMatches(const char* message) {
  Thread* thread = Thread::currentThread();
  if (thread->pendingException()->isNone()) {
    return 0;
  }

  HandleScope scope(thread);
  Handle<Object> exception_obj(
      &scope, ApiHandle::fromPyObject(PyErr_Occurred())->asObject());
  if (!exception_obj->isString()) {
    UNIMPLEMENTED("Handle non string exception objects");
  }

  Handle<String> exception(&scope, *exception_obj);
  return exception->equalsCString(message);
}

extern "C" PyObject* PyErr_Format(PyObject*, const char*, ...) {
  UNIMPLEMENTED("PyErr_Format");
}

extern "C" void PyErr_Clear(void) { UNIMPLEMENTED("PyErr_Clear"); }

extern "C" int PyErr_BadArgument(void) { UNIMPLEMENTED("PyErr_BadArgument"); }

extern "C" PyObject* PyErr_NoMemory(void) { UNIMPLEMENTED("PyErr_NoMemory"); }

extern "C" void Py_FatalError(const char*) { UNIMPLEMENTED("Py_FatalError"); }

extern "C" PyObject* _PyErr_FormatFromCause(PyObject*, const char*, ...) {
  UNIMPLEMENTED("_PyErr_FormatFromCause");
}

}  // namespace python
