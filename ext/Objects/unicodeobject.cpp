/* unicodeobject.c implementation */

#include "Python.h"

#include "runtime/handles.h"
#include "runtime/objects.h"
#include "runtime/runtime.h"

namespace python {

extern "C" PyObject* PyUnicode_FromString(const char* c_string) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread->handles());

  Handle<Object> value(&scope, runtime->newStringFromCString(c_string));
  return runtime->asApiHandle(*value)->asPyObject();
}

extern "C" PyObject* _PyUnicode_FromId(_Py_Identifier* id) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread->handles());

  Handle<Object> result(&scope, runtime->internStringFromCString(id->string));
  return runtime->asApiHandle(*result)->asPyObject();
}

extern "C" char* PyUnicode_AsUTF8AndSize(
    PyObject* pyunicode,
    Py_ssize_t* size) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread->handles());

  if (pyunicode == nullptr) {
    PyErr_BadArgument();
    return nullptr;
  }

  Handle<Object> str_obj(&scope, runtime->asObject(pyunicode));
  if (!str_obj->isString()) {
    return nullptr;
  }

  Handle<String> str(&scope, *str_obj);
  word length = str->length();
  byte* result = static_cast<byte*>(std::malloc(length + 1));
  str->copyTo(result, length);
  result[length] = '\0';
  if (size) {
    *size = length;
  }
  return reinterpret_cast<char*>(result);
}

} // namespace python
