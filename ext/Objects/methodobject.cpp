#include "runtime.h"

namespace python {

extern "C" PyObject* PyCFunction_NewEx(PyMethodDef* /* l */, PyObject* /* f */,
                                       PyObject* /* e */) {
  UNIMPLEMENTED("PyCFunction_NewEx");
}

extern "C" int PyCFunction_ClearFreeList(void) {
  UNIMPLEMENTED("PyCFunction_ClearFreeList");
}

extern "C" int PyCFunction_GetFlags(PyObject* /* p */) {
  UNIMPLEMENTED("PyCFunction_GetFlags");
}

extern "C" PyCFunction PyCFunction_GetFunction(PyObject* /* p */) {
  UNIMPLEMENTED("PyCFunction_GetFunction");
}

extern "C" PyObject* PyCFunction_GetSelf(PyObject* /* p */) {
  UNIMPLEMENTED("PyCFunction_GetSelf");
}

extern "C" PyObject* PyCFunction_Call(PyObject* /* c */, PyObject* /* s */,
                                      PyObject* /* s */) {
  UNIMPLEMENTED("PyCFunction_Call");
}

}  // namespace python
