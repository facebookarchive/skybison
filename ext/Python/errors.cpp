#include "runtime.h"

namespace python {

extern "C" void PyErr_SetString(PyObject*, const char* message) {
  // TODO(T32875119): Set the correct exception type.
  Thread::currentThread()->raiseSystemErrorWithCStr(message);
}

extern "C" PyObject* PyErr_Occurred(void) {
  Thread* thread = Thread::currentThread();
  if (!thread->hasPendingException()) {
    return nullptr;
  }
  return ApiHandle::fromObject(thread->exceptionType());
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

extern "C" void PyErr_BadInternalCall(void) {
  UNIMPLEMENTED("PyErr_BadInternalCall");
}

extern "C" int PyErr_ExceptionMatches(PyObject* /* c */) {
  UNIMPLEMENTED("PyErr_ExceptionMatches");
}

extern "C" void PyErr_Fetch(PyObject** /* p_type */, PyObject** /* p_value */,
                            PyObject** /* p_traceback */) {
  UNIMPLEMENTED("PyErr_Fetch");
}

extern "C" PyObject* PyErr_FormatV(PyObject* /* n */, const char* /* t */,
                                   va_list /* s */) {
  UNIMPLEMENTED("PyErr_FormatV");
}

extern "C" void PyErr_GetExcInfo(PyObject** /* p_type */,
                                 PyObject** /* p_value */,
                                 PyObject** /* p_traceback */) {
  UNIMPLEMENTED("PyErr_GetExcInfo");
}

extern "C" int PyErr_GivenExceptionMatches(PyObject* /* r */,
                                           PyObject* /* c */) {
  UNIMPLEMENTED("PyErr_GivenExceptionMatches");
}

extern "C" PyObject* PyErr_NewException(const char* /* e */, PyObject* /* e */,
                                        PyObject* /* t */) {
  UNIMPLEMENTED("PyErr_NewException");
}

extern "C" PyObject* PyErr_NewExceptionWithDoc(const char* /* e */,
                                               const char* /* c */,
                                               PyObject* /* e */,
                                               PyObject* /* t */) {
  UNIMPLEMENTED("PyErr_NewExceptionWithDoc");
}

extern "C" void PyErr_NormalizeException(PyObject** /* exc */,
                                         PyObject** /* val */,
                                         PyObject** /* tb */) {
  UNIMPLEMENTED("PyErr_NormalizeException");
}

extern "C" PyObject* PyErr_ProgramText(const char* /* e */, int /* o */) {
  UNIMPLEMENTED("PyErr_ProgramText");
}

extern "C" PyObject* PyErr_SetExcFromWindowsErr(PyObject* /* c */,
                                                int /* r */) {
  UNIMPLEMENTED("PyErr_SetExcFromWindowsErr");
}

extern "C" PyObject* PyErr_SetExcFromWindowsErrWithFilename(
    PyObject* /* c */, int /* r */, const char* /* e */) {
  UNIMPLEMENTED("PyErr_SetExcFromWindowsErrWithFilename");
}

extern "C" PyObject* PyErr_SetExcFromWindowsErrWithFilenameObject(
    PyObject* /* c */, int /* r */, PyObject* /* t */) {
  UNIMPLEMENTED("PyErr_SetExcFromWindowsErrWithFilenameObject");
}

extern "C" PyObject* PyErr_SetExcFromWindowsErrWithFilenameObjects(
    PyObject* /* c */, int /* r */, PyObject* /* t */, PyObject* /* 2 */) {
  UNIMPLEMENTED("PyErr_SetExcFromWindowsErrWithFilenameObjects");
}

extern "C" void PyErr_SetExcInfo(PyObject* /* e */, PyObject* /* e */,
                                 PyObject* /* k */) {
  UNIMPLEMENTED("PyErr_SetExcInfo");
}

extern "C" PyObject* PyErr_SetFromErrno(PyObject* /* c */) {
  UNIMPLEMENTED("PyErr_SetFromErrno");
}

extern "C" PyObject* PyErr_SetFromErrnoWithFilename(PyObject* /* c */,
                                                    const char* /* e */) {
  UNIMPLEMENTED("PyErr_SetFromErrnoWithFilename");
}

extern "C" PyObject* PyErr_SetFromErrnoWithFilenameObject(PyObject* /* c */,
                                                          PyObject* /* t */) {
  UNIMPLEMENTED("PyErr_SetFromErrnoWithFilenameObject");
}

extern "C" PyObject* PyErr_SetFromErrnoWithFilenameObjects(PyObject* /* c */,
                                                           PyObject* /* t */,
                                                           PyObject* /* 2 */) {
  UNIMPLEMENTED("PyErr_SetFromErrnoWithFilenameObjects");
}

extern "C" PyObject* PyErr_SetFromWindowsErr(int /* r */) {
  UNIMPLEMENTED("PyErr_SetFromWindowsErr");
}

extern "C" PyObject* PyErr_SetFromWindowsErrWithFilename(int /* r */,
                                                         const char* /* e */) {
  UNIMPLEMENTED("PyErr_SetFromWindowsErrWithFilename");
}

extern "C" PyObject* PyErr_SetImportError(PyObject* /* g */, PyObject* /* e */,
                                          PyObject* /* h */) {
  UNIMPLEMENTED("PyErr_SetImportError");
}

extern "C" PyObject* PyErr_SetImportErrorSubclass(PyObject* /* n */,
                                                  PyObject* /* g */,
                                                  PyObject* /* e */,
                                                  PyObject* /* h */) {
  UNIMPLEMENTED("PyErr_SetImportErrorSubclass");
}

extern "C" void PyErr_SetNone(PyObject* /* n */) {
  UNIMPLEMENTED("PyErr_SetNone");
}

extern "C" void PyErr_SetObject(PyObject* /* n */, PyObject* /* e */) {
  UNIMPLEMENTED("PyErr_SetObject");
}

extern "C" void PyErr_SyntaxLocation(const char* /* e */, int /* o */) {
  UNIMPLEMENTED("PyErr_SyntaxLocation");
}

extern "C" void PyErr_SyntaxLocationEx(const char* /* e */, int /* o */,
                                       int /* t */) {
  UNIMPLEMENTED("PyErr_SyntaxLocationEx");
}

extern "C" void PyErr_WriteUnraisable(PyObject* /* j */) {
  UNIMPLEMENTED("PyErr_WriteUnraisable");
}

extern "C" void _PyErr_BadInternalCall(const char* /* e */, int /* o */) {
  UNIMPLEMENTED("_PyErr_BadInternalCall");
}

extern "C" PyObject* PyErr_ProgramTextObject(PyObject* /* e */, int /* o */) {
  UNIMPLEMENTED("PyErr_ProgramTextObject");
}

}  // namespace python
