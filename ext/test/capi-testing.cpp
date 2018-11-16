#include "capi-testing.h"

namespace python {

int _IsBorrowed(PyObject* pyobj) {
  return ApiHandle::fromPyObject(pyobj)->isBorrowed();
}

}  // namespace python
