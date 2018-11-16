#include "runtime.h"

namespace python {

PY_EXPORT PyObject* PyInstanceMethod_New(PyObject* /* c */) {
  UNIMPLEMENTED("PyInstanceMethod_New");
}

PY_EXPORT PyObject* PyMethod_New(PyObject* /* c */, PyObject* /* f */) {
  UNIMPLEMENTED("PyMethod_New");
}

}  // namespace python
