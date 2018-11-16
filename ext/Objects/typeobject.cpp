// typeobject.c implementation

#include "Python.h"

#include "runtime/builtins.h"
#include "runtime/handles.h"
#include "runtime/mro.h"
#include "runtime/objects.h"
#include "runtime/runtime.h"
#include "runtime/trampolines.h"

namespace py = python;

PyTypeObject PyBaseObject_Type;
void initialize_PyBaseObject_Type() {
  PyBaseObject_Type = {
      PyVarObject_HEAD_INIT(&PyType_Type, 0) "object", /* tp_name */
      sizeof(PyObject), /* tp_basicsize */
      0, /* tp_itemsize */
      0, /* tp_dealloc */
      0, /* tp_print */
      0, /* tp_getattr */
      0, /* tp_setattr */
      0, /* tp_reserved */
      0, /* tp_repr */
      0, /* tp_as_number */
      0, /* tp_as_sequence */
      0, /* tp_as_mapping */
      0, /* tp_hash */
      0, /* tp_call */
      0, /* tp_str */
      0, /* tp_getattro */
      0, /* tp_setattro */
      0, /* tp_as_buffer */
      0, /* tp_flags */
      0, /* tp_doc */
      0, /* tp_traverse */
      0, /* tp_clear */
      0, /* tp_richcompare */
      0, /* tp_weaklistoffset */
      0, /* tp_iter */
      0, /* tp_iternext */
      0, /* tp_methods */
      0, /* tp_members */
      0, /* tp_getset */
      0, /* tp_base */
      0, /* tp_dict */
      0, /* tp_descr_get */
      0, /* tp_descr_set */
      0, /* tp_dictoffset */
      0, /* tp_init */
      0, /* tp_alloc */
      0, /* tp_new */
      0, /* tp_free */
      0, /* tp_is_gc */
      0, /* tp_bases */
      0, /* tp_mro */
      0, /* tp_cache */
      0, /* tp_subclasses */
      0, /* tp_weaklist */
      0, /* tp_del */
      0, /* tp_version_tag */
      0, /* tp_finalize */
  };
}

PyTypeObject PyType_Type;
void initialize_PyType_Type() {
  PyType_Type = {
      PyVarObject_HEAD_INIT(&PyType_Type, 0) "type", /* tp_name */
      sizeof(PyHeapTypeObject), /* tp_basicsize */
      0, /* tp_itemsize */
      0, /* tp_dealloc */
      0, /* tp_print */
      0, /* tp_getattr */
      0, /* tp_setattr */
      0, /* tp_reserved */
      0, /* tp_repr */
      0, /* tp_as_number */
      0, /* tp_as_sequence */
      0, /* tp_as_mapping */
      0, /* tp_hash */
      0, /* tp_call */
      0, /* tp_str */
      0, /* tp_getattro */
      0, /* tp_setattro */
      0, /* tp_as_buffer */
      0, /* tp_flags */
      0, /* tp_doc */
      0, /* tp_traverse */
      0, /* tp_clear */
      0, /* tp_richcompare */
      0, /* tp_weaklistoffset */
      0, /* tp_iter */
      0, /* tp_iternext */
      0, /* tp_methods */
      0, /* tp_members */
      0, /* tp_getset */
      0, /* tp_base */
      0, /* tp_dict */
      0, /* tp_descr_get */
      0, /* tp_descr_set */
      0, /* tp_dictoffset */
      0, /* tp_init */
      0, /* tp_alloc */
      0, /* tp_new */
      0, /* tp_free */
      0, /* tp_is_gc */
      0, /* tp_bases */
      0, /* tp_mro */
      0, /* tp_cache */
      0, /* tp_subclasses */
      0, /* tp_weaklist */
      0, /* tp_del */
      0, /* tp_version_tag */
      0, /* tp_finalize */
  };
}

unsigned long PyType_GetFlags(PyTypeObject* type) {
  return type->tp_flags;
}

int PyType_Ready(PyTypeObject* type) {
  // Type is already initialized
  if (type->tp_flags & Py_TPFLAGS_READY) {
    return 0;
  }

  type->tp_flags |= Py_TPFLAGS_READYING;

  if (!type->tp_name) {
    PyErr_Format(PyExc_SystemError, "Type does not define the tp_name field.");
    type->tp_flags &= ~Py_TPFLAGS_READYING;
    return -1;
  }

  py::Thread* thread = py::Thread::currentThread();
  py::Runtime* runtime = thread->runtime();
  py::HandleScope scope(thread->handles());

  // Create a new class for the PyTypeObject
  py::Handle<py::Class> type_class(&scope, runtime->newClass());
  py::Handle<py::Dictionary> dictionary(&scope, runtime->newDictionary());
  type_class->setDictionary(*dictionary);

  // Create the dictionary's ApiHandle
  type->tp_dict = runtime->asApiHandle(*dictionary)->asPyObject();

  // Set the class name
  py::Handle<py::Object> name(
      &scope, runtime->newStringFromCString(type->tp_name));
  type_class->setName(*name);
  py::Handle<py::Object> dict_key(&scope, runtime->symbols()->DunderName());
  runtime->dictionaryAtPutInValueCell(dictionary, dict_key, name);

  // Compute Mro
  py::Handle<py::ObjectArray> parents(&scope, runtime->newObjectArray(0));
  py::Handle<py::Object> mro(&scope, computeMro(thread, type_class, parents));
  type_class->setMro(*mro);

  // Initialize instance Layout
  // TODO(T29775652): The layout should contain a pointer to the real PyObject
  py::Handle<py::Layout> layout(
      &scope, runtime->computeInitialLayout(thread, type_class));
  layout->setDescribedClass(*type_class);
  type_class->setInstanceLayout(*layout);

  // Register DunderNew
  // TODO(T29775599): Make DunderNew run tp_new
  runtime->classAddBuiltinFunction(
      type_class,
      runtime->symbols()->DunderNew(),
      py::nativeTrampoline<py::builtinObjectNew>,
      py::unimplementedTrampoline,
      py::unimplementedTrampoline);

  // Register DunderInit
  // TODO(T29775466): Make DunderInit run tp_init
  runtime->classAddBuiltinFunction(
      type_class,
      runtime->symbols()->DunderInit(),
      py::nativeTrampoline<py::builtinObjectInit>,
      py::unimplementedTrampoline,
      py::unimplementedTrampoline);

  // TODO(T29618332): Implement missing PyType_Ready features

  // Add the runtime class object reference to PyTypeObject
  py::Handle<py::Dictionary> extensions_dict(&scope, runtime->extensionTypes());
  py::Handle<py::Object> type_class_obj(&scope, *type_class);
  runtime->dictionaryAtPut(extensions_dict, name, type_class_obj);

  // Add the PyTypeObject pointer to the class
  type_class->setExtensionType(runtime->newIntegerFromCPointer(type));

  // All done -- set the ready flag
  type->tp_flags = (type->tp_flags & ~Py_TPFLAGS_READYING) | Py_TPFLAGS_READY;
  return 0;
}
