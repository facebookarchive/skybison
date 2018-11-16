#include "runtime.h"

namespace python {

extern "C" int _PyObject_IsBorrowed(PyObject* pyobj) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  return ApiHandle::fromPyObject(pyobj)->isBorrowed();
}

}  // namespace python
