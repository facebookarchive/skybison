#include "runtime.h"

namespace python {

extern "C" PyObject* PySeqIter_New(PyObject* /* q */) {
  UNIMPLEMENTED("PySeqIter_New");
}

extern "C" PyObject* PyCallIter_New(PyObject* /* e */, PyObject* /* l */) {
  UNIMPLEMENTED("PyCallIter_New");
}

}  // namespace python
