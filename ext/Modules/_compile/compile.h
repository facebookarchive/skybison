#pragma once

#include <climits>
#include "cpython-types.h"

namespace py {

namespace compile {

/* Future feature support */

PyCodeObject* PyAST_CompileObject(_mod* mod, PyObject* filename,
                                  PyCompilerFlags* flags, int optimize,
                                  PyArena* arena);
PyFutureFeatures* PyFuture_FromAST(_mod* mod, const char* filename);
PyFutureFeatures* PyFuture_FromASTObject(_mod* mod, PyObject* filename);

PyCodeObject* PyAST_Compile(mod_ty mod, const char* filename,
                            PyCompilerFlags* flags, PyArena* arena);

PyObject* PyCode_Optimize(PyObject* code, PyObject* consts, PyObject* names,
                          PyObject* lnotab);

#define FUTURE_NESTED_SCOPES "nested_scopes"
#define FUTURE_GENERATORS "generators"
#define FUTURE_DIVISION "division"
#define FUTURE_ABSOLUTE_IMPORT "absolute_import"
#define FUTURE_WITH_STATEMENT "with_statement"
#define FUTURE_PRINT_FUNCTION "print_function"
#define FUTURE_UNICODE_LITERALS "unicode_literals"
#define FUTURE_BARRY_AS_BDFL "barry_as_FLUFL"
#define FUTURE_GENERATOR_STOP "generator_stop"

#define PY_INVALID_STACK_EFFECT INT_MAX

}  // namespace compile

}  // namespace py
