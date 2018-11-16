#include "runtime.h"

namespace python {

extern "C" int PyOS_snprintf(char* /* r */, size_t /* e */, const char* /* t */,
                             ...) {
  UNIMPLEMENTED("PyOS_snprintf");
}

extern "C" int PyOS_vsnprintf(char* /* r */, size_t /* e */,
                              const char* /* t */, va_list /* a */) {
  UNIMPLEMENTED("PyOS_vsnprintf");
}

}  // namespace python
