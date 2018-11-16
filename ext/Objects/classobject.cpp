#include "runtime.h"

namespace python {

extern "C" PyObject* PyInstanceMethod_New(PyObject* /* c */) {
  UNIMPLEMENTED("PyInstanceMethod_New");
}

extern "C" PyObject* PyMethod_New(PyObject* /* c */, PyObject* /* f */) {
  UNIMPLEMENTED("PyMethod_New");
}

}  // namespace python
