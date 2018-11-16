// object.c implementation

#include "Python.h"

#include "runtime.h"

namespace python {

void PyNone_Type_Init(void) {
  PyTypeObject* pynone_type =
      static_cast<PyTypeObject*>(std::malloc(sizeof(PyTypeObject)));
  *pynone_type = {
      PyVarObject_HEAD_INIT(&PyType_Type, 0) "NoneType",  // tp_name
      0,                                                  // tp_basicsize
      0,                                                  // tp_itemsize
      0,                                                  // tp_dealloc
      0,                                                  // tp_print
      0,                                                  // tp_getattr
      0,                                                  // tp_setattr
      0,                                                  // tp_reserved
      0,                                                  // tp_repr
      0,                                                  // tp_as_number
      0,                                                  // tp_as_sequence
      0,                                                  // tp_as_mapping
      0,                                                  // tp_hash
      0,                                                  // tp_call
      0,                                                  // tp_str
      0,                                                  // tp_getattro
      0,                                                  // tp_setattro
      0,                                                  // tp_as_buffer
      Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BUILTIN,            // tp_flags
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
  runtime->addBuiltinExtensionType(pynone_type);
}

extern "C" PyTypeObject* PyNone_Type_Ptr() {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  return static_cast<PyTypeObject*>(
      runtime->builtinExtensionTypes(ExtensionTypes::kNone));
}

extern "C" PyObject* PyNone_Ptr() {
  return ApiHandle::fromObject(None::object())->asPyObject();
}

extern "C" void _Py_Dealloc(PyObject* op) {
  // TODO(T30365701): Add a deallocation strategy for ApiHandles
  if (Py_TYPE(op) && Py_TYPE(op)->tp_dealloc) {
    destructor dealloc = Py_TYPE(op)->tp_dealloc;
    _Py_ForgetReference(op);
    (*dealloc)(op);
  }
}

}  // namespace python
