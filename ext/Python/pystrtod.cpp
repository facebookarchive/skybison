#include "runtime.h"

namespace python {

PY_EXPORT double PyOS_string_to_double(const char* /* s */, char** /* endptr */,
                                       PyObject* /* n */) {
  UNIMPLEMENTED("PyOS_string_to_double");
}

}  // namespace python
