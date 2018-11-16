#include "runtime.h"

namespace python {

PY_EXPORT long PyOS_strtol(const char* /* r */, char** /* ptr */, int /* e */) {
  UNIMPLEMENTED("PyOS_strtol");
}

}  // namespace python
