// dictobject.c implementation

#include "Python.h"

#include "handles.h"
#include "objects.h"
#include "runtime.h"

namespace python {

void PyDict_Type_Init(void) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();

  ApiTypeHandle* pytype_type =
      runtime->builtinTypeHandles(ExtensionTypes::kType);
  ApiTypeHandle* pydict_type =
      ApiTypeHandle::newTypeHandle("dict", pytype_type);

  runtime->addBuiltinTypeHandle(pydict_type);
}

extern "C" PyTypeObject* PyDict_Type_Ptr() {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  return static_cast<PyTypeObject*>(
      runtime->builtinTypeHandles(ExtensionTypes::kDict));
}

extern "C" int PyDict_SetItem(PyObject* pydict, PyObject* key,
                              PyObject* value) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Handle<Object> dictobj(&scope, ApiHandle::fromPyObject(pydict)->asObject());
  if (!dictobj->isDictionary()) {
    return -1;
  }

  Handle<Object> keyobj(&scope, ApiHandle::fromPyObject(key)->asObject());
  Handle<Object> valueobj(&scope, ApiHandle::fromPyObject(value)->asObject());
  Handle<Dictionary> dict(&scope, *dictobj);
  runtime->dictionaryAtPutInValueCell(dict, keyobj, valueobj);
  Py_INCREF(key);
  Py_INCREF(value);
  return 0;
}

extern "C" int PyDict_SetItemString(PyObject* pydict, const char* key,
                                    PyObject* value) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Handle<Object> keyobj(&scope, runtime->newStringFromCString(key));
  return PyDict_SetItem(pydict, ApiHandle::fromObject(*keyobj)->asPyObject(),
                        value);
}

extern "C" PyObject* PyDict_New(void) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Handle<Object> value(&scope, runtime->newDictionary());
  return ApiHandle::fromObject(*value)->asPyObject();
}

extern "C" PyObject* PyDict_GetItem(PyObject* pydict, PyObject* key) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Handle<Object> dictobj(&scope, ApiHandle::fromPyObject(pydict)->asObject());
  if (!dictobj->isDictionary()) {
    return nullptr;
  }
  Handle<Dictionary> dict(&scope, *dictobj);
  Handle<Object> key_obj(&scope, ApiHandle::fromPyObject(key)->asObject());
  Object* value = runtime->dictionaryAt(dict, key_obj);
  if (value->isError()) {
    return nullptr;
  }
  return ApiHandle::fromBorrowedObject(value)->asPyObject();
}

}  // namespace python
