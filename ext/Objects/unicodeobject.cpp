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
  return runtime->asApiHandle(*value)->asPyObject();
}

PyObject* _PyUnicode_FromId(_Py_Identifier* id) {
  py::Thread* thread = py::Thread::currentThread();
  py::Runtime* runtime = thread->runtime();
  py::HandleScope scope(thread->handles());

  py::Handle<py::Object> result(
      &scope, runtime->internStringFromCString(id->string));
  return runtime->asApiHandle(*result)->asPyObject();
}

char* PyUnicode_AsUTF8AndSize(PyObject* pyunicode, Py_ssize_t* size) {
  py::Thread* thread = py::Thread::currentThread();
  py::Runtime* runtime = thread->runtime();
  py::HandleScope scope(thread->handles());

  if (pyunicode == nullptr) {
    PyErr_BadArgument();
    return nullptr;
  }

  py::Handle<py::Object> str_obj(&scope, runtime->asObject(pyunicode));
  if (!str_obj->isString()) {
    return nullptr;
  }

  py::Handle<py::String> str(&scope, *str_obj);
  word length = str->length();
  byte* result = static_cast<byte*>(std::malloc(length + 1));
  str->copyTo(result, length);
  result[length] = '\0';
  if (size) {
    *size = length;
  }
  return reinterpret_cast<char*>(result);
}
