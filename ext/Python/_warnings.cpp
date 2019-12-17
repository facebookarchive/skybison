// _warnings.c implementation

#include <cstdarg>

#include "cpython-data.h"
#include "cpython-types.h"

#include "capi-handles.h"
#include "runtime.h"
#include "utils.h"

namespace py {

static int callWarn(PyObject* category, PyObject* message,
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
  if (thread
          ->invokeFunction4(SymbolId::kWarnings, SymbolId::kWarn, message_obj,
                            category_obj, stack_level_obj, source_obj)
          .isErrorException()) {
    return -1;
  }
  return 0;
}

PY_EXPORT int PyErr_WarnEx(PyObject* category, const char* text,
                           Py_ssize_t stack_level) {
  PyObject* message = PyUnicode_FromString(text);
  if (message == nullptr) {
    return -1;
  }
  int res = callWarn(category, message, stack_level, nullptr);
  Py_DECREF(message);
  return res;
}

static int warnFormat(PyObject* source, PyObject* category,
                      Py_ssize_t stack_level, const char* format,
                      va_list vargs) {
  PyObject* message = PyUnicode_FromFormatV(format, vargs);
  if (message == nullptr) {
    return -1;
  }
  int res = callWarn(category, message, stack_level, source);
  Py_DECREF(message);
  return res;
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

PY_EXPORT int PyErr_WarnExplicitObject(PyObject* category, PyObject* message,
                                       PyObject* filename, int lineno,
                                       PyObject* module, PyObject* registry) {
  // module can be None if a warning is emitted late during Python shutdown.
  // In this case, the Python warnings module was probably unloaded, so filters
  // are no longer available to choose as actions. It is safer to ignore the
  // warning and do nothing.
  if (module == Py_None) {
    return 0;
  }
  if (category == nullptr) {
    category = PyExc_RuntimeWarning;
  }
  if (module == nullptr) {
    // Signal to Python implementation that the module should be derived from
    // the filename.
    module = Py_None;
  }
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  DCHECK(category != nullptr, "category cannot be null");
  Object category_obj(&scope, ApiHandle::fromPyObject(category)->asObject());
  DCHECK(message != nullptr, "message cannot be null");
  Object message_obj(&scope, ApiHandle::fromPyObject(message)->asObject());
  DCHECK(filename != nullptr, "filename cannot be null");
  Object filename_obj(&scope, ApiHandle::fromPyObject(filename)->asObject());
  Int lineno_obj(&scope, thread->runtime()->newInt(lineno));
  DCHECK(module != nullptr, "module cannot be null");
  Object module_obj(&scope, ApiHandle::fromPyObject(module)->asObject());
  Object registry_obj(&scope,
                      registry == nullptr
                          ? NoneType::object()
                          : ApiHandle::fromPyObject(registry)->asObject());
  if (thread
          ->invokeFunction6(SymbolId::kWarnings, SymbolId::kWarnExplicit,
                            message_obj, category_obj, filename_obj, lineno_obj,
                            module_obj, registry_obj)
          .isError()) {
    return -1;
  }
  return 0;
}

PY_EXPORT int PyErr_WarnFormat(PyObject* category, Py_ssize_t stack_level,
                               const char* format, ...) {
  va_list vargs;
  va_start(vargs, format);
  int res = warnFormat(nullptr, category, stack_level, format, vargs);
  va_end(vargs);
  return res;
}

}  // namespace py
