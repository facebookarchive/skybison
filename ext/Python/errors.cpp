#include <cerrno>
#include <cstdarg>

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
  return ApiHandle::newReference(thread, thread->pendingExceptionType());
}

PY_EXPORT PyObject* PyErr_Format(PyObject* exception, const char* format, ...) {
  va_list vargs;
  va_start(vargs, format);
  PyErr_FormatV(exception, format, vargs);
  va_end(vargs);
  return nullptr;
}

PY_EXPORT void PyErr_Clear() {
  Thread::currentThread()->clearPendingException();
}

PY_EXPORT int PyErr_BadArgument() {
  Thread* thread = Thread::currentThread();
  thread->raiseBadArgument();
  return 0;
}

PY_EXPORT PyObject* PyErr_NoMemory() {
  Thread* thread = Thread::currentThread();
  thread->raiseMemoryError();
  return nullptr;
}

PY_EXPORT PyObject* _PyErr_FormatFromCause(PyObject* exception,
                                           const char* format, ...) {
  CHECK(PyErr_Occurred(),
        "_PyErr_FormatFromCause must be called with an exception set");
  PyObject* exc = nullptr;
  PyObject* val = nullptr;
  PyObject* tb = nullptr;
  PyErr_Fetch(&exc, &val, &tb);
  PyErr_NormalizeException(&exc, &val, &tb);
  if (tb != nullptr) {
    PyException_SetTraceback(val, tb);
    Py_DECREF(tb);
  }
  Py_DECREF(exc);
  DCHECK(!PyErr_Occurred(), "error must not have occurred");

  va_list vargs;
  va_start(vargs, format);
  PyErr_FormatV(exception, format, vargs);
  va_end(vargs);

  PyObject* val2 = nullptr;
  PyErr_Fetch(&exc, &val2, &tb);
  PyErr_NormalizeException(&exc, &val2, &tb);
  Py_INCREF(val);
  PyException_SetCause(val2, val);
  PyException_SetContext(val2, val);
  PyErr_Restore(exc, val2, tb);

  return nullptr;
}

// Remove the preprocessor macro for PyErr_BadInternalCall() so that we can
// export the entry point for existing object code:
#pragma push_macro("PyErr_BadInternalCall")
#undef PyErr_BadInternalCall
PY_EXPORT void PyErr_BadInternalCall() {
  Thread* thread = Thread::currentThread();
  thread->raiseBadInternalCall();
}
#pragma pop_macro("PyErr_BadInternalCall")

PY_EXPORT int PyErr_ExceptionMatches(PyObject* exc) {
  return PyErr_GivenExceptionMatches(PyErr_Occurred(), exc);
}

PY_EXPORT void PyErr_Fetch(PyObject** pexc, PyObject** pval, PyObject** ptb) {
  Thread* thread = Thread::currentThread();
  DCHECK(pexc != nullptr, "pexc is null");
  if (thread->pendingExceptionType().isNoneType()) {
    *pexc = nullptr;
  } else {
    *pexc =
        ApiHandle::borrowedReference(thread, thread->pendingExceptionType());
  }
  DCHECK(pval != nullptr, "pval is null");
  if (thread->pendingExceptionValue().isNoneType()) {
    *pval = nullptr;
  } else {
    *pval =
        ApiHandle::borrowedReference(thread, thread->pendingExceptionValue());
  }
  DCHECK(ptb != nullptr, "ptb is null");
  if (thread->pendingExceptionTraceback().isNoneType()) {
    *ptb = nullptr;
  } else {
    *ptb = ApiHandle::borrowedReference(thread,
                                        thread->pendingExceptionTraceback());
  }
  thread->clearPendingException();
}

PY_EXPORT PyObject* PyErr_FormatV(PyObject* exception, const char* format,
                                  va_list vargs) {
  PyErr_Clear();  // Cannot call PyUnicode_FromFormatV with an exception set

  PyObject* string = PyUnicode_FromFormatV(format, vargs);
  PyErr_SetObject(exception, string);
  Py_XDECREF(string);
  return nullptr;
}

PY_EXPORT void PyErr_GetExcInfo(PyObject** /* p_type */,
                                PyObject** /* p_value */,
                                PyObject** /* p_traceback */) {
  UNIMPLEMENTED("PyErr_GetExcInfo");
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

static void setFromErrno(PyObject* type, PyObject* name1, PyObject* name2) {
  int saved_errno = errno;
  if (saved_errno == EINTR) {
    UNIMPLEMENTED("&& PyErr_CheckSignals() != 0) return;");
  }

  const char* str = saved_errno ? strerror(saved_errno) : "Error";
  DCHECK(str != nullptr, "str must not be null");
  PyObject* msg = PyUnicode_FromString(str);
  PyObject* err = PyLong_FromLong(saved_errno);
  PyObject* result;
  if (name1 == nullptr) {
    DCHECK(name2 == nullptr, "name2 must be nullptr");
    result = PyObject_CallFunctionObjArgs(type, err, msg, nullptr);
  } else if (name2 == nullptr) {
    result = PyObject_CallFunctionObjArgs(type, err, msg, name1, nullptr);
  } else {
    PyObject* zero = PyLong_FromLong(0);
    result = PyObject_CallFunctionObjArgs(type, err, msg, name1, zero, name2,
                                          nullptr);
    Py_DECREF(zero);
  }

  Py_DECREF(msg);
  Py_DECREF(err);
  DCHECK(result != nullptr, "result must not be null");
  PyErr_SetObject(type, result);
  Py_DECREF(result);
}

PY_EXPORT PyObject* PyErr_SetFromErrno(PyObject* type) {
  setFromErrno(type, nullptr, nullptr);
  return nullptr;
}

PY_EXPORT PyObject* PyErr_SetFromErrnoWithFilename(PyObject* type,
                                                   const char* filename) {
  PyObject* name = filename ? PyUnicode_FromString(filename) : nullptr;
  setFromErrno(type, name, nullptr);
  Py_XDECREF(name);
  return nullptr;
}

PY_EXPORT PyObject* PyErr_SetFromErrnoWithFilenameObject(PyObject* type,
                                                         PyObject* name) {
  setFromErrno(type, name, nullptr);
  return nullptr;
}

PY_EXPORT PyObject* PyErr_SetFromErrnoWithFilenameObjects(PyObject* type,
                                                          PyObject* name1,
                                                          PyObject* name2) {
  setFromErrno(type, name1, name2);
  return nullptr;
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
  Py_XINCREF(val);
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
    thread->setPendingExceptionType(NoneType::object());
  } else {
    thread->setPendingExceptionType(ApiHandle::fromPyObject(type)->asObject());
    // This is a stolen reference, decrement the reference count
    ApiHandle::fromPyObject(type)->decref();
  }
  if (value == nullptr) {
    thread->setPendingExceptionValue(NoneType::object());
  } else {
    thread->setPendingExceptionValue(
        ApiHandle::fromPyObject(value)->asObject());
    // This is a stolen reference, decrement the reference count
    ApiHandle::fromPyObject(value)->decref();
  }
  if (traceback != nullptr &&
      !ApiHandle::fromPyObject(traceback)->asObject().isTraceback()) {
    ApiHandle::fromPyObject(traceback)->decref();
    // Can only store traceback instances as the traceback
    traceback = nullptr;
  }
  if (traceback == nullptr) {
    thread->setPendingExceptionTraceback(NoneType::object());
  } else {
    thread->setPendingExceptionTraceback(
        ApiHandle::fromPyObject(traceback)->asObject());
    // This is a stolen reference, decrement the reference count
    ApiHandle::fromPyObject(traceback)->decref();
  }
}

// Like PyErr_Restore(), but if an exception is already set, set the context
// associated with it.
PY_EXPORT void _PyErr_ChainExceptions(PyObject* exc, PyObject* val,
                                      PyObject* tb) {
  if (exc == nullptr) return;

  if (PyErr_Occurred()) {
    PyObject *exc2, *val2, *tb2;
    PyErr_Fetch(&exc2, &val2, &tb2);
    PyErr_NormalizeException(&exc, &val, &tb);
    if (tb != nullptr) {
      PyException_SetTraceback(val, tb);
      Py_DECREF(tb);
    }
    Py_DECREF(exc);
    PyErr_NormalizeException(&exc2, &val2, &tb2);
    PyException_SetContext(val2, val);
    PyErr_Restore(exc2, val2, tb2);
  } else {
    PyErr_Restore(exc, val, tb);
  }
}

}  // namespace python
