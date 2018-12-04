#include "cpython-data.h"
#include "cpython-func.h"

#include "exception-builtins.h"
#include "runtime.h"

namespace python {

PY_EXPORT void PyErr_SetString(PyObject* exc, const char* msg) {
  PyObject* value = PyUnicode_FromString(msg);
  PyErr_SetObject(exc, value);
  Py_XDECREF(value);
}

PY_EXPORT PyObject* PyErr_Occurred() {
  Thread* thread = Thread::currentThread();
  if (!thread->hasPendingException()) {
    return nullptr;
  }
  return ApiHandle::newReference(thread, thread->exceptionType());
}

PY_EXPORT PyObject* PyErr_Format(PyObject*, const char*, ...) {
  UNIMPLEMENTED("PyErr_Format");
}

PY_EXPORT void PyErr_Clear() {
  Thread::currentThread()->clearPendingException();
}

PY_EXPORT int PyErr_BadArgument() {
  Thread* thread = Thread::currentThread();
  thread->raiseTypeErrorWithCStr("bad argument type for built-in operation");
  return 0;
}

PY_EXPORT PyObject* PyErr_NoMemory() {
  Thread* thread = Thread::currentThread();
  thread->raiseMemoryError();
  return nullptr;
}

PY_EXPORT PyObject* _PyErr_FormatFromCause(PyObject*, const char*, ...) {
  UNIMPLEMENTED("_PyErr_FormatFromCause");
}

PY_EXPORT void PyErr_BadInternalCall() {
  Thread* thread = Thread::currentThread();
  thread->raiseSystemErrorWithCStr("bad argument to internal function");
}

PY_EXPORT int PyErr_ExceptionMatches(PyObject* exc) {
  return PyErr_GivenExceptionMatches(PyErr_Occurred(), exc);
}

PY_EXPORT void PyErr_Fetch(PyObject** pexc, PyObject** pval, PyObject** ptb) {
  Thread* thread = Thread::currentThread();
  DCHECK(pexe != nullptr, "pexc is null");
  if (thread->exceptionType().isNoneType()) {
    *pexc = nullptr;
  } else {
    *pexc = ApiHandle::borrowedReference(thread, thread->exceptionType());
  }
  DCHECK(pval != nullptr, "pval is null");
  if (thread->exceptionValue().isNoneType()) {
    *pval = nullptr;
  } else {
    *pval = ApiHandle::borrowedReference(thread, thread->exceptionValue());
  }
  DCHECK(ptb != nullptr, "ptb is null");
  if (thread->exceptionTraceback().isNoneType()) {
    *ptb = nullptr;
  } else {
    *ptb = ApiHandle::borrowedReference(thread, thread->exceptionTraceback());
  }
  thread->clearPendingException();
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

static bool givenExceptionMatches(Thread* thread, const Object& given,
                                  const Object& exc) {
  HandleScope scope(thread);
  if (exc->isTuple()) {
    Tuple tuple(&scope, *exc);
    Object item(&scope, NoneType::object());
    for (word i = 0; i < tuple->length(); i++) {
      item = tuple->at(i);
      if (givenExceptionMatches(thread, given, item)) {
        return true;
      }
    }
    return false;
  }
  Runtime* runtime = thread->runtime();
  Object given_type(&scope, *given);
  if (runtime->isInstanceOfBaseException(*given_type)) {
    given_type = runtime->typeOf(*given);
  }
  if (runtime->isInstanceOfType(*given_type) &&
      RawType::cast(*given_type).isBaseExceptionSubclass() &&
      runtime->isInstanceOfType(*exc) &&
      RawType::cast(*exc).isBaseExceptionSubclass()) {
    Type subtype(&scope, *given_type);
    Type supertype(&scope, *exc);
    return runtime->isSubclass(subtype, supertype);
  }
  return *given_type == *exc;
}

PY_EXPORT int PyErr_GivenExceptionMatches(PyObject* given, PyObject* exc) {
  if (given == nullptr || exc == nullptr) {
    return 0;
  }
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Object given_obj(&scope, ApiHandle::fromPyObject(given)->asObject());
  Object exc_obj(&scope, ApiHandle::fromPyObject(exc)->asObject());
  return givenExceptionMatches(thread, given_obj, exc_obj) ? 1 : 0;
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

PY_EXPORT void PyErr_NormalizeException(PyObject** exc, PyObject** val,
                                        PyObject** tb) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);

  Object exc_obj(&scope, *exc ? ApiHandle::fromPyObject(*exc)->asObject()
                              : NoneType::object());
  Object exc_orig(&scope, *exc_obj);
  Object val_obj(&scope, *val ? ApiHandle::fromPyObject(*val)->asObject()
                              : NoneType::object());
  Object val_orig(&scope, *val_obj);
  Object tb_obj(&scope, *tb ? ApiHandle::fromPyObject(*tb)->asObject()
                            : NoneType::object());
  Object tb_orig(&scope, *tb_obj);
  normalizeException(thread, &exc_obj, &val_obj, &tb_obj);
  if (*exc_obj != *exc_orig) {
    PyObject* tmp = *exc;
    *exc = ApiHandle::newReference(thread, *exc_obj);
    Py_XDECREF(tmp);
  }
  if (*val_obj != *val_orig) {
    PyObject* tmp = *val;
    *val = ApiHandle::newReference(thread, *val_obj);
    Py_XDECREF(tmp);
  }
  if (*tb_obj != *tb_orig) {
    PyObject* tmp = *tb;
    *tb = ApiHandle::newReference(thread, *tb_obj);
    Py_XDECREF(tmp);
  }
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

PY_EXPORT void PyErr_SetNone(PyObject* type) { PyErr_SetObject(type, Py_None); }

PY_EXPORT void PyErr_SetObject(PyObject* exc, PyObject* val) {
  // TODO(cshapiro): chain exception when val is an exception instance
  Py_XINCREF(exc);
  PyErr_Restore(exc, val, nullptr);
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

PY_EXPORT void _PyErr_BadInternalCall(const char* filename, int lineno) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Object message(&scope, thread->runtime()->newStrFromFormat(
                             "%s:%d: bad argument to internal function",
                             filename, lineno));
  thread->raiseSystemError(*message);
}

PY_EXPORT PyObject* PyErr_ProgramTextObject(PyObject* /* e */, int /* o */) {
  UNIMPLEMENTED("PyErr_ProgramTextObject");
}

PY_EXPORT void PyErr_Restore(PyObject* type, PyObject* value,
                             PyObject* traceback) {
  Thread* thread = Thread::currentThread();
  if (type == nullptr) {
    thread->setExceptionType(NoneType::object());
  } else {
    thread->setExceptionType(ApiHandle::fromPyObject(type)->asObject());
    // This is a stolen reference, decrement the reference count
    ApiHandle::fromPyObject(type)->decref();
  }
  if (value == nullptr) {
    thread->setExceptionValue(NoneType::object());
  } else {
    thread->setExceptionValue(ApiHandle::fromPyObject(value)->asObject());
    // This is a stolen reference, decrement the reference count
    ApiHandle::fromPyObject(value)->decref();
  }
  if (traceback != nullptr &&
      !ApiHandle::fromPyObject(traceback)->asObject()->isTraceback()) {
    ApiHandle::fromPyObject(traceback)->decref();
    // Can only store traceback instances as the traceback
    traceback = nullptr;
  }
  if (traceback == nullptr) {
    thread->setExceptionTraceback(NoneType::object());
  } else {
    thread->setExceptionTraceback(
        ApiHandle::fromPyObject(traceback)->asObject());
    // This is a stolen reference, decrement the reference count
    ApiHandle::fromPyObject(traceback)->decref();
  }
}

}  // namespace python
