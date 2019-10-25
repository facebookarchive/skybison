#include "capi-handles.h"
#include "cpython-data.h"
#include "cpython-func.h"
#include "runtime.h"

namespace py {

static ListEntry* fromGCObject(void* gc_obj) {
  return static_cast<ListEntry*>(gc_obj) - 1;
}

PY_EXPORT Py_ssize_t PyGC_Collect() { UNIMPLEMENTED("PyGC_Collect"); }

// Releases memory allocated to an object using PyObject_GC_New() or
// PyObject_GC_NewVar().
PY_EXPORT void PyObject_GC_Del(void* op) {
  PyObject_GC_UnTrack(op);
  ListEntry* entry = fromGCObject(op);
  PyMem_Free(entry);
}

PY_EXPORT void PyObject_GC_Track(void* op) {
  ListEntry* entry = fromGCObject(op);
  if (!Thread::current()->runtime()->trackNativeGcObject(entry)) {
    Py_FatalError("GC object already tracked");
  }
}

PY_EXPORT void PyObject_GC_UnTrack(void* op) {
  ListEntry* entry = fromGCObject(op);
  Thread::current()->runtime()->untrackNativeGcObject(entry);
}

PY_EXPORT PyObject* _PyObject_GC_Malloc(size_t basicsize) {
  ListEntry* entry =
      static_cast<ListEntry*>(PyMem_Malloc(sizeof(ListEntry) + basicsize));
  if (entry == nullptr) {
    return PyErr_NoMemory();
  }
  entry->prev = nullptr;
  entry->next = nullptr;

  return reinterpret_cast<PyObject*>(entry + 1);
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
