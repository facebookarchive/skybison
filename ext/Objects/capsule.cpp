#include "runtime.h"

namespace python {

PY_EXPORT void* PyCapsule_GetContext(PyObject* /* o */) {
  UNIMPLEMENTED("PyCapsule_GetContext");
}

PY_EXPORT PyCapsule_Destructor PyCapsule_GetDestructor(PyObject* /* o */) {
  UNIMPLEMENTED("PyCapsule_GetDestructor");
}

PY_EXPORT const char* PyCapsule_GetName(PyObject* /* o */) {
  UNIMPLEMENTED("PyCapsule_GetName");
}

PY_EXPORT void* PyCapsule_GetPointer(PyObject* /* o */, const char* /* e */) {
  UNIMPLEMENTED("PyCapsule_GetPointer");
}

PY_EXPORT void* PyCapsule_Import(const char* /* e */, int /* k */) {
  UNIMPLEMENTED("PyCapsule_Import");
}

PY_EXPORT int PyCapsule_IsValid(PyObject* /* o */, const char* /* e */) {
  UNIMPLEMENTED("PyCapsule_IsValid");
}

PY_EXPORT PyObject* PyCapsule_New(void* /* r */, const char* /* e */,
                                  PyCapsule_Destructor /* r */) {
  UNIMPLEMENTED("PyCapsule_New");
}

PY_EXPORT int PyCapsule_SetContext(PyObject* /* o */, void* /* t */) {
  UNIMPLEMENTED("PyCapsule_SetContext");
}

PY_EXPORT int PyCapsule_SetDestructor(PyObject* /* o */,
                                      PyCapsule_Destructor /* r */) {
  UNIMPLEMENTED("PyCapsule_SetDestructor");
}

PY_EXPORT int PyCapsule_SetName(PyObject* /* o */, const char* /* e */) {
  UNIMPLEMENTED("PyCapsule_SetName");
}

PY_EXPORT int PyCapsule_SetPointer(PyObject* /* o */, void* /* r */) {
  UNIMPLEMENTED("PyCapsule_SetPointer");
}

}  // namespace python
