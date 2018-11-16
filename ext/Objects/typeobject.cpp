// typeobject.c implementation

#include "handles.h"
#include "mro.h"
#include "objects.h"
#include "runtime.h"
#include "trampolines.h"
#include "utils.h"

namespace python {

void PyType_Type_Init() {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();

  // PyType_Type init is handled independently as its metatype is itself
  ApiTypeHandle* pytype_type =
      static_cast<ApiTypeHandle*>(std::calloc(1, sizeof(PyTypeObject)));
  pytype_type->ob_base.ob_base.ob_type = pytype_type;
  pytype_type->ob_base.ob_base.ob_refcnt = 1;
  pytype_type->tp_name = "type";
  pytype_type->tp_flags = ApiTypeHandle::kFlagsBuiltin;

  runtime->addBuiltinTypeHandle(pytype_type);
}

extern "C" PyTypeObject* PyType_Type_Ptr() {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  return static_cast<PyTypeObject*>(
      runtime->builtinTypeHandles(ExtensionTypes::kType));
}

void PyBaseObject_Type_Init() {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();

  ApiTypeHandle* pytype_type =
      runtime->builtinTypeHandles(ExtensionTypes::kType);
  ApiTypeHandle* pybaseobject_type =
      ApiTypeHandle::newTypeHandle("object", pytype_type);

  runtime->addBuiltinTypeHandle(pybaseobject_type);
}

extern "C" PyTypeObject* PyBaseObject_Type_Ptr() {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  return static_cast<PyTypeObject*>(
      runtime->builtinTypeHandles(ExtensionTypes::kBaseObject));
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

  // Create a new class for the PyTypeObject
  Handle<Class> type_class(&scope, runtime->newClass());
  Handle<Dictionary> dictionary(&scope, runtime->newDictionary());
  type_class->setDictionary(*dictionary);

  // Add the PyTypeObject pointer to the class
  type_class->setExtensionType(runtime->newIntegerFromCPointer(type));

  // Create the dictionary's ApiHandle
  type->tp_dict = ApiHandle::fromObject(*dictionary)->asPyObject();

  // Set the class name
  Handle<Object> name(&scope, runtime->newStringFromCString(type->tp_name));
  type_class->setName(*name);
  Handle<Object> dict_key(&scope, runtime->symbols()->DunderName());
  runtime->dictionaryAtPutInValueCell(dictionary, dict_key, name);

  // Compute Mro
  Handle<ObjectArray> parents(&scope, runtime->newObjectArray(0));
  Handle<Object> mro(&scope, computeMro(thread, type_class, parents));
  type_class->setMro(*mro);

  // Initialize instance Layout
  Handle<Layout> layout_init(&scope,
                             runtime->computeInitialLayout(thread, type_class));
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
  Handle<Dictionary> extensions_dict(&scope, runtime->extensionTypes());
  Handle<Object> type_class_obj(&scope, *type_class);
  Handle<Object> type_class_id(
      &scope, runtime->newIntegerFromCPointer(static_cast<void*>(type)));
  runtime->dictionaryAtPut(extensions_dict, type_class_id, type_class_obj);

  // All done -- set the ready flag
  type->tp_flags = (type->tp_flags & ~Py_TPFLAGS_READYING) | Py_TPFLAGS_READY;
  return 0;
}

}  // namespace python
