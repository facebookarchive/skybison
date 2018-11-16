#include "runtime.h"

namespace python {

extern "C" int PyOS_mystricmp(const char* /* 1 */, const char* /* 2 */) {
  UNIMPLEMENTED("PyOS_mystricmp");
}

extern "C" int PyOS_mystrnicmp(const char* /* 1 */, const char* /* 2 */,
                               Py_ssize_t /* e */) {
  UNIMPLEMENTED("PyOS_mystrnicmp");
}

}  // namespace python
