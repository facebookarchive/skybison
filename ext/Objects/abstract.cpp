// listobject.c implementation

#include "Python.h"

#include "runtime/handles.h"
#include "runtime/objects.h"
#include "runtime/runtime.h"

namespace python {

// Copied from cpython/Objects/abstract.c
extern "C" PyObject*
_Py_CheckFunctionResult(PyObject* func, PyObject* result, const char* where) {
  // TODO(T30925449): Implement PyErr_Occurred() to determine err_occurred
  int err_occurred = 0;

  assert((func != nullptr) ^ (where != nullptr));

  if (result == nullptr) {
    if (!err_occurred) {
      if (func) {
        PyErr_Format(
            PyExc_SystemError,
            "%R returned NULL without setting an error",
            func);
      } else {
        PyErr_Format(
            PyExc_SystemError,
            "%s returned NULL without setting an error",
            where);
      }
      return nullptr;
    }
  } else {
    if (err_occurred) {
      Py_DECREF(result);
      if (func) {
        _PyErr_FormatFromCause(
            PyExc_SystemError, "%R returned a result with an error set", func);
      } else {
        _PyErr_FormatFromCause(
            PyExc_SystemError, "%s returned a result with an error set", where);
      }
      return nullptr;
    }
  }
  return result;
}

extern "C" PyObject*
PyObject_Call(PyObject* callable, PyObject* args, PyObject* kwargs) {
  ternaryfunc call;
  PyObject* result;

  // PyObject_Call() must not be called with an exception set,
  // because it can clear it (directly or indirectly) and so the
  // caller loses its exception

  // TODO(T30925449): Implement PyErr_Occurred() function

  // TODO(T30526536): Set ob_type in ApiHandles and assert Tuple and Dict

  call = callable->ob_type->tp_call;
  if (call == nullptr) {
    PyErr_Format(
        PyExc_TypeError,
        "'%.200s' object is not callable",
        callable->ob_type->tp_name);
    return nullptr;
  }

  // TODO(T30925218): Start infinite recursive call counter

  result = (*call)(callable, args, kwargs);

  // TODO(T30925218): End infinite recursive call counter

  return _Py_CheckFunctionResult(callable, result, nullptr);
}

} // namespace python
