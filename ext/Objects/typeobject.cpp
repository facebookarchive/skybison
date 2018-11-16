// typeobject.c implementation

#include "handles.h"
#include "mro.h"
#include "objects.h"
#include "runtime.h"
#include "trampolines.h"
#include "utils.h"

namespace python {

struct PyType_Spec;

extern "C" int PyType_CheckExact_Func(PyObject* obj) {
  return ApiHandle::fromPyObject(obj)->asObject()->isType();
}

extern "C" int PyType_Check_Func(PyObject* obj) {
  if (PyType_CheckExact_Func(obj)) {
    return true;
  }
  return ApiHandle::fromPyObject(obj)->isSubClass(Thread::currentThread(),
                                                  LayoutId::kType);
}

extern "C" unsigned long PyType_GetFlags(PyTypeObject* type) {
  return type->tp_flags;
}

extern "C" int PyType_Ready(PyTypeObject* type) {
  // Type is already initialized
  if (type->tp_flags & Py_TPFLAGS_READY) {
    return 0;
  }

  type->tp_flags |= Py_TPFLAGS_READYING;

  if (!type->tp_name) {
    UNIMPLEMENTED("PyExc_SystemError");
    type->tp_flags &= ~Py_TPFLAGS_READYING;
    return -1;
  }

  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  // Set the base type to PyType_Type
  // TODO(T32512394): Handle metaclass initialization
  PyObject* pyobj = reinterpret_cast<PyObject*>(type);
  PyObject* pytype_type =
      ApiHandle::fromObject(runtime->typeAt(LayoutId::kType));
  pyobj->ob_type = reinterpret_cast<PyTypeObject*>(pytype_type);

  // Create a new class for the PyTypeObject
  Handle<Type> type_class(&scope, runtime->newClass());
  Handle<Dict> dict(&scope, runtime->newDict());
  type_class->setDict(*dict);

  // Add the PyTypeObject pointer to the class
  type_class->setExtensionType(runtime->newIntFromCPtr(type));

  // Create the dict's ApiHandle
  type->tp_dict = ApiHandle::fromObject(*dict);

  // Set the class name
  Handle<Object> name(&scope, runtime->newStrFromCStr(type->tp_name));
  type_class->setName(*name);
  Handle<Object> dict_key(&scope, runtime->symbols()->DunderName());
  runtime->dictAtPutInValueCell(dict, dict_key, name);

  // Compute Mro
  Handle<ObjectArray> parents(&scope, runtime->newObjectArray(0));
  Handle<Object> mro(&scope, computeMro(thread, type_class, parents));
  type_class->setMro(*mro);

  // Initialize instance Layout
  Handle<Layout> layout_init(
      &scope,
      runtime->computeInitialLayout(thread, type_class, LayoutId::kObject));
  Handle<Object> attr_name(&scope, runtime->symbols()->ExtensionPtr());
  Handle<Layout> layout(
      &scope, runtime->layoutAddAttribute(thread, layout_init, attr_name, 0));
  layout->setDescribedClass(*type_class);
  type_class->setInstanceLayout(*layout);

  // Register DunderNew
  runtime->classAddExtensionFunction(type_class, SymbolId::kDunderNew,
                                     Utils::castFnPtrToVoid(type->tp_new));

  // Register DunderInit
  runtime->classAddExtensionFunction(type_class, SymbolId::kDunderInit,
                                     Utils::castFnPtrToVoid(type->tp_init));

  // TODO(T29618332): Implement missing PyType_Ready features

  // Add the runtime class object reference to PyTypeObject
  // TODO(eelizondo): Handled this automatically in PyType_FromSpec
  pyobj->reference_ = *type_class;

  // All done -- set the ready flag
  type->tp_flags = (type->tp_flags & ~Py_TPFLAGS_READYING) | Py_TPFLAGS_READY;
  return 0;
}

extern "C" unsigned int PyType_ClearCache(void) {
  UNIMPLEMENTED("PyType_ClearCache");
}

extern "C" PyObject* PyType_FromSpec(PyType_Spec* /* c */) {
  UNIMPLEMENTED("PyType_FromSpec");
}

extern "C" PyObject* PyType_FromSpecWithBases(PyType_Spec* /* c */,
                                              PyObject* /* s */) {
  UNIMPLEMENTED("PyType_FromSpecWithBases");
}

extern "C" PyObject* PyType_GenericNew(PyTypeObject* /* e */, PyObject* /* s */,
                                       PyObject* /* s */) {
  UNIMPLEMENTED("PyType_GenericNew");
}

extern "C" void* PyType_GetSlot(PyTypeObject* /* e */, int /* t */) {
  UNIMPLEMENTED("PyType_GetSlot");
}

extern "C" int PyType_IsSubtype(PyTypeObject* /* a */, PyTypeObject* /* b */) {
  UNIMPLEMENTED("PyType_IsSubtype");
}

extern "C" void PyType_Modified(PyTypeObject* /* e */) {
  UNIMPLEMENTED("PyType_Modified");
}

}  // namespace python
