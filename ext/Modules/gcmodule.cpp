#include "cpython-data.h"
#include "cpython-func.h"

#include "api-handle.h"
#include "runtime.h"

namespace py {

PY_EXPORT Py_ssize_t PyGC_Collect() { UNIMPLEMENTED("PyGC_Collect"); }

// Releases memory allocated to an object using PyObject_GC_New() or
// PyObject_GC_NewVar().
PY_EXPORT void PyObject_GC_Del(void* op) { PyObject_Free(op); }

PY_EXPORT void PyObject_GC_Track(void*) {}

PY_EXPORT void PyObject_GC_UnTrack(void*) {}

PY_EXPORT PyObject* _PyObject_GC_Malloc(size_t basicsize) {
  return reinterpret_cast<PyObject*>(PyObject_Malloc(basicsize));
}

PY_EXPORT PyObject* _PyObject_GC_Calloc(size_t basicsize) {
  return reinterpret_cast<PyObject*>(PyObject_Calloc(1, basicsize));
}

PY_EXPORT PyObject* _PyObject_GC_New(PyTypeObject* type) {
  PyObject* obj =
      static_cast<PyObject*>(_PyObject_GC_Malloc(_PyObject_SIZE(type)));
  if (obj == nullptr) return PyErr_NoMemory();
  return PyObject_INIT(obj, type);
}

PY_EXPORT PyVarObject* _PyObject_GC_NewVar(PyTypeObject* type,
                                           Py_ssize_t nitems) {
  PyObject* obj = static_cast<PyObject*>(
      _PyObject_GC_Malloc(_PyObject_VAR_SIZE(type, nitems)));
  if (obj == nullptr) return reinterpret_cast<PyVarObject*>(PyErr_NoMemory());
  return PyObject_INIT_VAR(obj, type, nitems);
}

PY_EXPORT PyVarObject* _PyObject_GC_Resize(PyVarObject* /* p */,
                                           Py_ssize_t /* s */) {
  UNIMPLEMENTED("_PyObject_GC_Resize");
}

}  // namespace py
