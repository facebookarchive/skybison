// floatobject.c implementation

#include "objects.h"
#include "runtime.h"

namespace python {

extern "C" PyObject* PyFloat_FromDouble(double fval) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Handle<Object> flt(&scope, runtime->newFloat(fval));
  return ApiHandle::fromObject(*flt)->asPyObject();
}

extern "C" double PyFloat_AsDouble(PyObject*) {
  UNIMPLEMENTED("PyFloat_AsDouble");
}

}  // namespace python
