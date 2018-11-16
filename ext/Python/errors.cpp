#include "runtime.h"

namespace python {

PY_EXPORT void PyErr_SetString(PyObject*, const char* message) {
  // TODO(T32875119): Set the correct exception type.
  Thread::currentThread()->raiseSystemErrorWithCStr(message);
}

PY_EXPORT PyObject* PyErr_Occurred() {
  Thread* thread = Thread::currentThread();
  if (!thread->hasPendingException()) {
    return nullptr;
  }
  return ApiHandle::fromObject(thread->exceptionType());
}

PY_EXPORT PyObject* PyErr_Format(PyObject*, const char*, ...) {
  UNIMPLEMENTED("PyErr_Format");
}

PY_EXPORT void PyErr_Clear() { UNIMPLEMENTED("PyErr_Clear"); }

PY_EXPORT int PyErr_BadArgument() { UNIMPLEMENTED("PyErr_BadArgument"); }

PY_EXPORT PyObject* PyErr_NoMemory() { UNIMPLEMENTED("PyErr_NoMemory"); }

PY_EXPORT PyObject* _PyErr_FormatFromCause(PyObject*, const char*, ...) {
  UNIMPLEMENTED("_PyErr_FormatFromCause");
}

PY_EXPORT void PyErr_BadInternalCall() {
  UNIMPLEMENTED("PyErr_BadInternalCall");
}

PY_EXPORT int PyErr_ExceptionMatches(PyObject* /* c */) {
  UNIMPLEMENTED("PyErr_ExceptionMatches");
}

PY_EXPORT void PyErr_Fetch(PyObject** /* p_type */, PyObject** /* p_value */,
                           PyObject** /* p_traceback */) {
  UNIMPLEMENTED("PyErr_Fetch");
}

PY_EXPORT PyObject* PyErr_FormatV(PyObject* /* n */, const char* /* t */,
                                  va_list /* s */) {
  UNIMPLEMENTED("PyErr_FormatV");
}

PY_EXPORT void PyErr_GetExcInfo(PyObject** /* p_type */,
                                PyObject** /* p_value */,
                                PyObject** /* p_traceback */) {
  UNIMPLEMENTED("PyErr_GetExcInfo");
}

PY_EXPORT int PyErr_GivenExceptionMatches(PyObject* /* r */,
                                          PyObject* /* c */) {
  UNIMPLEMENTED("PyErr_GivenExceptionMatches");
}

PY_EXPORT PyObject* PyErr_NewException(const char* /* e */, PyObject* /* e */,
                                       PyObject* /* t */) {
  UNIMPLEMENTED("PyErr_NewException");
}

PY_EXPORT PyObject* PyErr_NewExceptionWithDoc(const char* /* e */,
                                              const char* /* c */,
                                              PyObject* /* e */,
                                              PyObject* /* t */) {
  UNIMPLEMENTED("PyErr_NewExceptionWithDoc");
}

PY_EXPORT void PyErr_NormalizeException(PyObject** /* exc */,
                                        PyObject** /* val */,
                                        PyObject** /* tb */) {
  UNIMPLEMENTED("PyErr_NormalizeException");
}

PY_EXPORT PyObject* PyErr_ProgramText(const char* /* e */, int /* o */) {
  UNIMPLEMENTED("PyErr_ProgramText");
}

PY_EXPORT PyObject* PyErr_SetExcFromWindowsErr(PyObject* /* c */, int /* r */) {
  UNIMPLEMENTED("PyErr_SetExcFromWindowsErr");
}

PY_EXPORT PyObject* PyErr_SetExcFromWindowsErrWithFilename(
    PyObject* /* c */, int /* r */, const char* /* e */) {
  UNIMPLEMENTED("PyErr_SetExcFromWindowsErrWithFilename");
}

PY_EXPORT PyObject* PyErr_SetExcFromWindowsErrWithFilenameObject(
    PyObject* /* c */, int /* r */, PyObject* /* t */) {
  UNIMPLEMENTED("PyErr_SetExcFromWindowsErrWithFilenameObject");
}

PY_EXPORT PyObject* PyErr_SetExcFromWindowsErrWithFilenameObjects(
    PyObject* /* c */, int /* r */, PyObject* /* t */, PyObject* /* 2 */) {
  UNIMPLEMENTED("PyErr_SetExcFromWindowsErrWithFilenameObjects");
}

PY_EXPORT void PyErr_SetExcInfo(PyObject* /* e */, PyObject* /* e */,
                                PyObject* /* k */) {
  UNIMPLEMENTED("PyErr_SetExcInfo");
}

PY_EXPORT PyObject* PyErr_SetFromErrno(PyObject* /* c */) {
  UNIMPLEMENTED("PyErr_SetFromErrno");
}

PY_EXPORT PyObject* PyErr_SetFromErrnoWithFilename(PyObject* /* c */,
                                                   const char* /* e */) {
  UNIMPLEMENTED("PyErr_SetFromErrnoWithFilename");
}

PY_EXPORT PyObject* PyErr_SetFromErrnoWithFilenameObject(PyObject* /* c */,
                                                         PyObject* /* t */) {
  UNIMPLEMENTED("PyErr_SetFromErrnoWithFilenameObject");
}

PY_EXPORT PyObject* PyErr_SetFromErrnoWithFilenameObjects(PyObject* /* c */,
                                                          PyObject* /* t */,
                                                          PyObject* /* 2 */) {
  UNIMPLEMENTED("PyErr_SetFromErrnoWithFilenameObjects");
}

PY_EXPORT PyObject* PyErr_SetFromWindowsErr(int /* r */) {
  UNIMPLEMENTED("PyErr_SetFromWindowsErr");
}

PY_EXPORT PyObject* PyErr_SetFromWindowsErrWithFilename(int /* r */,
                                                        const char* /* e */) {
  UNIMPLEMENTED("PyErr_SetFromWindowsErrWithFilename");
}

PY_EXPORT PyObject* PyErr_SetImportError(PyObject* /* g */, PyObject* /* e */,
                                         PyObject* /* h */) {
  UNIMPLEMENTED("PyErr_SetImportError");
}

PY_EXPORT PyObject* PyErr_SetImportErrorSubclass(PyObject* /* n */,
                                                 PyObject* /* g */,
                                                 PyObject* /* e */,
                                                 PyObject* /* h */) {
  UNIMPLEMENTED("PyErr_SetImportErrorSubclass");
}

PY_EXPORT void PyErr_SetNone(PyObject* /* n */) {
  UNIMPLEMENTED("PyErr_SetNone");
}

PY_EXPORT void PyErr_SetObject(PyObject* /* n */, PyObject* /* e */) {
  UNIMPLEMENTED("PyErr_SetObject");
}

PY_EXPORT void PyErr_SyntaxLocation(const char* /* e */, int /* o */) {
  UNIMPLEMENTED("PyErr_SyntaxLocation");
}

PY_EXPORT void PyErr_SyntaxLocationEx(const char* /* e */, int /* o */,
                                      int /* t */) {
  UNIMPLEMENTED("PyErr_SyntaxLocationEx");
}

PY_EXPORT void PyErr_WriteUnraisable(PyObject* /* j */) {
  UNIMPLEMENTED("PyErr_WriteUnraisable");
}

PY_EXPORT void _PyErr_BadInternalCall(const char* /* e */, int /* o */) {
  UNIMPLEMENTED("_PyErr_BadInternalCall");
}

PY_EXPORT PyObject* PyErr_ProgramTextObject(PyObject* /* e */, int /* o */) {
  UNIMPLEMENTED("PyErr_ProgramTextObject");
}

PY_EXPORT void PyErr_Restore(PyObject* /* type */, PyObject* /* value */,
                             PyObject* /* traceback */) {
  UNIMPLEMENTED("PyErr_Restore");
}

}  // namespace python
