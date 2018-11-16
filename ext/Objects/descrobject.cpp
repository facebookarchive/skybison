#include "runtime.h"

namespace python {

PY_EXPORT PyObject* PyDescr_NewClassMethod(PyTypeObject* /* e */,
                                           PyMethodDef* /* d */) {
  UNIMPLEMENTED("PyDescr_NewClassMethod");
}

PY_EXPORT PyObject* PyDictProxy_New(PyObject* /* g */) {
  UNIMPLEMENTED("PyDictProxy_New");
}

PY_EXPORT PyObject* PyDescr_NewGetSet(PyTypeObject* /* e */,
                                      PyGetSetDef* /* t */) {
  UNIMPLEMENTED("PyDescr_NewGetSet");
}

PY_EXPORT PyObject* PyDescr_NewMember(PyTypeObject* /* e */,
                                      PyMemberDef* /* r */) {
  UNIMPLEMENTED("PyDescr_NewMember");
}

PY_EXPORT PyObject* PyDescr_NewMethod(PyTypeObject* /* e */,
                                      PyMethodDef* /* d */) {
  UNIMPLEMENTED("PyDescr_NewMethod");
}

PY_EXPORT PyObject* PyWrapper_New(PyObject* /* d */, PyObject* /* f */) {
  UNIMPLEMENTED("PyWrapper_New");
}

}  // namespace python
