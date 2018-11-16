#include "runtime.h"

namespace python {

PY_EXPORT PyObject* PySeqIter_New(PyObject* /* q */) {
  UNIMPLEMENTED("PySeqIter_New");
}

PY_EXPORT PyObject* PyCallIter_New(PyObject* /* e */, PyObject* /* l */) {
  UNIMPLEMENTED("PyCallIter_New");
}

}  // namespace python
