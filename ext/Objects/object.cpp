// object.c implementation

#include "cpython-func.h"
#include "runtime.h"

namespace python {

extern "C" PyObject* PyNone_Ptr() {
  return ApiHandle::fromObject(None::object())->asPyObject();
}

extern "C" void _Py_Dealloc_Func(PyObject* obj) { std::free(obj); }

}  // namespace python
