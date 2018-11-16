// dictobject.c implementation

#include "handles.h"
#include "objects.h"
#include "runtime.h"

namespace python {

extern "C" int PyDict_CheckExact_Func(PyObject* obj) {
  return ApiHandle::fromPyObject(obj)->asObject()->isDict();
}

extern "C" int PyDict_Check_Func(PyObject* obj) {
  if (PyDict_CheckExact_Func(obj)) {
    return true;
  }
  return ApiHandle::fromPyObject(obj)->isSubClass(Thread::currentThread(),
                                                  LayoutId::kDict);
}

extern "C" int PyDict_SetItem(PyObject* pydict, PyObject* key,
                              PyObject* value) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Handle<Object> dictobj(&scope, ApiHandle::fromPyObject(pydict)->asObject());
  if (!dictobj->isDict()) {
    return -1;
  }

  Handle<Object> keyobj(&scope, ApiHandle::fromPyObject(key)->asObject());
  Handle<Object> valueobj(&scope, ApiHandle::fromPyObject(value)->asObject());
  Handle<Dict> dict(&scope, *dictobj);
  runtime->dictAtPutInValueCell(dict, keyobj, valueobj);
  // TODO(eelizondo): increment the reference count through ApiHandle
  key->ob_refcnt++;
  value->ob_refcnt++;
  return 0;
}

extern "C" int PyDict_SetItemString(PyObject* pydict, const char* key,
                                    PyObject* value) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Handle<Object> keyobj(&scope, runtime->newStrFromCStr(key));
  return PyDict_SetItem(pydict, ApiHandle::fromObject(*keyobj)->asPyObject(),
                        value);
}

extern "C" PyObject* PyDict_New() {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Handle<Object> value(&scope, runtime->newDict());
  return ApiHandle::fromObject(*value)->asPyObject();
}

extern "C" PyObject* PyDict_GetItem(PyObject* pydict, PyObject* key) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Handle<Object> dictobj(&scope, ApiHandle::fromPyObject(pydict)->asObject());
  if (!dictobj->isDict()) {
    return nullptr;
  }
  Handle<Dict> dict(&scope, *dictobj);
  Handle<Object> key_obj(&scope, ApiHandle::fromPyObject(key)->asObject());
  Object* value = runtime->dictAt(dict, key_obj);
  if (value->isError()) {
    return nullptr;
  }
  return ApiHandle::fromBorrowedObject(value)->asPyObject();
}

extern "C" void PyDict_Clear(PyObject* /* p */) {
  UNIMPLEMENTED("PyDict_Clear");
}

extern "C" int PyDict_Contains(PyObject* /* p */, PyObject* /* y */) {
  UNIMPLEMENTED("PyDict_Contains");
}

extern "C" PyObject* PyDict_Copy(PyObject* /* o */) {
  UNIMPLEMENTED("PyDict_Copy");
}

extern "C" int PyDict_DelItem(PyObject* /* p */, PyObject* /* y */) {
  UNIMPLEMENTED("PyDict_DelItem");
}

extern "C" Py_ssize_t PyDict_Size(PyObject* /* p */) {
  UNIMPLEMENTED("PyDict_Size");
}

extern "C" int PyDict_DelItemString(PyObject* /* v */, const char* /* y */) {
  UNIMPLEMENTED("PyDict_DelItemString");
}

extern "C" PyObject* PyDict_GetItemString(PyObject* /* v */,
                                          const char* /* y */) {
  UNIMPLEMENTED("PyDict_GetItemString");
}

extern "C" PyObject* PyDict_GetItemWithError(PyObject* /* p */,
                                             PyObject* /* y */) {
  UNIMPLEMENTED("PyDict_GetItemWithError");
}

extern "C" PyObject* PyDict_Items(PyObject* /* p */) {
  UNIMPLEMENTED("PyDict_Items");
}

extern "C" PyObject* PyDict_Keys(PyObject* /* p */) {
  UNIMPLEMENTED("PyDict_Keys");
}

extern "C" int PyDict_Merge(PyObject* /* a */, PyObject* /* b */, int /* e */) {
  UNIMPLEMENTED("PyDict_Merge");
}

extern "C" int PyDict_MergeFromSeq2(PyObject* /* d */, PyObject* /* 2 */,
                                    int /* e */) {
  UNIMPLEMENTED("PyDict_MergeFromSeq2");
}

extern "C" int PyDict_Next(PyObject* /* p */, Py_ssize_t* /* s */,
                           PyObject** /* pkey */, PyObject** /* pvalue */) {
  UNIMPLEMENTED("PyDict_Next");
}

extern "C" int PyDict_Update(PyObject* /* a */, PyObject* /* b */) {
  UNIMPLEMENTED("PyDict_Update");
}

extern "C" PyObject* PyDict_Values(PyObject* /* p */) {
  UNIMPLEMENTED("PyDict_Values");
}

extern "C" PyObject* PyObject_GenericGetDict(PyObject* /* j */, void* /* t */) {
  UNIMPLEMENTED("PyObject_GenericGetDict");
}

}  // namespace python
