// _warnings.c implementation

#include <cstdarg>

#include "cpython-data.h"
#include "cpython-types.h"
#include "runtime.h"
#include "utils.h"

namespace python {

static void callWarn(PyObject* category, PyObject* message,
                     Py_ssize_t stack_level, PyObject* source) {
  if (category == nullptr) {
    category = PyExc_RuntimeWarning;
  }
  if (source == nullptr) {
    source = Py_None;
  }
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object category_obj(&scope, ApiHandle::fromPyObject(category)->asObject());
  DCHECK(message != nullptr, "message cannot be null");
  Object message_obj(&scope, ApiHandle::fromPyObject(message)->asObject());
  Int stack_level_obj(&scope, thread->runtime()->newInt(stack_level));
  Object source_obj(&scope, ApiHandle::fromPyObject(source)->asObject());
  thread->invokeFunction4(SymbolId::kWarnings, SymbolId::kWarn, message_obj,
                          category_obj, stack_level_obj, source_obj);
  thread->clearPendingException();
}

PY_EXPORT int PyErr_WarnEx(PyObject* category, const char* text,
                           Py_ssize_t stack_level) {
  PyObject* message = PyUnicode_FromString(text);
  if (message == nullptr) {
    return -1;
  }
  callWarn(category, message, stack_level, nullptr);
  Py_DECREF(message);
  return 0;
}

static int warnFormat(PyObject* source, PyObject* category,
                      Py_ssize_t stack_level, const char* format,
                      va_list vargs) {
  PyObject* message = PyUnicode_FromFormatV(format, vargs);
  if (message == nullptr) {
    return -1;
  }
  callWarn(category, message, stack_level, source);
  Py_DECREF(message);
  return 0;
}

PY_EXPORT int PyErr_ResourceWarning(PyObject* source, Py_ssize_t stack_level,
                                    const char* format, ...) {
  va_list vargs;
  va_start(vargs, format);
  int res =
      warnFormat(source, PyExc_ResourceWarning, stack_level, format, vargs);
  va_end(vargs);
  return res;
}

PY_EXPORT int PyErr_WarnExplicit(PyObject* /* y */, const char* /* t */,
                                 const char* /* r */, int /* o */,
                                 const char* /* r */, PyObject* /* y */) {
  UNIMPLEMENTED("PyErr_WarnExplicit");
}

PY_EXPORT int PyErr_WarnFormat(PyObject* category, Py_ssize_t stack_level,
                               const char* format, ...) {
  va_list vargs;
  va_start(vargs, format);
  int res = warnFormat(nullptr, category, stack_level, format, vargs);
  va_end(vargs);
  return res;
}

}  // namespace python
