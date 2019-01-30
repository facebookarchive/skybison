#include "runtime.h"

namespace python {

PY_EXPORT int _PyOS_URandom(void*, Py_ssize_t) {
  UNIMPLEMENTED("_PyOS_URandom");
}

}  // namespace python
