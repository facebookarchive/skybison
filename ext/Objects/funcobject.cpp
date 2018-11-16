#include "runtime.h"

namespace python {

extern "C" PyObject* PyClassMethod_New(PyObject* /* e */) {
  UNIMPLEMENTED("PyClassMethod_New");
}

extern "C" PyObject* PyStaticMethod_New(PyObject* /* e */) {
  UNIMPLEMENTED("PyStaticMethod_New");
}

}  // namespace python
