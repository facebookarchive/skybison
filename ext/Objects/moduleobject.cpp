// moduleobject.c implementation

#include "runtime/handles.h"
#include "runtime/objects.h"
#include "runtime/runtime.h"

#include "Python.h"

namespace python {

void PyModule_Type_Init(void) {
  PyTypeObject* pymodule_type =
      static_cast<PyTypeObject*>(std::malloc(sizeof(PyTypeObject)));
  *pymodule_type = {
      PyVarObject_HEAD_INIT(&PyType_Type, 0) "module",  // tp_name
      0,                                                // tp_basicsize
      0,                                                // tp_itemsize
      0,                                                // tp_dealloc
      0,                                                // tp_print
      0,                                                // tp_getattr
      0,                                                // tp_setattr
      0,                                                // tp_reserved
      0,                                                // tp_repr
      0,                                                // tp_as_number
      0,                                                // tp_as_sequence
      0,                                                // tp_as_mapping
      0,                                                // tp_hash
      0,                                                // tp_call
      0,                                                // tp_str
      0,                                                // tp_getattro
      0,                                                // tp_setattro
      0,                                                // tp_as_buffer
      Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_BASETYPE |
          Py_TPFLAGS_BUILTIN,  // tp_flags
      0,                       // tp_doc
      0,                       // tp_traverse
      0,                       // tp_clear
      0,                       // tp_richcompare
      0,                       // tp_weaklistoffset
      0,                       // tp_iter
      0,                       // tp_iternext
      0,                       // tp_methods
      0,                       // tp_members
      0,                       // tp_getset
      0,                       // tp_base
      0,                       // tp_dict
      0,                       // tp_descr_get
      0,                       // tp_descr_set
      0,                       // tp_dictoffset
      0,                       // tp_init
      0,                       // tp_alloc
      0,                       // tp_new
      0,                       // tp_free
      0,                       // tp_is_gc
      0,                       // tp_bases
      0,                       // tp_mro
      0,                       // tp_cache
      0,                       // tp_subclasses
      0,                       // tp_weaklist
      0,                       // tp_del
      0,                       // tp_version_tag
      0,                       // tp_finalize
  };

  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  runtime->addBuiltinExtensionType(pymodule_type);
}

extern "C" PyTypeObject* PyModule_Type_Ptr() {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  return static_cast<PyTypeObject*>(runtime->builtinExtensionTypes(
      static_cast<int>(ExtensionTypes::kModule)));
}

extern "C" PyObject* PyModule_Create2(struct PyModuleDef* pymodule, int) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  const char* c_name = pymodule->m_name;
  Handle<Object> name(&scope, runtime->newStringFromCString(c_name));
  Handle<Module> module(&scope, runtime->newModule(name));
  runtime->addModule(module);

  // TODO: Check m_slots
  // TODO: Set md_state
  // TODO: Validate m_methods
  // TODO: Add methods
  // TODO: Add m_doc

  return runtime->asApiHandle(*module)->asPyObject();
}

extern "C" PyObject* PyModule_GetDict(PyObject* pymodule) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Handle<Module> module(&scope, ApiHandle::fromPyObject(pymodule)->asObject());
  return runtime->asApiHandle(module->dictionary())->asPyObject();
}

}  // namespace python
