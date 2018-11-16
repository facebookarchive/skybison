/* longobject.c implementation */

#include "Python.h"

#include "runtime/handles.h"
#include "runtime/objects.h"
#include "runtime/runtime.h"

namespace py = python;

extern "C" {

PyObject* PyLong_FromLong(long ival) {
  py::Thread* thread = py::Thread::currentThread();
  py::Runtime* runtime = thread->runtime();
  py::HandleScope scope(thread->handles());

  word val = reinterpret_cast<word>(ival);
  py::Handle<py::Object> value(&scope, runtime->newInteger(val));
  return runtime->asApiHandle(*value);
}

} /* extern "C" */
