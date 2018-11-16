#include "runtime.h"

namespace python {

struct PyCodeObject;

extern "C" PyCodeObject* PyCode_New(int /* t */, int /* t */, int /* s */,
                                    int /* e */, int /* s */, PyObject* /* e */,
                                    PyObject* /* s */, PyObject* /* s */,
                                    PyObject* /* s */, PyObject* /* s */,
                                    PyObject* /* s */, PyObject* /* e */,
                                    PyObject* /* e */, int /* o */,
                                    PyObject* /* b */) {
  UNIMPLEMENTED("PyCode_New");
}

}  // namespace python
