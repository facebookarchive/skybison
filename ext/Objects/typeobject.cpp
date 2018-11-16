// typeobject.c implementation

#include "handles.h"
#include "mro.h"
#include "objects.h"
#include "runtime.h"
#include "trampolines.h"
#include "utils.h"

namespace python {

extern "C" int PyType_CheckExact_Func(PyObject* obj) {
  PyObject* type = reinterpret_cast<PyObject*>(obj->ob_type);
  return ApiHandle::fromPyObject(type)->asObject()->isType();
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
      ApiHandle::fromObject(runtime->typeAt(LayoutId::kType))->asPyObject();
  pyobj->ob_type = reinterpret_cast<PyTypeObject*>(pytype_type);

  // Create a new class for the PyTypeObject
  Handle<Type> type_class(&scope, runtime->newClass());
  Handle<Dict> dict(&scope, runtime->newDict());
  type_class->setDict(*dict);

  // Add the PyTypeObject pointer to the class
  type_class->setExtensionType(runtime->newIntFromCPointer(type));

  // Create the dict's ApiHandle
  type->tp_dict = ApiHandle::fromObject(*dict)->asPyObject();

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

}  // namespace python
