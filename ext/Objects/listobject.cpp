// listobject.c implementation

#include "Python.h"

#include "handles.h"
#include "objects.h"
#include "runtime.h"

namespace python {

void PyList_Type_Init(void) {
  PyTypeObject* pylist_type =
      static_cast<PyTypeObject*>(std::malloc(sizeof(PyTypeObject)));
  *pylist_type = {
      PyVarObject_HEAD_INIT(&PyType_Type, 0) "list",  // tp_name
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
          Py_TPFLAGS_LIST_SUBCLASS | Py_TPFLAGS_BUILTIN,  // tp_flags
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
  runtime->addBuiltinExtensionType(pylist_type);
}

extern "C" PyTypeObject* PyList_Type_Ptr() {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  return static_cast<PyTypeObject*>(
      runtime->builtinExtensionTypes(ExtensionTypes::kList));
}

extern "C" PyObject* PyList_New(Py_ssize_t size) {
  if (size < 0) {
    return nullptr;
  }

  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Handle<List> list(&scope, runtime->newList());
  Handle<ObjectArray> items(&scope, runtime->newObjectArray(size));
  list->setAllocated(size);
  list->setItems(*items);

  return ApiHandle::fromObject(*list)->asPyObject();
}

}  // namespace python
