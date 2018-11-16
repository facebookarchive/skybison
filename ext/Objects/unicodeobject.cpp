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
  return runtime->asPyObject(*value);
}

PyObject* _PyUnicode_FromId(_Py_Identifier* id) {
  py::Thread* thread = py::Thread::currentThread();
  py::Runtime* runtime = thread->runtime();
  py::HandleScope scope(thread->handles());

  py::Handle<py::Object> result(
      &scope, runtime->internStringFromCString(id->string));
  return runtime->asPyObject(*result);
}
