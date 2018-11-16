#include "runtime.h"

namespace python {

extern "C" int PyODict_DelItem(PyObject* /* d */, PyObject* /* y */) {
  UNIMPLEMENTED("PyODict_DelItem");
}

extern "C" PyObject* PyODict_New(void) { UNIMPLEMENTED("PyODict_New"); }

extern "C" int PyODict_SetItem(PyObject* /* d */, PyObject* /* y */,
                               PyObject* /* e */) {
  UNIMPLEMENTED("PyODict_SetItem");
}

}  // namespace python
