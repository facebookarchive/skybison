// dictobject.c implementation

#include "handles.h"
#include "objects.h"
#include "runtime.h"

namespace python {

PY_EXPORT int PyDict_CheckExact_Func(PyObject* obj) {
  return ApiHandle::fromPyObject(obj)->asObject()->isDict();
}

PY_EXPORT int PyDict_Check_Func(PyObject* obj) {
  if (PyDict_CheckExact_Func(obj)) {
    return true;
  }
  return ApiHandle::fromPyObject(obj)->isSubClass(Thread::currentThread(),
                                                  LayoutId::kDict);
}

PY_EXPORT int PyDict_SetItem(PyObject* pydict, PyObject* key, PyObject* value) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Object dictobj(&scope, ApiHandle::fromPyObject(pydict)->asObject());
  if (!dictobj->isDict()) {
    return -1;
  }

  Object keyobj(&scope, ApiHandle::fromPyObject(key)->asObject());
  Object valueobj(&scope, ApiHandle::fromPyObject(value)->asObject());
  Dict dict(&scope, *dictobj);
  runtime->dictAtPut(dict, keyobj, valueobj);
  // TODO(eelizondo): increment the reference count through ApiHandle
  key->ob_refcnt++;
  value->ob_refcnt++;
  return 0;
}

PY_EXPORT int PyDict_SetItemString(PyObject* pydict, const char* key,
                                   PyObject* value) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Object keyobj(&scope, runtime->newStrFromCStr(key));
  return PyDict_SetItem(pydict, ApiHandle::fromObject(*keyobj), value);
}

PY_EXPORT PyObject* PyDict_New() {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Object value(&scope, runtime->newDict());
  return ApiHandle::fromObject(*value);
}

PY_EXPORT PyObject* PyDict_GetItem(PyObject* pydict, PyObject* key) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Object dictobj(&scope, ApiHandle::fromPyObject(pydict)->asObject());
  if (!dictobj->isDict()) {
    return nullptr;
  }
  Dict dict(&scope, *dictobj);
  Object key_obj(&scope, ApiHandle::fromPyObject(key)->asObject());
  RawObject value = runtime->dictAt(dict, key_obj);
  if (value->isError()) {
    return nullptr;
  }
  return ApiHandle::fromBorrowedObject(value);
}

PY_EXPORT void PyDict_Clear(PyObject* /* p */) {
  UNIMPLEMENTED("PyDict_Clear");
}

PY_EXPORT int PyDict_ClearFreeList() { return 0; }

PY_EXPORT int PyDict_Contains(PyObject* /* p */, PyObject* /* y */) {
  UNIMPLEMENTED("PyDict_Contains");
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

PY_EXPORT PyObject* PyDict_GetItemString(PyObject* /* v */,
                                         const char* /* y */) {
  UNIMPLEMENTED("PyDict_GetItemString");
}

PY_EXPORT PyObject* PyDict_GetItemWithError(PyObject* /* p */,
                                            PyObject* /* y */) {
  UNIMPLEMENTED("PyDict_GetItemWithError");
}

PY_EXPORT PyObject* PyDict_Items(PyObject* /* p */) {
  UNIMPLEMENTED("PyDict_Items");
}

PY_EXPORT PyObject* PyDict_Keys(PyObject* /* p */) {
  UNIMPLEMENTED("PyDict_Keys");
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
    // TODO(wmeehan) replace with PyErr_BadInternalCall
    thread->raiseSystemErrorWithCStr("bad argument to internal function");
    return -1;
  }

  Dict dict(&scope, *dict_obj);
  return dict->numItems();
}

PY_EXPORT int PyDict_Update(PyObject* /* a */, PyObject* /* b */) {
  UNIMPLEMENTED("PyDict_Update");
}

PY_EXPORT PyObject* PyDict_Values(PyObject* /* p */) {
  UNIMPLEMENTED("PyDict_Values");
}

PY_EXPORT PyObject* PyObject_GenericGetDict(PyObject* /* j */, void* /* t */) {
  UNIMPLEMENTED("PyObject_GenericGetDict");
}

}  // namespace python
