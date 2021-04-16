#include "api-handle.h"
#include "runtime.h"

namespace py {

PY_EXPORT void PyFrame_FastToLocals(PyFrameObject* /* f */) {
  UNIMPLEMENTED("PyFrame_FastToLocals");
}

PY_EXPORT int PyFrame_GetLineNumber(PyFrameObject* /* f */) {
  UNIMPLEMENTED("PyFrame_GetLineNumber");
}

PY_EXPORT void PyFrame_LocalsToFast(PyFrameObject* /* f */, int /* r */) {
  UNIMPLEMENTED("PyFrame_LocalsToFast");
}

PY_EXPORT PyFrameObject* PyFrame_New(PyThreadState* /* e */,
                                     PyCodeObject* /* e */, PyObject* /* s */,
                                     PyObject* /* s */) {
  UNIMPLEMENTED("PyFrame_New");
}

}  // namespace py
