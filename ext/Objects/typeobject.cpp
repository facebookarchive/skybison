// typeobject.c implementation

#include "cpython-data.h"
#include "cpython-func.h"
#include "cpython-types.h"

#include "handles.h"
#include "mro.h"
#include "objects.h"
#include "runtime.h"
#include "trampolines.h"
#include "utils.h"

namespace python {

PY_EXPORT int PyType_CheckExact_Func(PyObject* obj) {
  return ApiHandle::fromPyObject(obj)->asObject()->isType();
}

PY_EXPORT int PyType_Check_Func(PyObject* obj) {
  if (PyType_CheckExact_Func(obj)) {
    return true;
  }
  return ApiHandle::fromPyObject(obj)->isSubclass(Thread::currentThread(),
                                                  LayoutId::kType);
}

static RawObject extensionSlot(const Type& type, Type::ExtensionSlot slot_id) {
  DCHECK(!type->extensionSlots()->isNoneType(), "Type is not an extension");
  return RawTuple::cast(type->extensionSlots())->at(static_cast<word>(slot_id));
}

static void setExtensionSlot(const Type& type, Type::ExtensionSlot slot_id,
                             RawObject slot) {
  DCHECK(!type->extensionSlots()->isNoneType(), "Type is not an extension");
  return RawTuple::cast(type->extensionSlots())
      ->atPut(static_cast<word>(slot_id), slot);
}

PY_EXPORT unsigned long PyType_GetFlags(PyTypeObject* type_obj) {
  ApiHandle* handle =
      ApiHandle::fromPyObject(reinterpret_cast<PyObject*>(type_obj));
  CHECK(handle->isManaged(),
        "Type is unmanaged. Please initialize using PyType_FromSpec");

  HandleScope scope;
  Type type(&scope, handle->asObject());
  if (type->isBuiltin()) {
    UNIMPLEMENTED("GetFlags from built-in types");
  }

  if (type->extensionSlots()->isNoneType()) {
    UNIMPLEMENTED("GetFlags from types initialized through Python code");
  }

  Int flags(&scope, extensionSlot(type, Type::ExtensionSlot::kFlags));
  return flags->asWord();
}

static Type::ExtensionSlot slotToTypeSlot(int slot) {
  // TODO(eelizondo): this should cover all of the slots but,
  // we are starting with just these few for now
  switch (slot) {
    case Py_tp_alloc:
      return Type::ExtensionSlot::kAlloc;
    case Py_tp_dealloc:
      return Type::ExtensionSlot::kDealloc;
    case Py_tp_init:
      return Type::ExtensionSlot::kInit;
    case Py_tp_new:
      return Type::ExtensionSlot::kNew;
    case Py_tp_free:
      return Type::ExtensionSlot::kFree;
    default:
      return Type::ExtensionSlot::kEnd;
  }
}

PY_EXPORT void* PyType_GetSlot(PyTypeObject* type_obj, int slot) {
  Thread* thread = Thread::currentThread();
  if (slot < 0) {
    thread->raiseBadInternalCall();
    return nullptr;
  }

  ApiHandle* handle =
      ApiHandle::fromPyObject(reinterpret_cast<PyObject*>(type_obj));
  if (!handle->isManaged()) {
    thread->raiseBadInternalCall();
    return nullptr;
  }

  HandleScope scope(thread);
  Type type(&scope, handle->asObject());
  if (type->isBuiltin()) {
    thread->raiseBadInternalCall();
    return nullptr;
  }

  // Extension module requesting slot from a future version
  Type::ExtensionSlot field_id = slotToTypeSlot(slot);
  if (field_id >= Type::ExtensionSlot::kEnd) {
    return nullptr;
  }

  if (type->extensionSlots()->isNoneType()) {
    UNIMPLEMENTED("Get slots from types initialized through Python code");
  }

  DCHECK(!type->extensionSlots()->isNoneType(), "Type is not extension type");
  Int address(&scope, extensionSlot(type, field_id));
  return address->asCPtr();
}

PY_EXPORT int PyType_Ready(PyTypeObject*) {
  UNIMPLEMENTED("This funciton will never be implemented");
}

PY_EXPORT PyObject* PyType_FromSpec(PyType_Spec* spec) {
  return PyType_FromSpecWithBases(spec, nullptr);
}

PY_EXPORT PyObject* PyType_FromSpecWithBases(PyType_Spec* spec,
                                             PyObject* /* bases */) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  // Create a new type for the PyTypeObject
  Type type(&scope, runtime->newType());
  Dict dict(&scope, runtime->newDict());
  type->setDict(*dict);

  // Set the class name
  const char* class_name = strrchr(spec->name, '.');
  if (class_name == nullptr) {
    class_name = spec->name;
  } else {
    class_name++;
  }
  Object name_obj(&scope, runtime->newStrFromCStr(class_name));
  type->setName(*name_obj);
  Object dict_key(&scope, runtime->symbols()->DunderName());
  runtime->dictAtPutInValueCell(dict, dict_key, name_obj);

  // Compute Mro
  Tuple parents(&scope, runtime->newTuple(0));
  Object mro(&scope, computeMro(thread, type, parents));
  type->setMro(*mro);

  // Initialize instance Layout
  Layout layout_init(
      &scope, runtime->computeInitialLayout(thread, type, LayoutId::kObject));
  Object attr_name(&scope, runtime->symbols()->ExtensionPtr());
  Layout layout(&scope,
                runtime->layoutAddAttribute(thread, layout_init, attr_name, 0));
  layout->setDescribedType(*type);
  type->setInstanceLayout(*layout);

  // Initialize the extension slots tuple
  Object extension_slots(
      &scope, runtime->newTuple(static_cast<int>(Type::ExtensionSlot::kEnd)));
  type->setExtensionSlots(*extension_slots);

  // Set the type slots
  for (PyType_Slot* slot = spec->slots; slot->slot; slot++) {
    void* slot_ptr = bit_cast<void*>(slot->pfunc);
    Object field(&scope, runtime->newIntFromCPtr(slot_ptr));
    Type::ExtensionSlot field_id = slotToTypeSlot(slot->slot);
    if (slot->slot < 0 || field_id >= Type::ExtensionSlot::kEnd) {
      thread->raiseRuntimeErrorWithCStr("invalid slot offset");
      return nullptr;
    }
    setExtensionSlot(type, field_id, *field);

    switch (slot->slot) {
      case Py_tp_new:
        runtime->classAddExtensionFunction(type, SymbolId::kDunderNew,
                                           slot_ptr);
        break;
      case Py_tp_init:
        runtime->classAddExtensionFunction(type, SymbolId::kDunderInit,
                                           slot_ptr);
        break;
    }
  }

  // Set size
  Object basic_size(&scope, runtime->newInt(spec->basicsize));
  Object item_size(&scope, runtime->newInt(spec->itemsize));
  setExtensionSlot(type, Type::ExtensionSlot::kBasicSize, *basic_size);
  setExtensionSlot(type, Type::ExtensionSlot::kItemSize, *item_size);

  // Set the class flags
  Object flags(&scope, runtime->newInt(spec->flags | Py_TPFLAGS_READY));
  setExtensionSlot(type, Type::ExtensionSlot::kFlags, *flags);

  return ApiHandle::newReference(thread, *type);
}

PY_EXPORT PyObject* PyType_GenericAlloc(PyTypeObject* type_obj,
                                        Py_ssize_t nitems) {
  ApiHandle* handle =
      ApiHandle::fromPyObject(reinterpret_cast<PyObject*>(type_obj));
  DCHECK(handle->isManaged(),
         "Type is unmanaged. Please initialize using PyType_FromSpec");
  HandleScope scope;
  Type type(&scope, handle->asObject());
  DCHECK(!type->isBuiltin(),
         "Type is unmanaged. Please initialize using PyType_FromSpec");
  DCHECK(!type->extensionSlots()->isNoneType(),
         "GenericAlloc from types initialized through Python code");
  Int basic_size(&scope, extensionSlot(type, Type::ExtensionSlot::kBasicSize));
  Int item_size(&scope, extensionSlot(type, Type::ExtensionSlot::kItemSize));
  Py_ssize_t size = Utils::roundUp(
      nitems * item_size->asWord() + basic_size->asWord(), kWordSize);

  PyObject* pyobj = static_cast<PyObject*>(PyObject_Calloc(1, size));
  if (pyobj == nullptr) return nullptr;
  pyobj->ob_refcnt = 1;
  pyobj->ob_type = type_obj;
  if (item_size->asWord() != 0) {
    reinterpret_cast<PyVarObject*>(pyobj)->ob_size = nitems;
  }
  return pyobj;
}

PY_EXPORT unsigned int PyType_ClearCache() {
  UNIMPLEMENTED("PyType_ClearCache");
}

PY_EXPORT PyObject* PyType_GenericNew(PyTypeObject* /* e */, PyObject* /* s */,
                                      PyObject* /* s */) {
  UNIMPLEMENTED("PyType_GenericNew");
}

PY_EXPORT int PyType_IsSubtype(PyTypeObject* a, PyTypeObject* b) {
  if (a == b) return 1;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Type a_obj(
      &scope,
      ApiHandle::fromPyObject(reinterpret_cast<PyObject*>(a))->asObject());
  Type b_obj(
      &scope,
      ApiHandle::fromPyObject(reinterpret_cast<PyObject*>(b))->asObject());
  return thread->runtime()->isSubclass(a_obj, b_obj) ? 1 : 0;
}

PY_EXPORT void PyType_Modified(PyTypeObject* /* e */) {
  UNIMPLEMENTED("PyType_Modified");
}

PY_EXPORT PyObject* _PyObject_LookupSpecial(PyObject* /* f */,
                                            _Py_Identifier* /* d */) {
  UNIMPLEMENTED("_PyObject_LookupSpecial");
}

}  // namespace python
