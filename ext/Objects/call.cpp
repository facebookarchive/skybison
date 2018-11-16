#include "runtime.h"

namespace python {

extern "C" PyObject* PyCFunction_Call(PyObject* /* c */, PyObject* /* s */,
                                      PyObject* /* s */) {
  UNIMPLEMENTED("PyCFunction_Call");
}

extern "C" PyObject* PyEval_CallFunction(PyObject* /* e */, const char* /* t */,
                                         ...) {
  UNIMPLEMENTED("PyEval_CallFunction");
}

extern "C" PyObject* PyEval_CallMethod(PyObject* /* j */, const char* /* e */,
                                       const char* /* t */, ...) {
  UNIMPLEMENTED("PyEval_CallMethod");
}

extern "C" PyObject* PyEval_CallObjectWithKeywords(PyObject* /* e */,
                                                   PyObject* /* s */,
                                                   PyObject* /* s */) {
  UNIMPLEMENTED("PyEval_CallObjectWithKeywords");
}

extern "C" PyObject* PyObject_Call(PyObject* /* e */, PyObject* /* s */,
                                   PyObject* /* s */) {
  UNIMPLEMENTED("PyObject_Call");
}

extern "C" PyObject* PyObject_CallFunction(PyObject* /* e */,
                                           const char* /* t */, ...) {
  UNIMPLEMENTED("PyObject_CallFunction");
}

extern "C" PyObject* PyObject_CallFunctionObjArgs(PyObject* /* e */, ...) {
  UNIMPLEMENTED("PyObject_CallFunctionObjArgs");
}

extern "C" PyObject* PyObject_CallMethod(PyObject* /* j */, const char* /* e */,
                                         const char* /* t */, ...) {
  UNIMPLEMENTED("PyObject_CallMethod");
}

extern "C" PyObject* PyObject_CallMethodObjArgs(PyObject* /* e */,
                                                PyObject* /* e */, ...) {
  UNIMPLEMENTED("PyObject_CallMethodObjArgs");
}

extern "C" PyObject* PyObject_CallObject(PyObject* /* e */, PyObject* /* s */) {
  UNIMPLEMENTED("PyObject_CallObject");
}

extern "C" PyObject* _PyObject_CallFunction_SizeT(PyObject* /* e */,
                                                  const char* /* t */, ...) {
  UNIMPLEMENTED("_PyObject_CallFunction_SizeT");
}

extern "C" PyObject* _PyObject_CallMethod_SizeT(PyObject* /* j */,
                                                const char* /* e */,
                                                const char* /* t */, ...) {
  UNIMPLEMENTED("_PyObject_CallMethod_SizeT");
}

}  // namespace python
