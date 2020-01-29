#include "cpython-func.h"

#include "globals.h"
#include "version.h"

namespace py {

PY_EXPORT const char* Py_GetCompiler() { return compilerInfo(); }

}  // namespace py
