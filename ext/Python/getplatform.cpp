#include "os.h"

namespace py {

PY_EXPORT const char* Py_GetPlatform() { return OS::name(); }

}  // namespace py
