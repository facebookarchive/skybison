#include "runtime.h"

namespace python {

extern "C" long PyOS_strtol(const char* /* r */, char** /* ptr */,
                            int /* e */) {
  UNIMPLEMENTED("PyOS_strtol");
}

}  // namespace python
