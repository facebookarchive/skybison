#include "runtime.h"

namespace python {

struct PyFrameObject;
struct PyThreadState;
struct PyCodeObject;

extern "C" void PyFrame_FastToLocals(PyFrameObject* /* f */) {
  UNIMPLEMENTED("PyFrame_FastToLocals");
}

extern "C" int PyFrame_GetLineNumber(PyFrameObject* /* f */) {
  UNIMPLEMENTED("PyFrame_GetLineNumber");
}

extern "C" void PyFrame_LocalsToFast(PyFrameObject* /* f */, int /* r */) {
  UNIMPLEMENTED("PyFrame_LocalsToFast");
}

extern "C" PyFrameObject* PyFrame_New(PyThreadState* /* e */,
                                      PyCodeObject* /* e */, PyObject* /* s */,
                                      PyObject* /* s */) {
  UNIMPLEMENTED("PyFrame_New");
}

}  // namespace python
