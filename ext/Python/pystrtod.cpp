#include "runtime.h"

namespace python {

extern "C" double PyOS_string_to_double(const char* /* s */,
                                        char** /* endptr */,
                                        PyObject* /* n */) {
  UNIMPLEMENTED("PyOS_string_to_double");
}

}  // namespace python
