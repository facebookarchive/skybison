#include "runtime.h"

namespace python {

typedef void (*PyCapsule_Destructor)(PyObject*);

extern "C" void* PyCapsule_GetContext(PyObject* /* o */) {
  UNIMPLEMENTED("PyCapsule_GetContext");
}

extern "C" PyCapsule_Destructor PyCapsule_GetDestructor(PyObject* /* o */) {
  UNIMPLEMENTED("PyCapsule_GetDestructor");
}

extern "C" const char* PyCapsule_GetName(PyObject* /* o */) {
  UNIMPLEMENTED("PyCapsule_GetName");
}

extern "C" void* PyCapsule_GetPointer(PyObject* /* o */, const char* /* e */) {
  UNIMPLEMENTED("PyCapsule_GetPointer");
}

extern "C" void* PyCapsule_Import(const char* /* e */, int /* k */) {
  UNIMPLEMENTED("PyCapsule_Import");
}

extern "C" int PyCapsule_IsValid(PyObject* /* o */, const char* /* e */) {
  UNIMPLEMENTED("PyCapsule_IsValid");
}

extern "C" PyObject* PyCapsule_New(void* /* r */, const char* /* e */,
                                   PyCapsule_Destructor /* r */) {
  UNIMPLEMENTED("PyCapsule_New");
}

extern "C" int PyCapsule_SetContext(PyObject* /* o */, void* /* t */) {
  UNIMPLEMENTED("PyCapsule_SetContext");
}

extern "C" int PyCapsule_SetDestructor(PyObject* /* o */,
                                       PyCapsule_Destructor /* r */) {
  UNIMPLEMENTED("PyCapsule_SetDestructor");
}

extern "C" int PyCapsule_SetName(PyObject* /* o */, const char* /* e */) {
  UNIMPLEMENTED("PyCapsule_SetName");
}

extern "C" int PyCapsule_SetPointer(PyObject* /* o */, void* /* r */) {
  UNIMPLEMENTED("PyCapsule_SetPointer");
}

}  // namespace python
