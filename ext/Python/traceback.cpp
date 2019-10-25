#include "capi-handles.h"

namespace py {

struct PyFrameObject;

PY_EXPORT int PyTraceBack_Here(PyFrameObject* /* e */) {
  UNIMPLEMENTED("PyTraceBack_Here");
}

PY_EXPORT int PyTraceBack_Print(PyObject* /* v */, PyObject* /* f */) {
  UNIMPLEMENTED("PyTraceBack_Print");
}

}  // namespace py
