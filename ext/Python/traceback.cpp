#include "runtime.h"

namespace python {

struct PyFrameObject;

extern "C" int PyTraceBack_Here(PyFrameObject* /* e */) {
  UNIMPLEMENTED("PyTraceBack_Here");
}

extern "C" int PyTraceBack_Print(PyObject* /* v */, PyObject* /* f */) {
  UNIMPLEMENTED("PyTraceBack_Print");
}

}  // namespace python
