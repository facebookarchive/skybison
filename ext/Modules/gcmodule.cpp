#include "runtime.h"

namespace python {

PY_EXPORT Py_ssize_t PyGC_Collect() { UNIMPLEMENTED("PyGC_Collect"); }

PY_EXPORT void PyObject_GC_Del(void* /* p */) {
  UNIMPLEMENTED("PyObject_GC_Del");
}

PY_EXPORT void PyObject_GC_Track(void* /* p */) {
  UNIMPLEMENTED("PyObject_GC_Track");
}

PY_EXPORT void PyObject_GC_UnTrack(void* /* p */) {
  UNIMPLEMENTED("PyObject_GC_UnTrack");
}

PY_EXPORT PyObject* _PyObject_GC_Malloc(size_t /* e */) {
  UNIMPLEMENTED("_PyObject_GC_Malloc");
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
