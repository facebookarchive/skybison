// descrobject.c implementation

#include "Python.h"
#include "structmember.h"

#include "runtime/runtime.h"

namespace python {

void PyMethodDescr_Type_Init(void) {
  PyTypeObject* pymethoddescr_type =
      static_cast<PyTypeObject*>(std::malloc(sizeof(PyTypeObject)));
  *pymethoddescr_type = {
      PyVarObject_HEAD_INIT(&PyType_Type, 0) "method_descriptor",
      sizeof(PyMethodDescrObject),
      0, // tp_itemsize
      0, // tp_dealloc
      0, // tp_print
      0, // tp_getattr
      0, // tp_setattr
      0, // tp_reserved
      0, // tp_repr
      0, // tp_as_number
      0, // tp_as_sequence
      0, // tp_as_mapping
      0, // tp_hash
      0, // tp_call
      0, // tp_str
      0, // tp_getattro
      0, // tp_setattro
      0, // tp_as_buffer
      Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC, // tp_flags
      0, // tp_doc
      0, // tp_traverse
      0, // tp_clear
      0, // tp_richcompare
      0, // tp_weaklistoffset
      0, // tp_iter
      0, // tp_iternext
      0, // tp_methods
      0, // tp_members
      0, // tp_getset
      0, // tp_base
      0, // tp_dict
      0, // tp_descr_get
      0, // tp_descr_set
      0, // tp_dictoffset
      0, // tp_init
      0, // tp_alloc
      0, // tp_new
      0, // tp_free
      0, // tp_is_gc
      0, // tp_bases
      0, // tp_mro
      0, // tp_cache
      0, // tp_subclasses
      0, // tp_weaklist
      0, // tp_del
      0, // tp_version_tag
      0, // tp_finalize
  };

  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  runtime->addBuiltinExtensionType(pymethoddescr_type);
}

extern "C" PyTypeObject* PyMethodDescr_Type_Ptr() {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  PyTypeObject* pymethoddescr_type =
      static_cast<PyTypeObject*>(runtime->builtinExtensionTypes(
          static_cast<int>(ExtensionTypes::kMethodDescr)));
  PyType_Ready(pymethoddescr_type);
  return pymethoddescr_type;
}

void PyMemberDescr_Type_Init(void) {
  PyTypeObject* pymemberdescr_type =
      static_cast<PyTypeObject*>(std::malloc(sizeof(PyTypeObject)));
  *pymemberdescr_type = {
      PyVarObject_HEAD_INIT(&PyType_Type, 0) "member_descriptor",
      sizeof(PyMemberDescrObject),
      0, // tp_itemsize
      0, // tp_dealloc
      0, // tp_print
      0, // tp_getattr
      0, // tp_setattr
      0, // tp_reserved
      0, // tp_repr
      0, // tp_as_number
      0, // tp_as_sequence
      0, // tp_as_mapping
      0, // tp_hash
      0, // tp_call
      0, // tp_str
      0, // tp_getattro
      0, // tp_setattro
      0, // tp_as_buffer
      Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC, // tp_flags
      0, // tp_doc
      0, // tp_traverse
      0, // tp_clear
      0, // tp_richcompare
      0, // tp_weaklistoffset
      0, // tp_iter
      0, // tp_iternext
      0, // tp_methods
      0, // tp_members
      0, // tp_getset
      0, // tp_base
      0, // tp_dict
      0, // tp_descr_get
      0, // tp_descr_set
      0, // tp_dictoffset
      0, // tp_init
      0, // tp_alloc
      0, // tp_new
      0, // tp_free
      0, // tp_is_gc
      0, // tp_bases
      0, // tp_mro
      0, // tp_cache
      0, // tp_subclasses
      0, // tp_weaklist
      0, // tp_del
      0, // tp_version_tag
      0, // tp_finalize
  };

  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  runtime->addBuiltinExtensionType(pymemberdescr_type);
}

extern "C" PyTypeObject* PyMemberDescr_Type_Ptr() {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  PyTypeObject* pymemberdescr_type =
      static_cast<PyTypeObject*>(runtime->builtinExtensionTypes(
          static_cast<int>(ExtensionTypes::kMemberDescr)));
  PyType_Ready(pymemberdescr_type);
  return pymemberdescr_type;
}

void PyGetSetDescr_Type_Init(void) {
  PyTypeObject* pygetsetdescr_type =
      static_cast<PyTypeObject*>(std::malloc(sizeof(PyTypeObject)));
  *pygetsetdescr_type = {
      PyVarObject_HEAD_INIT(&PyType_Type, 0) "getset_descriptor",
      sizeof(PyGetSetDescrObject),
      0, // tp_itemsize
      0, // tp_dealloc
      0, // tp_print
      0, // tp_getattr
      0, // tp_setattr
      0, // tp_reserved
      0, // tp_repr
      0, // tp_as_number
      0, // tp_as_sequence
      0, // tp_as_mapping
      0, // tp_hash
      0, // tp_call
      0, // tp_str
      0, // tp_getattro
      0, // tp_setattro
      0, // tp_as_buffer
      Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC, // tp_flags
      0, // tp_doc
      0, // tp_traverse
      0, // tp_clear
      0, // tp_richcompare
      0, // tp_weaklistoffset
      0, // tp_iter
      0, // tp_iternext
      0, // tp_methods
      0, // tp_members
      0, // tp_getset
      0, // tp_base
      0, // tp_dict
      0, // tp_descr_get
      0, // tp_descr_set
      0, // tp_dictoffset
      0, // tp_init
      0, // tp_alloc
      0, // tp_new
      0, // tp_free
      0, // tp_is_gc
      0, // tp_bases
      0, // tp_mro
      0, // tp_cache
      0, // tp_subclasses
      0, // tp_weaklist
      0, // tp_del
      0, // tp_version_tag
      0, // tp_finalize
  };

  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  runtime->addBuiltinExtensionType(pygetsetdescr_type);
}

extern "C" PyTypeObject* PyGetSetDescr_Type_Ptr() {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  PyTypeObject* pygetsetdescr_type =
      static_cast<PyTypeObject*>(runtime->builtinExtensionTypes(
          static_cast<int>(ExtensionTypes::kGetSetDescr)));
  PyType_Ready(pygetsetdescr_type);
  return pygetsetdescr_type;
}

} // namespace python
