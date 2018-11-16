#include "runtime.h"

namespace python {

extern "C" Py_ssize_t PyGC_Collect(void) { UNIMPLEMENTED("PyGC_Collect"); }

extern "C" void PyObject_GC_Del(void* /* p */) {
  UNIMPLEMENTED("PyObject_GC_Del");
}

extern "C" void PyObject_GC_Track(void* /* p */) {
  UNIMPLEMENTED("PyObject_GC_Track");
}

extern "C" void PyObject_GC_UnTrack(void* /* p */) {
  UNIMPLEMENTED("PyObject_GC_UnTrack");
}

extern "C" PyObject* _PyObject_GC_Malloc(size_t /* e */) {
  UNIMPLEMENTED("_PyObject_GC_Malloc");
}

extern "C" PyObject* _PyObject_GC_New(PyTypeObject* /* p */) {
  UNIMPLEMENTED("_PyObject_GC_New");
}

extern "C" PyVarObject* _PyObject_GC_NewVar(PyTypeObject* /* p */,
                                            Py_ssize_t /* s */) {
  UNIMPLEMENTED("_PyObject_GC_NewVar");
}

extern "C" PyVarObject* _PyObject_GC_Resize(PyVarObject* /* p */,
                                            Py_ssize_t /* s */) {
  UNIMPLEMENTED("_PyObject_GC_Resize");
}

}  // namespace python
