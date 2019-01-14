// dictobject.c implementation

#include "dict-builtins.h"
#include "handles.h"
#include "objects.h"
#include "runtime.h"

namespace python {

PY_EXPORT int PyDict_CheckExact_Func(PyObject* obj) {
  return ApiHandle::fromPyObject(obj)->asObject()->isDict();
}

PY_EXPORT int PyDict_Check_Func(PyObject* obj) {
  return Thread::currentThread()->runtime()->isInstanceOfDict(
      ApiHandle::fromPyObject(obj)->asObject());
}

PY_EXPORT int PyDict_SetItem(PyObject* pydict, PyObject* key, PyObject* value) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Object dictobj(&scope, ApiHandle::fromPyObject(pydict)->asObject());

  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfDict(dictobj)) {
    thread->raiseBadInternalCall();
    return -1;
  }

  Dict dict(&scope, *dictobj);
  Object keyobj(&scope, ApiHandle::fromPyObject(key)->asObject());
  Object valueobj(&scope, ApiHandle::fromPyObject(value)->asObject());
  runtime->dictAtPut(dict, keyobj, valueobj);
  return 0;
}

PY_EXPORT int PyDict_SetItemString(PyObject* pydict, const char* key,
                                   PyObject* value) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Object keyobj(&scope, runtime->newStrFromCStr(key));
  return PyDict_SetItem(pydict, ApiHandle::newReference(thread, *keyobj),
                        value);
}

PY_EXPORT PyObject* PyDict_New() {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Object value(&scope, runtime->newDict());
  return ApiHandle::newReference(thread, *value);
}

static PyObject* getItem(Thread* thread, const Object& dict_obj,
                         const Object& key_obj) {
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfDict(dict_obj)) return nullptr;

  HandleScope scope(thread);
  Dict dict(&scope, *dict_obj);
  Object value(&scope, runtime->dictAt(dict, key_obj));
  if (value->isError()) return nullptr;
  return ApiHandle::borrowedReference(thread, *value);
}

PY_EXPORT PyObject* PyDict_GetItem(PyObject* pydict, PyObject* key) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Object dict(&scope, ApiHandle::fromPyObject(pydict)->asObject());
  Object key_obj(&scope, ApiHandle::fromPyObject(key)->asObject());
  return getItem(thread, dict, key_obj);
}

PY_EXPORT PyObject* PyDict_GetItemString(PyObject* pydict, const char* key) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Object dict(&scope, ApiHandle::fromPyObject(pydict)->asObject());
  Object key_obj(&scope, thread->runtime()->newStrFromCStr(key));
  return getItem(thread, dict, key_obj);
}

PY_EXPORT void PyDict_Clear(PyObject* /* p */) {
  UNIMPLEMENTED("PyDict_Clear");
}

PY_EXPORT int PyDict_ClearFreeList() { return 0; }

PY_EXPORT int PyDict_Contains(PyObject* pydict, PyObject* key) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Dict dict(&scope, ApiHandle::fromPyObject(pydict)->asObject());
  Object key_obj(&scope, ApiHandle::fromPyObject(key)->asObject());
  // TODO(T36757907): Return -1 when dictIncludes fails to hash the key
  return runtime->dictIncludes(dict, key_obj);
}

PY_EXPORT PyObject* PyDict_Copy(PyObject* /* o */) {
  UNIMPLEMENTED("PyDict_Copy");
}

PY_EXPORT int PyDict_DelItem(PyObject* /* p */, PyObject* /* y */) {
  UNIMPLEMENTED("PyDict_DelItem");
}

PY_EXPORT int PyDict_DelItemString(PyObject* /* v */, const char* /* y */) {
  UNIMPLEMENTED("PyDict_DelItemString");
}

PY_EXPORT PyObject* PyDict_GetItemWithError(PyObject* /* p */,
                                            PyObject* /* y */) {
  UNIMPLEMENTED("PyDict_GetItemWithError");
}

PY_EXPORT PyObject* PyDict_Items(PyObject* pydict) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Object dict_obj(&scope, ApiHandle::fromPyObject(pydict)->asObject());
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfDict(dict_obj)) {
    thread->raiseBadInternalCall();
    return nullptr;
  }
  Dict dict(&scope, *dict_obj);
  List items(&scope, runtime->newList());
  items->setNumItems(dict->numItems());
  items->setItems(runtime->dictItems(thread, dict));
  return ApiHandle::newReference(thread, *items);
}

PY_EXPORT PyObject* PyDict_Keys(PyObject* pydict) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Object dict_obj(&scope, ApiHandle::fromPyObject(pydict)->asObject());
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfDict(dict_obj)) {
    thread->raiseBadInternalCall();
    return nullptr;
  }
  Dict dict(&scope, *dict_obj);
  Tuple keys_tuple(&scope, runtime->dictKeys(dict));
  List keys(&scope, runtime->newList());
  keys->setItems(*keys_tuple);
  keys->setNumItems(keys_tuple->length());
  return ApiHandle::newReference(thread, *keys);
}

PY_EXPORT int PyDict_Merge(PyObject* /* a */, PyObject* /* b */, int /* e */) {
  UNIMPLEMENTED("PyDict_Merge");
}

PY_EXPORT int PyDict_MergeFromSeq2(PyObject* /* d */, PyObject* /* 2 */,
                                   int /* e */) {
  UNIMPLEMENTED("PyDict_MergeFromSeq2");
}

PY_EXPORT int PyDict_Next(PyObject* /* p */, Py_ssize_t* /* s */,
                          PyObject** /* pkey */, PyObject** /* pvalue */) {
  UNIMPLEMENTED("PyDict_Next");
}

PY_EXPORT Py_ssize_t PyDict_Size(PyObject* p) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Object dict_obj(&scope, ApiHandle::fromPyObject(p)->asObject());
  if (!runtime->isInstanceOfDict(dict_obj)) {
    thread->raiseBadInternalCall();
    return -1;
  }

  Dict dict(&scope, *dict_obj);
  return dict->numItems();
}

PY_EXPORT int PyDict_Update(PyObject* /* a */, PyObject* /* b */) {
  UNIMPLEMENTED("PyDict_Update");
}

PY_EXPORT PyObject* PyDict_Values(PyObject* pydict) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Object dict_obj(&scope, ApiHandle::fromPyObject(pydict)->asObject());
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfDict(dict_obj)) {
    thread->raiseBadInternalCall();
    return nullptr;
  }
  Dict dict(&scope, *dict_obj);
  List values(&scope, runtime->newList());
  values->setNumItems(dict->numItems());
  values->setItems(runtime->dictValues(thread, dict));
  return ApiHandle::newReference(thread, *values);
}

PY_EXPORT PyObject* PyObject_GenericGetDict(PyObject* /* j */, void* /* t */) {
  UNIMPLEMENTED("PyObject_GenericGetDict");
}

}  // namespace python
