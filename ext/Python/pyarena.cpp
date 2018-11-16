#include "runtime.h"

namespace python {

struct PyArena;

PY_EXPORT int PyArena_AddPyObject(PyArena* /* a */, PyObject* /* j */) {
  UNIMPLEMENTED("PyArena_AddPyObject");
}

PY_EXPORT void PyArena_Free(PyArena* /* a */) { UNIMPLEMENTED("PyArena_Free"); }

PY_EXPORT void* PyArena_Malloc(PyArena* /* a */, size_t /* e */) {
  UNIMPLEMENTED("PyArena_Malloc");
}

PY_EXPORT PyArena* PyArena_New() { UNIMPLEMENTED("PyArena_New"); }

}  // namespace python
