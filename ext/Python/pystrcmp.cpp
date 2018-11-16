#include "runtime.h"

namespace python {

PY_EXPORT int PyOS_mystricmp(const char* /* 1 */, const char* /* 2 */) {
  UNIMPLEMENTED("PyOS_mystricmp");
}

PY_EXPORT int PyOS_mystrnicmp(const char* /* 1 */, const char* /* 2 */,
                              Py_ssize_t /* e */) {
  UNIMPLEMENTED("PyOS_mystrnicmp");
}

}  // namespace python
