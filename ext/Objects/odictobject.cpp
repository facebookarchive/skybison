#include "runtime.h"

namespace python {

PY_EXPORT int PyODict_DelItem(PyObject* /* d */, PyObject* /* y */) {
  UNIMPLEMENTED("PyODict_DelItem");
}

PY_EXPORT PyObject* PyODict_New(void) { UNIMPLEMENTED("PyODict_New"); }

PY_EXPORT int PyODict_SetItem(PyObject* /* d */, PyObject* /* y */,
                              PyObject* /* e */) {
  UNIMPLEMENTED("PyODict_SetItem");
}

}  // namespace python
