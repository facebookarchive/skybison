// dictobject.c implementation

#include "Python.h"

#include "runtime/handles.h"
#include "runtime/objects.h"
#include "runtime/runtime.h"

namespace python {

void PyDict_Type_Init(void) {
  PyTypeObject* pydict_type =
      static_cast<PyTypeObject*>(std::malloc(sizeof(PyTypeObject)));
  *pydict_type = {
      PyVarObject_HEAD_INIT(&PyType_Type, 0) "dict",  // tp_name
      0,                                              // tp_basicsize
      0,                                              // tp_itemsize
      0,                                              // tp_dealloc
      0,                                              // tp_print
      0,                                              // tp_getattr
      0,                                              // tp_setattr
      0,                                              // tp_reserved
      0,                                              // tp_repr
      0,                                              // tp_as_number
      0,                                              // tp_as_sequence
      0,                                              // tp_as_mapping
      0,                                              // tp_hash
      0,                                              // tp_call
      0,                                              // tp_str
      0,                                              // tp_getattro
      0,                                              // tp_setattro
      0,                                              // tp_as_buffer
      Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_BASETYPE |
          Py_TPFLAGS_DICT_SUBCLASS | Py_TPFLAGS_BUILTIN,  // tp_flags
      0,                                                  // tp_doc
      0,                                                  // tp_traverse
      0,                                                  // tp_clear
      0,                                                  // tp_richcompare
      0,                                                  // tp_weaklistoffset
      0,                                                  // tp_iter
      0,                                                  // tp_iternext
      0,                                                  // tp_methods
      0,                                                  // tp_members
      0,                                                  // tp_getset
      0,                                                  // tp_base
      0,                                                  // tp_dict
      0,                                                  // tp_descr_get
      0,                                                  // tp_descr_set
      0,                                                  // tp_dictoffset
      0,                                                  // tp_init
      0,                                                  // tp_alloc
      0,                                                  // tp_new
      0,                                                  // tp_free
      0,                                                  // tp_is_gc
      0,                                                  // tp_bases
      0,                                                  // tp_mro
      0,                                                  // tp_cache
      0,                                                  // tp_subclasses
      0,                                                  // tp_weaklist
      0,                                                  // tp_del
      0,                                                  // tp_version_tag
      0,                                                  // tp_finalize
  };

  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  runtime->addBuiltinExtensionType(pydict_type);
}

extern "C" PyTypeObject* PyDict_Type_Ptr() {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  return static_cast<PyTypeObject*>(
      runtime->builtinExtensionTypes(static_cast<int>(ExtensionTypes::kDict)));
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
