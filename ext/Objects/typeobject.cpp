// typeobject.c implementation

#include "cpython-func.h"
#include "cpython-types.h"

#include "handles.h"
#include "mro.h"
#include "objects.h"
#include "runtime.h"
#include "trampolines.h"
#include "utils.h"

namespace python {

struct PyType_Spec;
struct _Py_Identifier;

PY_EXPORT int PyType_CheckExact_Func(PyObject* obj) {
  return ApiHandle::fromPyObject(obj)->asObject()->isType();
}

PY_EXPORT int PyType_Check_Func(PyObject* obj) {
  if (PyType_CheckExact_Func(obj)) {
    return true;
  }
  return ApiHandle::fromPyObject(obj)->isSubClass(Thread::currentThread(),
                                                  LayoutId::kType);
}

PY_EXPORT unsigned long PyType_GetFlags(PyTypeObject* type) {
  return type->tp_flags;
}

PY_EXPORT int PyType_Ready(PyTypeObject* type) {
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
  Type type_class(&scope, runtime->newType());
  Dict dict(&scope, runtime->newDict());
  type_class->setDict(*dict);

  // Add the PyTypeObject pointer to the class
  type_class->setExtensionType(runtime->newIntFromCPtr(type));

  // Create the dict's ApiHandle
  type->tp_dict = ApiHandle::fromObject(*dict);

  // Set the class name
  Object name(&scope, runtime->newStrFromCStr(type->tp_name));
  type_class->setName(*name);
  Object dict_key(&scope, runtime->symbols()->DunderName());
  runtime->dictAtPutInValueCell(dict, dict_key, name);

  // Compute Mro
  Tuple parents(&scope, runtime->newTuple(0));
  Object mro(&scope, computeMro(thread, type_class, parents));
  type_class->setMro(*mro);

  // Initialize instance Layout
  Layout layout_init(&scope, runtime->computeInitialLayout(thread, type_class,
                                                           LayoutId::kObject));
  Object attr_name(&scope, runtime->symbols()->ExtensionPtr());
  Layout layout(&scope,
                runtime->layoutAddAttribute(thread, layout_init, attr_name, 0));
  layout->setDescribedType(*type_class);
  type_class->setInstanceLayout(*layout);

  // Register DunderNew
  runtime->classAddExtensionFunction(type_class, SymbolId::kDunderNew,
                                     bit_cast<void*>(type->tp_new));

  // Register DunderInit
  runtime->classAddExtensionFunction(type_class, SymbolId::kDunderInit,
                                     bit_cast<void*>(type->tp_init));

  // TODO(T29618332): Implement missing PyType_Ready features

  // Add the runtime class object reference to PyTypeObject
  // TODO(eelizondo): Handled this automatically in PyType_FromSpec
  pyobj->reference_ = reinterpret_cast<void*>(type_class->raw());

  // All done -- set the ready flag
  type->tp_flags = (type->tp_flags & ~Py_TPFLAGS_READYING) | Py_TPFLAGS_READY;
  return 0;
}

PY_EXPORT unsigned int PyType_ClearCache() {
  UNIMPLEMENTED("PyType_ClearCache");
}

PY_EXPORT PyObject* PyType_FromSpec(PyType_Spec* /* c */) {
  UNIMPLEMENTED("PyType_FromSpec");
}

PY_EXPORT PyObject* PyType_FromSpecWithBases(PyType_Spec* /* c */,
                                             PyObject* /* s */) {
  UNIMPLEMENTED("PyType_FromSpecWithBases");
}

PY_EXPORT PyObject* PyType_GenericAlloc(PyTypeObject* type, Py_ssize_t nitems) {
  size_t size = Utils::roundUp(
      type->tp_basicsize + ((nitems + 1) * type->tp_itemsize), kWordSize);
  PyObject* obj = static_cast<PyObject*>(PyObject_Calloc(1, size));
  if (obj == nullptr) {
    return nullptr;
  }
  obj->ob_refcnt = 1;
  obj->ob_type = type;
  if (type->tp_itemsize == 0) {
    reinterpret_cast<PyVarObject*>(obj)->ob_size = nitems;
  }
  return obj;
}

PY_EXPORT PyObject* PyType_GenericNew(PyTypeObject* /* e */, PyObject* /* s */,
                                      PyObject* /* s */) {
  UNIMPLEMENTED("PyType_GenericNew");
}

PY_EXPORT void* PyType_GetSlot(PyTypeObject* /* e */, int /* t */) {
  UNIMPLEMENTED("PyType_GetSlot");
}

PY_EXPORT int PyType_IsSubtype(PyTypeObject* /* a */, PyTypeObject* /* b */) {
  UNIMPLEMENTED("PyType_IsSubtype");
}

PY_EXPORT void PyType_Modified(PyTypeObject* /* e */) {
  UNIMPLEMENTED("PyType_Modified");
}

PY_EXPORT PyObject* _PyObject_LookupSpecial(PyObject* /* f */,
                                            _Py_Identifier* /* d */) {
  UNIMPLEMENTED("_PyObject_LookupSpecial");
}

}  // namespace python
