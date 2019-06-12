#include <unistd.h>
#include <cerrno>
#include <cstdarg>

#include "cpython-data.h"
#include "cpython-func.h"

#include "exception-builtins.h"
#include "fileutils.h"
#include "runtime.h"
#include "sys-module.h"

namespace python {

PY_EXPORT void PyErr_SetString(PyObject* exc, const char* msg) {
  PyObject* value = PyUnicode_FromString(msg);
  PyErr_SetObject(exc, value);
  Py_XDECREF(value);
}

PY_EXPORT PyObject* PyErr_Occurred() {
  Thread* thread = Thread::current();
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

PY_EXPORT void PyErr_Clear() { Thread::current()->clearPendingException(); }

PY_EXPORT int PyErr_BadArgument() {
  Thread* thread = Thread::current();
  thread->raiseBadArgument();
  return 0;
}

PY_EXPORT PyObject* PyErr_NoMemory() {
  Thread* thread = Thread::current();
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
  Thread* thread = Thread::current();
  thread->raiseBadInternalCall();
}
#pragma pop_macro("PyErr_BadInternalCall")

PY_EXPORT int PyErr_ExceptionMatches(PyObject* exc) {
  return PyErr_GivenExceptionMatches(PyErr_Occurred(), exc);
}

PY_EXPORT void PyErr_Fetch(PyObject** pexc, PyObject** pval, PyObject** ptb) {
  Thread* thread = Thread::current();
  DCHECK(pexc != nullptr, "pexc is null");
  if (thread->pendingExceptionType().isNoneType()) {
    *pexc = nullptr;
  } else {
    *pexc = ApiHandle::newReference(thread, thread->pendingExceptionType());
  }
  DCHECK(pval != nullptr, "pval is null");
  if (thread->pendingExceptionValue().isNoneType()) {
    *pval = nullptr;
  } else {
    *pval = ApiHandle::newReference(thread, thread->pendingExceptionValue());
  }
  DCHECK(ptb != nullptr, "ptb is null");
  if (thread->pendingExceptionTraceback().isNoneType()) {
    *ptb = nullptr;
  } else {
    *ptb = ApiHandle::newReference(thread, thread->pendingExceptionTraceback());
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

PY_EXPORT void PyErr_GetExcInfo(PyObject** p_type, PyObject** p_value,
                                PyObject** p_traceback) {
  Thread* thread = Thread::current();
  if (!thread->hasCaughtException()) {
    *p_type = nullptr;
    *p_value = nullptr;
    *p_traceback = nullptr;
    return;
  }
  *p_type = ApiHandle::newReference(thread, thread->caughtExceptionType());
  *p_value = ApiHandle::newReference(thread, thread->caughtExceptionValue());
  // TODO(T42241510): Pass caughtExeptionTraceback() when it becomes available.
  *p_traceback = nullptr;
}

PY_EXPORT int PyErr_GivenExceptionMatches(PyObject* given, PyObject* exc) {
  if (given == nullptr || exc == nullptr) {
    return 0;
  }
  Thread* thread = Thread::current();
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
  Thread* thread = Thread::current();
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

PY_EXPORT void PyErr_SetExcInfo(PyObject* type, PyObject* value,
                                PyObject* traceback) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object type_obj(&scope, type == nullptr
                              ? NoneType::object()
                              : ApiHandle::fromPyObject(type)->asObject());
  thread->setCaughtExceptionType(*type_obj);
  Object value_obj(&scope, value == nullptr
                               ? NoneType::object()
                               : ApiHandle::fromPyObject(value)->asObject());
  thread->setCaughtExceptionValue(*value_obj);
  Object traceback_obj(&scope,
                       traceback == nullptr
                           ? NoneType::object()
                           : ApiHandle::fromPyObject(traceback)->asObject());
  thread->setCaughtExceptionTraceback(*traceback_obj);
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
  Thread* thread = Thread::current();
  if (exc == nullptr) {
    DCHECK(val == nullptr, "nullptr exc with non-nullptr val");
    thread->clearPendingException();
    return;
  }

  HandleScope scope(thread);
  Object exc_obj(&scope, ApiHandle::fromPyObject(exc)->asObject());
  Object val_obj(&scope, ApiHandle::fromPyObject(val)->asObject());
  thread->raiseWithType(*exc_obj, *val_obj);
}

PY_EXPORT void PyErr_SyntaxLocation(const char* /* e */, int /* o */) {
  UNIMPLEMENTED("PyErr_SyntaxLocation");
}

PY_EXPORT void PyErr_SyntaxLocationEx(const char* filename, int lineno,
                                      int col_offset) {
  PyObject* fileobj;
  if (filename != nullptr) {
    fileobj = PyUnicode_DecodeFSDefault(filename);
    if (fileobj == nullptr) {
      PyErr_Clear();
    }
  } else {
    fileobj = nullptr;
  }
  PyErr_SyntaxLocationObject(fileobj, lineno, col_offset);
  Py_XDECREF(fileobj);
}

PY_EXPORT void PyErr_SyntaxLocationObject(PyObject* filename, int lineno,
                                          int col_offset) {
  PyObject *exc, *val, *tb;
  // Add attributes for the line number and filename for the error
  PyErr_Fetch(&exc, &val, &tb);
  PyErr_NormalizeException(&exc, &val, &tb);
  // XXX check that it is, indeed, a syntax error. It might not be, though.
  PyObject* lineno_obj = PyLong_FromLong(lineno);
  if (lineno_obj == nullptr) {
    PyErr_Clear();
  } else {
    if (PyObject_SetAttrString(val, "lineno", lineno_obj)) {
      PyErr_Clear();
    }
    Py_DECREF(lineno_obj);
  }
  PyObject* col_obj = nullptr;
  if (col_offset >= 0) {
    col_obj = PyLong_FromLong(col_offset);
    if (col_obj == nullptr) {
      PyErr_Clear();
    }
  }
  if (PyObject_SetAttrString(val, "offset", col_obj ? col_obj : Py_None)) {
    PyErr_Clear();
  }
  Py_XDECREF(col_obj);
  if (filename != nullptr) {
    if (PyObject_SetAttrString(val, "filename", filename)) {
      PyErr_Clear();
    }

    PyObject* text_obj = PyErr_ProgramTextObject(filename, lineno);
    if (text_obj) {
      if (PyObject_SetAttrString(val, "text", text_obj)) {
        PyErr_Clear();
      }
      Py_DECREF(text_obj);
    }
  }
  if (exc != PyExc_SyntaxError) {
    if (!PyObject_HasAttrString(val, "msg")) {
      PyObject* msg_obj = PyObject_Str(val);
      if (msg_obj) {
        if (PyObject_SetAttrString(val, "msg", msg_obj)) {
          PyErr_Clear();
        }
        Py_DECREF(msg_obj);
      } else {
        PyErr_Clear();
      }
    }
    if (!PyObject_HasAttrString(val, "print_file_and_line")) {
      if (PyObject_SetAttrString(val, "print_file_and_line", Py_None)) {
        PyErr_Clear();
      }
    }
  }
  PyErr_Restore(exc, val, tb);
}

static RawObject fileWriteObjectStrUnraisable(Thread* thread,
                                              const Object& file,
                                              const Object& obj) {
  HandleScope scope(thread);
  Object obj_str(&scope, thread->invokeFunction1(SymbolId::kBuiltins,
                                                 SymbolId::kStr, obj));
  if (obj_str.isError()) {
    thread->clearPendingException();
    return *obj_str;
  }
  RawObject result = thread->invokeMethod2(file, SymbolId::kWrite, obj_str);
  thread->clearPendingException();
  return result;
}

static RawObject fileWriteObjectReprUnraisable(Thread* thread,
                                               const Object& file,
                                               const Object& obj) {
  HandleScope scope(thread);
  Object obj_repr(&scope, thread->invokeFunction1(SymbolId::kBuiltins,
                                                  SymbolId::kRepr, obj));
  if (obj_repr.isError()) {
    thread->clearPendingException();
    return *obj_repr;
  }
  RawObject result = thread->invokeMethod2(file, SymbolId::kWrite, obj_repr);
  thread->clearPendingException();
  return result;
}

static RawObject fileWriteCStrUnraisable(Thread* thread, const Object& file,
                                         const char* c_str) {
  HandleScope scope(thread);
  Object str(&scope, thread->runtime()->newStrFromFmt(c_str));
  RawObject result = thread->invokeMethod2(file, SymbolId::kWrite, str);
  thread->clearPendingException();
  return result;
}

PY_EXPORT void PyErr_WriteUnraisable(PyObject* obj) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object exc_obj(&scope, thread->pendingExceptionType());
  Object val(&scope, thread->pendingExceptionValue());
  thread->clearPendingException();

  Runtime* runtime = thread->runtime();
  Object sys_stderr(&scope, runtime->lookupNameInModule(thread, SymbolId::kSys,
                                                        SymbolId::kStderr));
  if (obj != nullptr) {
    if (fileWriteCStrUnraisable(thread, sys_stderr, "Exception ignored in: ")
            .isError()) {
      return;
    }
    Object object(&scope, ApiHandle::fromPyObject(obj)->asObject());
    if (fileWriteObjectReprUnraisable(thread, sys_stderr, object).isError()) {
      if (fileWriteCStrUnraisable(thread, sys_stderr, "<object repr() failed>")
              .isError()) {
        return;
      }
    }
    if (fileWriteCStrUnraisable(thread, sys_stderr, "\n").isError()) {
      return;
    }
  }

  if (runtime->printTraceback(thread, sys_stderr).isError()) {
    thread->clearPendingException();
    return;
  }

  if (exc_obj.isNoneType()) {
    thread->clearPendingException();
    return;
  }

  DCHECK(runtime->isInstanceOfType(*exc_obj), "exc must be a type");
  Type exc_type(&scope, *exc_obj);
  Type base_exception(&scope, runtime->typeAt(LayoutId::kBaseException));
  DCHECK(runtime->isSubclass(exc_type, base_exception),
         "exc must be a subclass of BaseException");
  // TODO(T42602623): If exc_type.name() is None, Remove dotted components of
  // name, eg A.B.C => C

  Object module_name_obj(
      &scope,
      runtime->attributeAtId(thread, exc_type, SymbolId::kDunderModule));
  if (!runtime->isInstanceOfStr(*module_name_obj)) {
    thread->clearPendingException();
    if (fileWriteCStrUnraisable(thread, sys_stderr, "<unknown>").isError()) {
      return;
    }
  } else {
    Str module_name(&scope, *module_name_obj);
    if (!module_name.equalsCStr("builtins")) {
      if (fileWriteObjectStrUnraisable(thread, sys_stderr, module_name)
              .isError()) {
        return;
      }
      if (fileWriteCStrUnraisable(thread, sys_stderr, ".").isError()) {
        return;
      }
    }
  }

  if (exc_type.name().isNoneType()) {
    if (fileWriteCStrUnraisable(thread, sys_stderr, "<unknown>").isError()) {
      return;
    }
  } else {
    Str exc_type_name(&scope, exc_type.name());
    if (fileWriteObjectStrUnraisable(thread, sys_stderr, exc_type_name)
            .isError()) {
      return;
    }
  }

  if (!val.isNoneType()) {
    if (fileWriteCStrUnraisable(thread, sys_stderr, ": ").isError()) {
      return;
    }
    if (fileWriteObjectStrUnraisable(thread, sys_stderr, val).isError()) {
      if (fileWriteCStrUnraisable(thread, sys_stderr,
                                  "<exception str() failed>")
              .isError()) {
        return;
      }
    }
  }
  if (fileWriteCStrUnraisable(thread, sys_stderr, "\n").isError()) {
    return;
  }
}

PY_EXPORT void _PyErr_BadInternalCall(const char* filename, int lineno) {
  Thread::current()->raiseWithFmt(LayoutId::kSystemError,
                                  "%s:%d: bad argument to internal function",
                                  filename, lineno);
}

// Attempt to load the line of text that the exception refers to. If it fails,
// it will return nullptr but will not set an exception.
//
// XXX The functionality of this function is quite similar to the functionality
// in tb_displayline() in traceback.c.
static PyObject* err_programtext(FILE* fp, int lineno) {
  if (fp == nullptr) {
    return nullptr;
  }
  int i;
  char linebuf[1000];
  for (i = 0; i < lineno; i++) {
    char* plastchar = &linebuf[sizeof(linebuf) - 2];
    do {
      *plastchar = '\0';
      if (Py_UniversalNewlineFgets(linebuf, sizeof linebuf, fp, nullptr) ==
          nullptr) {
        break;
      }
      // fgets read *something*:
      // * if it didn't get as far as plastchar, it must have found a newline or
      //   hit the end of the file;
      // * if plastchar is \n, it obviously found a newline;
      // * else we haven't yet seen a newline, so must continue
    } while (*plastchar != '\0' && *plastchar != '\n');
  }
  fclose(fp);
  if (i == lineno) {
    PyObject* res = PyUnicode_FromString(linebuf);
    if (res == nullptr) {
      PyErr_Clear();
    }
    return res;
  }
  return nullptr;
}

// TODO(T44862285): This should be re-written in managed code
PY_EXPORT PyObject* PyErr_ProgramTextObject(PyObject* filename, int lineno) {
  if (filename == nullptr || lineno <= 0) {
    return nullptr;
  }
  FILE* fp = _Py_fopen_obj(filename, "rb");
  if (fp == nullptr) {
    PyErr_Clear();
    return nullptr;
  }
  return err_programtext(fp, lineno);
}

PY_EXPORT void PyErr_Restore(PyObject* type, PyObject* value,
                             PyObject* traceback) {
  Thread* thread = Thread::current();
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
