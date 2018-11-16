// object.c implementation

#include "Python.h"

namespace python {

extern "C" void _Py_Dealloc(PyObject* op) {
  // TODO(T30365701): Add a deallocation strategy for ApiHandles
  if (Py_TYPE(op) && Py_TYPE(op)->tp_dealloc) {
    destructor dealloc = Py_TYPE(op)->tp_dealloc;
    _Py_ForgetReference(op);
    (*dealloc)(op);
  }
}

} // namespace python
