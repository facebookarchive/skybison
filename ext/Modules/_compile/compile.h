#pragma once

#include <climits>

#include "cpython-types.h"

namespace py {

namespace compile {

/* Future feature support */

PyCodeObject* PyAST_CompileObject(_mod* mod, PyObject* filename,
                                  PyCompilerFlags* flags, int optimize,
                                  PyArena* arena);

PyCodeObject* PyAST_Compile(struct _mod* mod, const char* filename,
                            PyCompilerFlags* flags, PyArena* arena);

PyObject* PyCode_Optimize(PyObject* code, PyObject* consts, PyObject* names,
                          PyObject* lnotab);

#define PY_INVALID_STACK_EFFECT INT_MAX

}  // namespace compile

}  // namespace py
