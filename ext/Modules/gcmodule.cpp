#include "cpython-data.h"
#include "cpython-func.h"
#include "runtime.h"

namespace python {

static ListEntry* fromGCObject(void* gc_obj) {
  return static_cast<ListEntry*>(gc_obj) - 1;
}

PY_EXPORT Py_ssize_t PyGC_Collect() { UNIMPLEMENTED("PyGC_Collect"); }

// Releases memory allocated to an object using PyObject_GC_New() or
// PyObject_GC_NewVar().
PY_EXPORT void PyObject_GC_Del(void* op) {
  PyObject_GC_UnTrack(op);
  ListEntry* entry = fromGCObject(op);
  PyObject_Free(entry);
}

PY_EXPORT void PyObject_GC_Track(void* p) {
  ListEntry* entry = fromGCObject(p);
  if (!Thread::current()->runtime()->trackObject(entry)) {
    Py_FatalError("GC object already tracked");
  }
}

PY_EXPORT void PyObject_GC_UnTrack(void* op) {
  ListEntry* entry = fromGCObject(op);
  Thread::current()->runtime()->untrackObject(entry);
}

PY_EXPORT PyObject* _PyObject_GC_Malloc(size_t basicsize) {
  ListEntry* entry =
      static_cast<ListEntry*>(PyObject_Malloc(sizeof(ListEntry) + basicsize));
  if (entry == nullptr) {
    return PyErr_NoMemory();
  }
  entry->prev = nullptr;
  entry->next = nullptr;

  return reinterpret_cast<PyObject*>(entry + 1);
}

PY_EXPORT PyObject* _PyObject_GC_New(PyTypeObject* /* p */) {
  UNIMPLEMENTED("_PyObject_GC_New");
}

PY_EXPORT PyVarObject* _PyObject_GC_NewVar(PyTypeObject* /* p */,
                                           Py_ssize_t /* s */) {
  UNIMPLEMENTED("_PyObject_GC_NewVar");
}

PY_EXPORT PyVarObject* _PyObject_GC_Resize(PyVarObject* /* p */,
                                           Py_ssize_t /* s */) {
  UNIMPLEMENTED("_PyObject_GC_Resize");
}

}  // namespace python
