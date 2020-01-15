// object.c implementation

#include "cpython-data.h"
#include "cpython-func.h"

#include "builtins-module.h"
#include "bytes-builtins.h"
#include "capi-handles.h"
#include "dict-builtins.h"
#include "object-builtins.h"
#include "runtime.h"
#include "str-builtins.h"
#include "type-builtins.h"

namespace py {

PY_EXPORT PyObject* PyEllipsis_Ptr() {
  Thread* thread = Thread::current();
  return ApiHandle::borrowedReference(thread, thread->runtime()->ellipsis());
}

PY_EXPORT PyObject* PyNone_Ptr() {
  return ApiHandle::borrowedReference(Thread::current(), NoneType::object());
}

PY_EXPORT PyObject* PyNotImplemented_Ptr() {
  return ApiHandle::borrowedReference(Thread::current(),
                                      NotImplementedType::object());
}

PY_EXPORT void _Py_Dealloc_Func(PyObject*) {
  // Do nothing, since this function is only relevant for functions that define
  // their own memory allocation. Right now we don't support this and all
  // object memory is tracked by the GC.
  UNIMPLEMENTED("_Py_Dealloc_Func");
}

PY_EXPORT void Py_INCREF_Func(PyObject* obj) {
  if (ApiHandle::isManaged(obj)) return ApiHandle::fromPyObject(obj)->incref();
  obj->ob_refcnt++;
}

PY_EXPORT Py_ssize_t Py_REFCNT_Func(PyObject* obj) {
  if (ApiHandle::isManaged(obj)) return ApiHandle::fromPyObject(obj)->refcnt();
  return obj->ob_refcnt;
}

PY_EXPORT void Py_DECREF_Func(PyObject* obj) {
  if (ApiHandle::isManaged(obj)) return ApiHandle::fromPyObject(obj)->decref();
  // All extension objects have a reference count of 1 which describes the
  // reference from the heap. Therefore, only the garbage collector can cause
  // an object to have its reference go below 1.
  DCHECK(obj->ob_refcnt > 1, "Reference count underflowed");
  obj->ob_refcnt--;
  if (obj->ob_refcnt == 0) _Py_Dealloc_Func(obj);
}

PY_EXPORT int PyCallable_Check(PyObject* obj) {
  if (obj == nullptr) return 0;
  return PyObject_HasAttrString(obj, "__call__");
}

PY_EXPORT PyObject* PyObject_ASCII(PyObject* pyobj) {
  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  if (pyobj == nullptr) {
    return ApiHandle::newReference(thread, runtime->symbols()->LtNullGt());
  }
  HandleScope scope(thread);
  Object obj(&scope, ApiHandle::fromPyObject(pyobj)->asObject());
  Object result(&scope, thread->invokeFunction1(SymbolId::kBuiltins,
                                                SymbolId::kAscii, obj));
  if (result.isError()) {
    return nullptr;
  }
  return ApiHandle::newReference(thread, *result);
}

PY_EXPORT PyObject* PyObject_Bytes(PyObject* pyobj) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  if (pyobj == nullptr) {
    static const byte value[] = "<NULL>";
    return ApiHandle::newReference(thread, runtime->newBytesWithAll(value));
  }

  ApiHandle* handle = ApiHandle::fromPyObject(pyobj);
  Object obj(&scope, handle->asObject());
  if (obj.isBytes()) {
    handle->incref();
    return pyobj;
  }

  Object result(&scope, thread->invokeMethod1(obj, SymbolId::kDunderBytes));
  if (result.isError()) {
    if (result.isErrorException()) return nullptr;
    // Attribute lookup failed
    result = thread->invokeFunction1(SymbolId::kBuiltins,
                                     SymbolId::kUnderBytesNew, obj);
    if (result.isErrorException()) return nullptr;
    DCHECK(!result.isError(), "Couldn't call builtins._bytes_new");
  } else if (!runtime->isInstanceOfBytes(*result)) {
    thread->raiseWithFmt(LayoutId::kTypeError,
                         "__bytes__ returned non-bytes (type %T)", &result);
    return nullptr;
  }
  return ApiHandle::newReference(thread, *result);
}

PY_EXPORT void PyObject_CallFinalizer(PyObject* self) {
  PyTypeObject* type = Py_TYPE(self);
  unsigned long type_flags = PyType_GetFlags(type);
  auto finalizer =
      reinterpret_cast<destructor>(PyType_GetSlot(type, Py_tp_finalize));
  if (!(type_flags & Py_TPFLAGS_HAVE_FINALIZE) || finalizer == nullptr) {
    // Nothing to finalize.
    return;
  }
  bool is_gc = type_flags & Py_TPFLAGS_HAVE_GC;
  if (is_gc) {
    // TODO(T55208267): Support GC types
    UNIMPLEMENTED(
        "PyObject_CallFinalizer with finalizer and gc type is not "
        "yet supported");
  }
  // TODO(T55208267): Check if the type has GC flags and the object is already
  // finalized and return early. tp_finalize should only be called once.
  (*finalizer)(self);
  // TODO(T55208267): Check if the type has GC flags set a bit on the object to
  // indicate that it has been finalized already.
}

PY_EXPORT int PyObject_CallFinalizerFromDealloc(PyObject* self) {
  DCHECK(self != nullptr, "self cannot be null");
  if (Py_REFCNT(self) != 0) {
    Py_FatalError(
        "PyObject_CallFinalizerFromDealloc called on "
        "object with a non-zero refcount");
  }
  // Temporarily resurrect the object.
  self->ob_refcnt = 1;
  // Finalize the object.
  PyObject_CallFinalizer(self);
  if (self->ob_refcnt == 1) {
    // tp_finalize did not resurrect the object, so undo the temporary
    // resurrection and put it to rest.
    self->ob_refcnt--;
    return 0;
  }
  DCHECK(Py_REFCNT(self) > 0, "refcnt must be positive");
  // If we get here, tp_finalize resurrected the object.
  return -1;
}

PY_EXPORT PyObject* PyObject_Dir(PyObject* pyobj) {
  Thread* thread = Thread::current();
  if (pyobj == nullptr && thread->currentFrame()->isSentinel()) {
    return nullptr;
  }
  HandleScope scope(thread);
  Object result(&scope, NoneType::object());
  if (pyobj == nullptr) {
    result = thread->invokeFunction0(SymbolId::kBuiltins, SymbolId::kDir);
  } else {
    Object obj(&scope, ApiHandle::fromPyObject(pyobj)->asObject());
    result = thread->invokeFunction1(SymbolId::kBuiltins, SymbolId::kDir, obj);
  }
  if (result.isError()) {
    return nullptr;
  }
  return ApiHandle::newReference(thread, *result);
}

PY_EXPORT PyObject* PyObject_GenericGetAttr(PyObject* obj, PyObject* name) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object object(&scope, ApiHandle::fromPyObject(obj)->asObject());
  Object name_obj(&scope, ApiHandle::fromPyObject(name)->asObject());
  name_obj = attributeName(thread, name_obj);
  if (name_obj.isErrorException()) return nullptr;
  Object result(&scope, objectGetAttribute(thread, object, name_obj));
  if (result.isError()) {
    if (result.isErrorNotFound()) {
      objectRaiseAttributeError(thread, object, name_obj);
    }
    return nullptr;
  }
  return ApiHandle::newReference(thread, *result);
}

PY_EXPORT int PyObject_GenericSetAttr(PyObject* obj, PyObject* name,
                                      PyObject* value) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object object(&scope, ApiHandle::fromPyObject(obj)->asObject());
  Object name_obj(&scope, ApiHandle::fromPyObject(name)->asObject());
  name_obj = attributeName(thread, name_obj);
  if (name_obj.isErrorException()) return -1;
  Object value_obj(&scope, ApiHandle::fromPyObject(value)->asObject());
  Object result(&scope, objectSetAttr(thread, object, name_obj, value_obj));
  if (result.isErrorException()) {
    return -1;
  }
  return 0;
}

PY_EXPORT int PyObject_GenericSetDict(PyObject* /* j */, PyObject* /* e */,
                                      void* /* t */) {
  UNIMPLEMENTED("PyObject_GenericSetDict");
}

PY_EXPORT PyObject* PyObject_GetAttr(PyObject* obj, PyObject* name) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object object(&scope, ApiHandle::fromPyObject(obj)->asObject());
  Object name_obj(&scope, ApiHandle::fromPyObject(name)->asObject());
  Object result(&scope, getAttribute(thread, object, name_obj));
  return result.isError() ? nullptr : ApiHandle::newReference(thread, *result);
}

PY_EXPORT PyObject* PyObject_GetAttrString(PyObject* pyobj, const char* name) {
  DCHECK(pyobj != nullptr, "pyobj must not be nullptr");
  DCHECK(name != nullptr, "name must not be nullptr");
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object object(&scope, ApiHandle::fromPyObject(pyobj)->asObject());
  Object result(&scope,
                thread->runtime()->attributeAtByCStr(thread, object, name));
  if (result.isError()) return nullptr;
  return ApiHandle::newReference(thread, *result);
}

PY_EXPORT int PyObject_HasAttr(PyObject* pyobj, PyObject* pyname) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object obj(&scope, ApiHandle::fromPyObject(pyobj)->asObject());
  Object name(&scope, ApiHandle::fromPyObject(pyname)->asObject());
  Object result(&scope, hasAttribute(thread, obj, name));
  if (result.isBool()) return Bool::cast(*result).value();
  thread->clearPendingException();
  return false;
}

PY_EXPORT int PyObject_HasAttrString(PyObject* pyobj, const char* name) {
  PyObject* pyname = PyUnicode_FromString(name);
  if (pyname == nullptr) {
    PyErr_Clear();
    return 0;
  }
  int result = PyObject_HasAttr(pyobj, pyname);
  Py_DECREF(pyname);
  return result;
}

PY_EXPORT Py_hash_t PyObject_Hash(PyObject* obj) {
  DCHECK(obj != nullptr, "obj should not be nullptr");
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object object(&scope, ApiHandle::fromPyObject(obj)->asObject());
  Object result(&scope, Interpreter::hash(thread, object));
  if (result.isErrorException()) return -1;
  return SmallInt::cast(*result).value();
}

PY_EXPORT Py_hash_t PyObject_HashNotImplemented(PyObject* /* v */) {
  Thread* thread = Thread::current();
  thread->raiseWithFmt(LayoutId::kTypeError, "unhashable type");
  return -1;
}

PY_EXPORT PyObject* PyObject_Init(PyObject* obj, PyTypeObject* typeobj) {
  if (obj == nullptr) return PyErr_NoMemory();

  // Create a managed proxy for the native instance
  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Type type_obj(&scope, ApiHandle::fromPyTypeObject(typeobj)->asObject());
  Layout layout(&scope, type_obj.instanceLayout());
  Object native_proxy(&scope, runtime->newInstance(layout));
  runtime->setNativeProxyPtr(*native_proxy, obj);

  // Native GC instace tracking is handled by PyObject_GC_Track
  if (!type_obj.hasFlag(Type::Flag::kHasCycleGC)) {
    runtime->trackNativeObject(reinterpret_cast<ListEntry*>(obj) - 1);
  }

  // Initialize the native object
  obj->reference_ = native_proxy.raw();
  Py_INCREF(typeobj);
  obj->ob_refcnt = 2;
  return obj;
}

PY_EXPORT PyVarObject* PyObject_InitVar(PyVarObject* obj, PyTypeObject* type,
                                        Py_ssize_t size) {
  if (obj == nullptr) return reinterpret_cast<PyVarObject*>(PyErr_NoMemory());
  obj->ob_size = size;
  PyObject_Init(reinterpret_cast<PyObject*>(obj), type);
  return obj;
}

PY_EXPORT int PyObject_IsTrue(PyObject* obj) {
  DCHECK(obj != nullptr, "nullptr passed into PyObject_IsTrue");
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object obj_obj(&scope, ApiHandle::fromPyObject(obj)->asObject());
  Object result(&scope, Interpreter::isTrue(thread, *obj_obj));
  if (result.isError()) {
    return -1;
  }
  return Bool::cast(*result).value();
}

PY_EXPORT int PyObject_Not(PyObject* obj) {
  int res = PyObject_IsTrue(obj);
  if (res < 0) {
    return res;
  }
  return res == 0;
}

PY_EXPORT int PyObject_Print(PyObject* obj, FILE* fp, int flags) {
  if (PyErr_CheckSignals()) return -1;
  std::clearerr(fp);  // Clear any previous error condition
  if (obj == nullptr) {
    std::fprintf(fp, "<nil>");
  } else {
    PyObject* str =
        flags & Py_PRINT_RAW ? PyObject_Str(obj) : PyObject_Repr(obj);
    if (str == nullptr) return -1;
    if (!PyUnicode_Check(str)) {
      PyErr_Format(PyExc_TypeError, "str() or repr() returned '%.100s'",
                   _PyType_Name(Py_TYPE(str)));
      Py_DECREF(str);
      return -1;
    }
    PyObject* bytes =
        PyUnicode_AsEncodedString(str, "utf-8", "backslashreplace");
    Py_DECREF(str);
    if (bytes == nullptr) {
      return -1;
    }
    char* c_str = PyBytes_AsString(bytes);
    std::fputs(c_str, fp);
    Py_DECREF(bytes);
  }
  if (std::ferror(fp)) {
    PyErr_SetFromErrno(PyExc_IOError);
    std::clearerr(fp);
    return -1;
  }
  return 0;
}

// TODO(T38571506): Handle recursive objects safely.
PY_EXPORT PyObject* PyObject_Repr(PyObject* obj) {
  Thread* thread = Thread::current();
  if (obj == nullptr) {
    return ApiHandle::newReference(thread,
                                   thread->runtime()->symbols()->LtNullGt());
  }
  HandleScope scope(thread);
  Object object(&scope, ApiHandle::fromPyObject(obj)->asObject());
  Object result(&scope, thread->invokeMethod1(object, SymbolId::kDunderRepr));
  if (result.isError()) {
    return nullptr;
  }
  if (!thread->runtime()->isInstanceOfStr(*result)) {
    thread->raiseWithFmt(LayoutId::kTypeError,
                         "__repr__ returned non-str instance");
    return nullptr;
  }
  return ApiHandle::newReference(thread, *result);
}

PY_EXPORT PyObject* PyObject_RichCompare(PyObject* v, PyObject* w, int op) {
  DCHECK(CompareOp::LT <= op && op <= CompareOp::GE, "Bad op");
  Thread* thread = Thread::current();
  if (v == nullptr || w == nullptr) {
    if (!thread->hasPendingException()) {
      thread->raiseBadInternalCall();
    }
    return nullptr;
  }
  // TODO(emacs): Recursive call check
  HandleScope scope(thread);
  Object left(&scope, ApiHandle::fromPyObject(v)->asObject());
  Object right(&scope, ApiHandle::fromPyObject(w)->asObject());
  Object result(&scope, Interpreter::compareOperation(
                            thread, thread->currentFrame(),
                            static_cast<CompareOp>(op), left, right));
  if (result.isError()) {
    return nullptr;
  }
  return ApiHandle::newReference(thread, *result);
}

PY_EXPORT int PyObject_RichCompareBool(PyObject* left, PyObject* right,
                                       int op) {
  // Quick result when objects are the same. Guarantees that identity implies
  // equality.
  if (left == right) {
    if (op == Py_EQ) {
      return 1;
    }
    if (op == Py_NE) {
      return 0;
    }
  }
  PyObject* res = PyObject_RichCompare(left, right, op);
  if (res == nullptr) {
    return -1;
  }
  int ok;
  if (PyBool_Check(res)) {
    ok = (res == Py_True);
  } else {
    ok = PyObject_IsTrue(res);
  }
  Py_DECREF(res);
  return ok;
}

PY_EXPORT PyObject* PyObject_SelfIter(PyObject* obj) {
  Py_INCREF(obj);
  return obj;
}

PY_EXPORT int PyObject_SetAttr(PyObject* obj, PyObject* name, PyObject* value) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object object(&scope, ApiHandle::fromPyObject(obj)->asObject());
  Object name_obj(&scope, ApiHandle::fromPyObject(name)->asObject());
  Object value_obj(&scope, ApiHandle::fromPyObject(value)->asObject());
  Object result(&scope, setAttribute(thread, object, name_obj, value_obj));
  return result.isError() ? -1 : 0;
}

PY_EXPORT int PyObject_SetAttrString(PyObject* v, const char* name,
                                     PyObject* w) {
  PyObject* str = PyUnicode_FromString(name);
  if (str == nullptr) return -1;
  int result = PyObject_SetAttr(v, str, w);
  Py_DECREF(str);
  return result;
}

// TODO(T38571506): Handle recursive objects safely.
PY_EXPORT PyObject* PyObject_Str(PyObject* obj) {
  Thread* thread = Thread::current();
  if (obj == nullptr) {
    return ApiHandle::newReference(thread,
                                   thread->runtime()->symbols()->LtNullGt());
  }
  HandleScope scope(thread);
  Object object(&scope, ApiHandle::fromPyObject(obj)->asObject());
  Object result(&scope, thread->invokeMethod1(object, SymbolId::kDunderStr));
  if (result.isError()) {
    return nullptr;
  }
  if (!thread->runtime()->isInstanceOfStr(*result)) {
    thread->raiseWithFmt(LayoutId::kTypeError,
                         "__str__ returned non-str instance");
    return nullptr;
  }
  return ApiHandle::newReference(thread, *result);
}

PY_EXPORT void Py_DecRef(PyObject* obj) {
  if (obj == nullptr) return;
  Py_DECREF_Func(obj);
}

PY_EXPORT void Py_IncRef(PyObject* obj) {
  if (obj == nullptr) return;
  Py_INCREF_Func(obj);
}

PY_EXPORT int Py_ReprEnter(PyObject* obj) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object object(&scope, ApiHandle::fromPyObject(obj)->asObject());
  Object result(&scope, thread->reprEnter(object));
  if (result.isError()) {
    return -1;
  }
  return Bool::cast(*result).value();
}

PY_EXPORT void Py_ReprLeave(PyObject* obj) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object object(&scope, ApiHandle::fromPyObject(obj)->asObject());
  thread->reprLeave(object);
}

PY_EXPORT PyObject* _PyObject_GetAttrId(PyObject* /* v */,
                                        _Py_Identifier* /* e */) {
  UNIMPLEMENTED("_PyObject_GetAttrId");
}

PY_EXPORT int _PyObject_HasAttrId(PyObject* /* v */, _Py_Identifier* /* e */) {
  UNIMPLEMENTED("_PyObject_HasAttrId");
}

PY_EXPORT PyObject* _PyObject_New(PyTypeObject* type) {
  PyObject* obj = static_cast<PyObject*>(PyObject_MALLOC(_PyObject_SIZE(type)));
  if (obj == nullptr) return PyErr_NoMemory();
  return PyObject_INIT(obj, type);
}

PY_EXPORT PyVarObject* _PyObject_NewVar(PyTypeObject* type, Py_ssize_t nitems) {
  PyObject* obj =
      static_cast<PyObject*>(PyObject_MALLOC(_PyObject_VAR_SIZE(type, nitems)));
  if (obj == nullptr) return reinterpret_cast<PyVarObject*>(PyErr_NoMemory());
  return PyObject_INIT_VAR(obj, type, nitems);
}

PY_EXPORT int _PyObject_SetAttrId(PyObject* /* v */, _Py_Identifier* /* e */,
                                  PyObject* /* w */) {
  UNIMPLEMENTED("_PyObject_SetAttrId");
}

PY_EXPORT void _PyTrash_deposit_object(PyObject* /* p */) {
  UNIMPLEMENTED("_PyTrash_deposit_object");
}

PY_EXPORT void _PyTrash_destroy_chain() {
  UNIMPLEMENTED("_PyTrash_destroy_chain");
}

PY_EXPORT void _PyTrash_thread_deposit_object(PyObject* /* p */) {
  UNIMPLEMENTED("_PyTrash_thread_deposit_object");
}

PY_EXPORT void _PyTrash_thread_destroy_chain() {
  UNIMPLEMENTED("_PyTrash_thread_destroy_chain");
}

}  // namespace py
