/* dictobject.c implementation */

#include "Python.h"

#include "runtime/handles.h"
#include "runtime/objects.h"
#include "runtime/runtime.h"

namespace python {

extern "C" int
PyDict_SetItem(PyObject* pydict, PyObject* key, PyObject* value) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread->handles());

  Handle<Object> dictobj(&scope, runtime->asObject(pydict));
  if (!dictobj->isDictionary()) {
    return -1;
  }

  Handle<Object> keyobj(&scope, runtime->asObject(key));
  Handle<Object> valueobj(&scope, runtime->asObject(value));
  Handle<Dictionary> dict(&scope, *dictobj);
  runtime->dictionaryAtPutInValueCell(dict, keyobj, valueobj);
  Py_INCREF(key);
  Py_INCREF(value);
  return 0;
}

extern "C" int
PyDict_SetItemString(PyObject* pydict, const char* key, PyObject* value) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread->handles());

  Handle<Object> keyobj(&scope, runtime->newStringFromCString(key));
  return PyDict_SetItem(
      pydict, runtime->asApiHandle(*keyobj)->asPyObject(), value);
}

extern "C" PyObject* PyDict_New(void) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread->handles());

  Handle<Object> value(&scope, runtime->newDictionary());
  return runtime->asApiHandle(*value)->asPyObject();
}

extern "C" PyObject* PyDict_GetItem(PyObject* pydict, PyObject* key) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread->handles());

  Handle<Object> dictobj(&scope, runtime->asObject(pydict));
  if (!dictobj->isDictionary()) {
    return nullptr;
  }
  Handle<Dictionary> dict(&scope, *dictobj);
  Handle<Object> key_obj(&scope, runtime->asObject(key));
  Object* value = runtime->dictionaryAt(dict, key_obj);
  if (value->isError()) {
    return nullptr;
  }
  return runtime->asBorrowedApiHandle(value)->asPyObject();
}

} // namespace python
