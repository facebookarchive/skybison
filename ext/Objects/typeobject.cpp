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
  return ApiHandle::fromPyObject(obj)->asObject().isType();
}

PY_EXPORT int PyType_Check_Func(PyObject* obj) {
  return Thread::currentThread()->runtime()->isInstanceOfType(
      ApiHandle::fromPyObject(obj)->asObject());
}

static RawObject extensionSlot(const Type& type, Type::ExtensionSlot slot_id) {
  DCHECK(!type.extensionSlots().isNoneType(), "Type is not an extension");
  return RawTuple::cast(type.extensionSlots()).at(static_cast<word>(slot_id));
}

static void setExtensionSlot(const Type& type, Type::ExtensionSlot slot_id,
                             RawObject slot) {
  DCHECK(!type.extensionSlots().isNoneType(), "Type is not an extension");
  return RawTuple::cast(type.extensionSlots())
      .atPut(static_cast<word>(slot_id), slot);
}

PY_EXPORT unsigned long PyType_GetFlags(PyTypeObject* type_obj) {
  ApiHandle* handle =
      ApiHandle::fromPyObject(reinterpret_cast<PyObject*>(type_obj));
  CHECK(handle->isManaged(),
        "Type is unmanaged. Please initialize using PyType_FromSpec");

  HandleScope scope;
  Type type(&scope, handle->asObject());
  if (type.isBuiltin()) {
    UNIMPLEMENTED("GetFlags from built-in types");
  }

  if (type.extensionSlots().isNoneType()) {
    UNIMPLEMENTED("GetFlags from types initialized through Python code");
  }

  Int flags(&scope, extensionSlot(type, Type::ExtensionSlot::kFlags));
  return flags.asWord();
}

static Type::ExtensionSlot slotToTypeSlot(int slot) {
  switch (slot) {
    case Py_mp_ass_subscript:
      return Type::ExtensionSlot::kMapAssSubscript;
    case Py_mp_length:
      return Type::ExtensionSlot::kMapLength;
    case Py_mp_subscript:
      return Type::ExtensionSlot::kMapSubscript;
    case Py_nb_absolute:
      return Type::ExtensionSlot::kNumberAbsolute;
    case Py_nb_add:
      return Type::ExtensionSlot::kNumberAdd;
    case Py_nb_and:
      return Type::ExtensionSlot::kNumberAnd;
    case Py_nb_bool:
      return Type::ExtensionSlot::kNumberBool;
    case Py_nb_divmod:
      return Type::ExtensionSlot::kNumberDivmod;
    case Py_nb_float:
      return Type::ExtensionSlot::kNumberFloat;
    case Py_nb_floor_divide:
      return Type::ExtensionSlot::kNumberFloorDivide;
    case Py_nb_index:
      return Type::ExtensionSlot::kNumberIndex;
    case Py_nb_inplace_add:
      return Type::ExtensionSlot::kNumberInplaceAdd;
    case Py_nb_inplace_and:
      return Type::ExtensionSlot::kNumberInplaceAnd;
    case Py_nb_inplace_floor_divide:
      return Type::ExtensionSlot::kNumberInplaceFloorDivide;
    case Py_nb_inplace_lshift:
      return Type::ExtensionSlot::kNumberInplaceLshift;
    case Py_nb_inplace_multiply:
      return Type::ExtensionSlot::kNumberInplaceMultiply;
    case Py_nb_inplace_or:
      return Type::ExtensionSlot::kNumberInplaceOr;
    case Py_nb_inplace_power:
      return Type::ExtensionSlot::kNumberInplacePower;
    case Py_nb_inplace_remainder:
      return Type::ExtensionSlot::kNumberInplaceRemainder;
    case Py_nb_inplace_rshift:
      return Type::ExtensionSlot::kNumberInplaceRshift;
    case Py_nb_inplace_subtract:
      return Type::ExtensionSlot::kNumberInplaceSubtract;
    case Py_nb_inplace_true_divide:
      return Type::ExtensionSlot::kNumberInplaceTrueDivide;
    case Py_nb_inplace_xor:
      return Type::ExtensionSlot::kNumberInplaceXor;
    case Py_nb_int:
      return Type::ExtensionSlot::kNumberInt;
    case Py_nb_invert:
      return Type::ExtensionSlot::kNumberInvert;
    case Py_nb_lshift:
      return Type::ExtensionSlot::kNumberLshift;
    case Py_nb_multiply:
      return Type::ExtensionSlot::kNumberMultiply;
    case Py_nb_negative:
      return Type::ExtensionSlot::kNumberNegative;
    case Py_nb_or:
      return Type::ExtensionSlot::kNumberOr;
    case Py_nb_positive:
      return Type::ExtensionSlot::kNumberPositive;
    case Py_nb_power:
      return Type::ExtensionSlot::kNumberPower;
    case Py_nb_remainder:
      return Type::ExtensionSlot::kNumberRemainder;
    case Py_nb_rshift:
      return Type::ExtensionSlot::kNumberRshift;
    case Py_nb_subtract:
      return Type::ExtensionSlot::kNumberSubtract;
    case Py_nb_true_divide:
      return Type::ExtensionSlot::kNumberTrueDivide;
    case Py_nb_xor:
      return Type::ExtensionSlot::kNumberXor;
    case Py_sq_ass_item:
      return Type::ExtensionSlot::kSequenceAssItem;
    case Py_sq_concat:
      return Type::ExtensionSlot::kSequenceConcat;
    case Py_sq_contains:
      return Type::ExtensionSlot::kSequenceContains;
    case Py_sq_inplace_concat:
      return Type::ExtensionSlot::kSequenceInplaceConcat;
    case Py_sq_inplace_repeat:
      return Type::ExtensionSlot::kSequenceInplaceRepeat;
    case Py_sq_item:
      return Type::ExtensionSlot::kSequenceItem;
    case Py_sq_length:
      return Type::ExtensionSlot::kSequenceLength;
    case Py_sq_repeat:
      return Type::ExtensionSlot::kSequenceRepeat;
    case Py_tp_alloc:
      return Type::ExtensionSlot::kAlloc;
    case Py_tp_base:
      return Type::ExtensionSlot::kBase;
    case Py_tp_bases:
      return Type::ExtensionSlot::kBases;
    case Py_tp_call:
      return Type::ExtensionSlot::kCall;
    case Py_tp_clear:
      return Type::ExtensionSlot::kClear;
    case Py_tp_dealloc:
      return Type::ExtensionSlot::kDealloc;
    case Py_tp_del:
      return Type::ExtensionSlot::kDel;
    case Py_tp_descr_get:
      return Type::ExtensionSlot::kDescrGet;
    case Py_tp_descr_set:
      return Type::ExtensionSlot::kDescrSet;
    case Py_tp_doc:
      return Type::ExtensionSlot::kDoc;
    case Py_tp_getattr:
      return Type::ExtensionSlot::kGetattr;
    case Py_tp_getattro:
      return Type::ExtensionSlot::kGetattro;
    case Py_tp_hash:
      return Type::ExtensionSlot::kHash;
    case Py_tp_init:
      return Type::ExtensionSlot::kInit;
    case Py_tp_is_gc:
      return Type::ExtensionSlot::kIsGc;
    case Py_tp_iter:
      return Type::ExtensionSlot::kIter;
    case Py_tp_iternext:
      return Type::ExtensionSlot::kIternext;
    case Py_tp_methods:
      return Type::ExtensionSlot::kMethods;
    case Py_tp_new:
      return Type::ExtensionSlot::kNew;
    case Py_tp_repr:
      return Type::ExtensionSlot::kRepr;
    case Py_tp_richcompare:
      return Type::ExtensionSlot::kRichcokMapare;
    case Py_tp_setattr:
      return Type::ExtensionSlot::kSetattr;
    case Py_tp_setattro:
      return Type::ExtensionSlot::kSetattro;
    case Py_tp_str:
      return Type::ExtensionSlot::kStr;
    case Py_tp_traverse:
      return Type::ExtensionSlot::kTraverse;
    case Py_tp_members:
      return Type::ExtensionSlot::kMembers;
    case Py_tp_getset:
      return Type::ExtensionSlot::kGetset;
    case Py_tp_free:
      return Type::ExtensionSlot::kFree;
    case Py_nb_matrix_multiply:
      return Type::ExtensionSlot::kNumberMatrixMultiply;
    case Py_nb_inplace_matrix_multiply:
      return Type::ExtensionSlot::kNumberInplaceMatrixMultiply;
    case Py_am_await:
      return Type::ExtensionSlot::kAsyncAwait;
    case Py_am_aiter:
      return Type::ExtensionSlot::kAsyncAiter;
    case Py_am_anext:
      return Type::ExtensionSlot::kAsyncAnext;
    case Py_tp_finalize:
      return Type::ExtensionSlot::kFinalize;
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
  if (type.isBuiltin()) {
    thread->raiseBadInternalCall();
    return nullptr;
  }

  // Extension module requesting slot from a future version
  Type::ExtensionSlot field_id = slotToTypeSlot(slot);
  if (field_id >= Type::ExtensionSlot::kEnd) {
    return nullptr;
  }

  if (type.extensionSlots().isNoneType()) {
    UNIMPLEMENTED("Get slots from types initialized through Python code");
  }

  DCHECK(!type.extensionSlots().isNoneType(), "Type is not extension type");
  Int address(&scope, extensionSlot(type, field_id));
  return address.asCPtr();
}

PY_EXPORT int PyType_Ready(PyTypeObject*) {
  UNIMPLEMENTED("This function will never be implemented");
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
  type.setDict(*dict);

  // Set the class name
  const char* class_name = strrchr(spec->name, '.');
  if (class_name == nullptr) {
    class_name = spec->name;
  } else {
    class_name++;
  }
  Object name_obj(&scope, runtime->newStrFromCStr(class_name));
  type.setName(*name_obj);
  Object dict_key(&scope, runtime->symbols()->DunderName());
  runtime->dictAtPutInValueCell(dict, dict_key, name_obj);

  // Compute Mro
  Tuple parents(&scope, runtime->newTuple(0));
  Object mro(&scope, computeMro(thread, type, parents));
  type.setMro(*mro);

  // Initialize instance Layout
  Layout layout_init(
      &scope, runtime->computeInitialLayout(thread, type, LayoutId::kObject));
  Object attr_name(&scope, runtime->symbols()->ExtensionPtr());
  Layout layout(&scope,
                runtime->layoutAddAttribute(thread, layout_init, attr_name, 0));
  layout.setDescribedType(*type);
  type.setInstanceLayout(*layout);

  // Initialize the extension slots tuple
  Object extension_slots(
      &scope, runtime->newTuple(static_cast<int>(Type::ExtensionSlot::kEnd)));
  type.setExtensionSlots(*extension_slots);

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
  DCHECK(!type.isBuiltin(),
         "Type is unmanaged. Please initialize using PyType_FromSpec");
  DCHECK(!type.extensionSlots().isNoneType(),
         "GenericAlloc from types initialized through Python code");
  Int basic_size(&scope, extensionSlot(type, Type::ExtensionSlot::kBasicSize));
  Int item_size(&scope, extensionSlot(type, Type::ExtensionSlot::kItemSize));
  Py_ssize_t size = Utils::roundUp(
      nitems * item_size.asWord() + basic_size.asWord(), kWordSize);

  PyObject* pyobj = static_cast<PyObject*>(PyObject_Calloc(1, size));
  if (pyobj == nullptr) return nullptr;
  pyobj->ob_refcnt = 1;
  pyobj->ob_type = type_obj;
  if (item_size.asWord() != 0) {
    reinterpret_cast<PyVarObject*>(pyobj)->ob_size = nitems;
  }
  return pyobj;
}

PY_EXPORT Py_ssize_t _PyObject_SIZE_Func(PyObject* obj) {
  HandleScope scope;
  Type type(&scope, ApiHandle::fromPyObject(obj)->asObject());
  Int basic_size(&scope, extensionSlot(type, Type::ExtensionSlot::kBasicSize));
  return basic_size.asWord();
}

PY_EXPORT Py_ssize_t _PyObject_VAR_SIZE_Func(PyObject* obj, Py_ssize_t nitems) {
  HandleScope scope;
  Type type(&scope, ApiHandle::fromPyObject(obj)->asObject());
  Int basic_size(&scope, extensionSlot(type, Type::ExtensionSlot::kBasicSize));
  Int item_size(&scope, extensionSlot(type, Type::ExtensionSlot::kItemSize));
  return Utils::roundUp(nitems * item_size.asWord() + basic_size.asWord(),
                        kWordSize);
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
