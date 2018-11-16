// boolobject.c implementation

#include "Python.h"

#include "runtime/objects.h"
#include "runtime/runtime.h"

namespace python {

void PyBool_Type_Init(void) {
  PyTypeObject* pybool_type =
      static_cast<PyTypeObject*>(std::malloc(sizeof(PyTypeObject)));
  *pybool_type = {
      PyVarObject_HEAD_INIT(&PyType_Type, 0) "bool", // tp_name
      0, // tp_basicsize
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
      Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BUILTIN, // tp_flags
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
  runtime->addBuiltinExtensionType(pybool_type);
}

extern "C" PyTypeObject* PyBool_Type_Ptr() {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  return static_cast<PyTypeObject*>(
      runtime->builtinExtensionTypes(static_cast<int>(ExtensionTypes::kBool)));
}

extern "C" PyObject* PyTrue_Ptr() {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  return runtime->asApiHandle(Boolean::trueObj())->asPyObject();
}

extern "C" PyObject* PyFalse_Ptr() {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  return runtime->asApiHandle(Boolean::falseObj())->asPyObject();
}

// Start of automatically generated code, from Object/boolobject.c
extern "C" PyObject* PyBool_FromLong(long ok) {
  PyObject* result;

  if (ok) {
    result = Py_True;
  } else {
    result = Py_False;
  }
  Py_INCREF(result);
  return result;
}
// End of automatically generated code

} // namespace python
