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

  py::Handle<py::Object> dictobj(&scope, runtime->asObject(pydict));
  if (!dictobj->isDictionary()) {
    return -1;
  }

  py::Handle<py::Object> keyobj(&scope, runtime->asObject(key));
  py::Handle<py::Object> valueobj(&scope, runtime->asObject(value));
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
      pydict, runtime->asApiHandle(*keyobj)->asPyObject(), value);
}

PyObject* PyDict_New(void) {
  py::Thread* thread = py::Thread::currentThread();
  py::Runtime* runtime = thread->runtime();
  py::HandleScope scope(thread->handles());

  py::Handle<py::Object> value(&scope, runtime->newDictionary());
  return runtime->asApiHandle(*value)->asPyObject();
}

PyObject* PyDict_GetItem(PyObject* pydict, PyObject* key) {
  py::Thread* thread = py::Thread::currentThread();
  py::Runtime* runtime = thread->runtime();
  py::HandleScope scope(thread->handles());

  py::Handle<py::Object> dictobj(&scope, runtime->asObject(pydict));
  if (!dictobj->isDictionary()) {
    return nullptr;
  }
  py::Handle<py::Dictionary> dict(&scope, *dictobj);
  py::Handle<py::Object> key_obj(&scope, runtime->asObject(key));
  py::Object* value = runtime->dictionaryAt(dict, key_obj);
  if (value->isError()) {
    return nullptr;
  }
  return runtime->asBorrowedApiHandle(value)->asPyObject();
}
