#include "runtime.h"

namespace python {

struct PyArena;

extern "C" int PyArena_AddPyObject(PyArena* /* a */, PyObject* /* j */) {
  UNIMPLEMENTED("PyArena_AddPyObject");
}

extern "C" void PyArena_Free(PyArena* /* a */) {
  UNIMPLEMENTED("PyArena_Free");
}

extern "C" void* PyArena_Malloc(PyArena* /* a */, size_t /* e */) {
  UNIMPLEMENTED("PyArena_Malloc");
}

extern "C" PyArena* PyArena_New() { UNIMPLEMENTED("PyArena_New"); }

}  // namespace python
