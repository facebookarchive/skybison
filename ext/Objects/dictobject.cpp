/* dictobject.c implementation */

#include "Python.h"

#include "runtime/handles.h"
#include "runtime/objects.h"
#include "runtime/runtime.h"

namespace py = python;

int PyDict_SetItem(PyObject* pydict, PyObject* key, PyObject* value) {
  py::Thread* thread = py::Thread::currentThread();
  py::Runtime* runtime = thread->runtime();
  py::HandleScope scope(thread->handles());

  py::Handle<py::Object> dictobj(
      &scope, runtime->asObject(Py_AsApiHandle(pydict)));
  if (!dictobj->isDictionary()) {
    return -1;
  }

  py::Handle<py::Object> keyobj(&scope, runtime->asObject(Py_AsApiHandle(key)));
  py::Handle<py::Object> valueobj(
      &scope, runtime->asObject(Py_AsApiHandle(value)));
  py::Handle<py::Dictionary> dict(&scope, *dictobj);
  runtime->dictionaryAtPutInValueCell(dict, keyobj, valueobj);
  Py_INCREF(key);
  Py_INCREF(value);
  return 0;
}

int PyDict_SetItemString(PyObject* pydict, const char* key, PyObject* value) {
  py::Thread* thread = py::Thread::currentThread();
  py::Runtime* runtime = thread->runtime();
  py::HandleScope scope(thread->handles());

  py::Handle<py::Object> keyobj(&scope, runtime->newStringFromCString(key));
  return PyDict_SetItem(
      pydict, Py_AsPyObject(runtime->asApiHandle(*keyobj)), value);
}

PyObject* PyDict_New(void) {
  py::Thread* thread = py::Thread::currentThread();
  py::Runtime* runtime = thread->runtime();
  py::HandleScope scope(thread->handles());

  py::Handle<py::Object> value(&scope, runtime->newDictionary());
  return Py_AsPyObject(runtime->asApiHandle(*value));
}
