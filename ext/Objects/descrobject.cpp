#include "runtime.h"

namespace python {

extern "C" PyObject* PyDescr_NewClassMethod(PyTypeObject* /* e */,
                                            PyMethodDef* /* d */) {
  UNIMPLEMENTED("PyDescr_NewClassMethod");
}

extern "C" PyObject* PyDictProxy_New(PyObject* /* g */) {
  UNIMPLEMENTED("PyDictProxy_New");
}

extern "C" PyObject* PyDescr_NewGetSet(PyTypeObject* /* e */,
                                       PyGetSetDef* /* t */) {
  UNIMPLEMENTED("PyDescr_NewGetSet");
}

extern "C" PyObject* PyDescr_NewMember(PyTypeObject* /* e */,
                                       PyMemberDef* /* r */) {
  UNIMPLEMENTED("PyDescr_NewMember");
}

extern "C" PyObject* PyDescr_NewMethod(PyTypeObject* /* e */,
                                       PyMethodDef* /* d */) {
  UNIMPLEMENTED("PyDescr_NewMethod");
}

extern "C" PyObject* PyWrapper_New(PyObject* /* d */, PyObject* /* f */) {
  UNIMPLEMENTED("PyWrapper_New");
}

}  // namespace python
