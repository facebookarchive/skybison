// errors.c implementation

#include "cpython-func.h"
#include "utils.h"

void PyErr_SetString(PyObject*, const char*) {
  UNIMPLEMENTED("PyErr_SetString");
}

PyObject* PyErr_Occurred(void) { UNIMPLEMENTED("PyErr_Ocurred"); }

PyObject* PyErr_Format(PyObject*, const char*, ...) {
  UNIMPLEMENTED("PyErr_Format");
}

void PyErr_Clear(void) { UNIMPLEMENTED("PyErr_Clear"); }

int PyErr_BadArgument(void) { UNIMPLEMENTED("PyErr_BadArgument"); }

PyObject* PyErr_NoMemory(void) { UNIMPLEMENTED("PyErr_NoMemory"); }

void Py_FatalError(const char*) { UNIMPLEMENTED("Py_FatalError"); }

PyObject* _PyErr_FormatFromCause(PyObject*, const char*, ...) {
  UNIMPLEMENTED("_PyErr_FormatFromCause");
}
