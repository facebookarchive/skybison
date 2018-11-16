// tupleobject.c implementation

#include "Python.h"

#include "runtime/objects.h"
#include "runtime/runtime.h"

namespace python {

void PyTuple_Type_Init(void) {
  PyTypeObject* pytuple_type =
      static_cast<PyTypeObject*>(std::malloc(sizeof(PyTypeObject)));
  *pytuple_type = {
      PyVarObject_HEAD_INIT(&PyType_Type, 0) "tuple",  // tp_name
      0,                                               // tp_basicsize
      0,                                               // tp_itemsize
      0,                                               // tp_dealloc
      0,                                               // tp_print
      0,                                               // tp_getattr
      0,                                               // tp_setattr
      0,                                               // tp_reserved
      0,                                               // tp_repr
      0,                                               // tp_as_number
      0,                                               // tp_as_sequence
      0,                                               // tp_as_mapping
      0,                                               // tp_hash
      0,                                               // tp_call
      0,                                               // tp_str
      0,                                               // tp_getattro
      0,                                               // tp_setattro
      0,                                               // tp_as_buffer
      Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_BASETYPE |
          Py_TPFLAGS_TUPLE_SUBCLASS | Py_TPFLAGS_BUILTIN,  // tp_flags
      0,                                                   // tp_doc
      0,                                                   // tp_traverse
      0,                                                   // tp_clear
      0,                                                   // tp_richcompare
      0,                                                   // tp_weaklistoffset
      0,                                                   // tp_iter
      0,                                                   // tp_iternext
      0,                                                   // tp_methods
      0,                                                   // tp_members
      0,                                                   // tp_getset
      0,                                                   // tp_base
      0,                                                   // tp_dict
      0,                                                   // tp_descr_get
      0,                                                   // tp_descr_set
      0,                                                   // tp_dictoffset
      0,                                                   // tp_init
      0,                                                   // tp_alloc
      0,                                                   // tp_new
      0,                                                   // tp_free
      0,                                                   // tp_is_gc
      0,                                                   // tp_bases
      0,                                                   // tp_mro
      0,                                                   // tp_cache
      0,                                                   // tp_subclasses
      0,                                                   // tp_weaklist
      0,                                                   // tp_del
      0,                                                   // tp_version_tag
      0,                                                   // tp_finalize
  };

  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  runtime->addBuiltinExtensionType(pytuple_type);
}

extern "C" PyTypeObject* PyTuple_Type_Ptr() {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  return static_cast<PyTypeObject*>(
      runtime->builtinExtensionTypes(static_cast<int>(ExtensionTypes::kTuple)));
}

extern "C" PyObject* PyTuple_New(Py_ssize_t length) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Handle<ObjectArray> tuple(&scope, runtime->newObjectArray(length));
  return ApiHandle::fromObject(*tuple)->asPyObject();
}

extern "C" Py_ssize_t PyTuple_Size(PyObject* pytuple) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);

  Handle<Object> tupleobj(&scope, ApiHandle::fromPyObject(pytuple)->asObject());
  if (!tupleobj->isObjectArray()) {
    return -1;
  }

  Handle<ObjectArray> tuple(&scope, *tupleobj);
  return tuple->length();
}

extern "C" PyObject* PyTuple_GetItem(PyObject* pytuple, Py_ssize_t pos) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);

  Handle<Object> tupleobj(&scope, ApiHandle::fromPyObject(pytuple)->asObject());
  if (!tupleobj->isObjectArray()) {
    return nullptr;
  }

  Handle<ObjectArray> tuple(&scope, *tupleobj);
  if (pos < 0 || pos >= tuple->length()) {
    return nullptr;
  }

  return ApiHandle::fromBorrowedObject(tuple->at(pos))->asPyObject();
}

extern "C" PyObject* PyTuple_Pack(Py_ssize_t n, ...) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  PyObject* item;
  va_list vargs;
  va_start(vargs, n);
  Handle<ObjectArray> tuple(&scope, runtime->newObjectArray(n));
  for (Py_ssize_t i = 0; i < n; i++) {
    item = va_arg(vargs, PyObject*);
    tuple->atPut(i, ApiHandle::fromPyObject(item)->asObject());
  }
  va_end(vargs);
  return ApiHandle::fromObject(*tuple)->asPyObject();
}

}  // namespace python
