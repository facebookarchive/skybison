/* unicodeobject.c implementation */

#include "Python.h"

#include "runtime/handles.h"
#include "runtime/objects.h"
#include "runtime/runtime.h"

namespace py = python;

PyObject* PyUnicode_FromString(const char* c_string) {
  py::Thread* thread = py::Thread::currentThread();
  py::Runtime* runtime = thread->runtime();
  py::HandleScope scope(thread->handles());

  py::Handle<py::Object> value(&scope, runtime->newStringFromCString(c_string));
  return Py_AsPyObject(runtime->asApiHandle(*value));
}
