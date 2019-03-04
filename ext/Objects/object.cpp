// object.c implementation

#include "builtins-module.h"
#include "bytes-builtins.h"
#include "cpython-data.h"
#include "cpython-func.h"
#include "runtime.h"
#include "str-builtins.h"

namespace python {

PY_EXPORT PyObject* PyNone_Ptr() {
  return ApiHandle::borrowedReference(Thread::currentThread(),
                                      NoneType::object());
}

PY_EXPORT void _Py_Dealloc_Func(PyObject* obj) {
  ApiHandle::fromPyObject(obj)->dispose();
}

PY_EXPORT void Py_INCREF_Func(PyObject* obj) {
  ApiHandle::fromPyObject(obj)->incref();
}

PY_EXPORT void Py_DECREF_Func(PyObject* obj) {
  obj->ob_refcnt--;
  if (ApiHandle::fromPyObject(obj)->refcnt() == 0) _Py_Dealloc_Func(obj);
}

PY_EXPORT Py_ssize_t Py_REFCNT_Func(PyObject* obj) {
  return ApiHandle::fromPyObject(obj)->refcnt();
}

PY_EXPORT int PyCallable_Check(PyObject* /* x */) {
  UNIMPLEMENTED("PyCallable_Check");
}

PY_EXPORT PyObject* PyObject_ASCII(PyObject* pyobj) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  if (pyobj == nullptr) {
    return ApiHandle::newReference(thread, runtime->symbols()->Null());
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
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  if (pyobj == nullptr) {
    View<byte> view(reinterpret_cast<const byte*>("<NULL>"), 6);
    Bytes result(&scope, thread->runtime()->newBytesWithAll(view));
    return ApiHandle::newReference(thread, *result);
  }

  ApiHandle* handle = ApiHandle::fromPyObject(pyobj);
  Object obj(&scope, handle->asObject());
  if (obj.isBytes()) {
    handle->incref();
    return pyobj;
  }

  Object result(&scope, callDunderBytes(thread, obj));
  if (result.isNoneType()) {
    result = bytesFromIterable(thread, obj);
  }
  if (result.isError()) return nullptr;
  return ApiHandle::newReference(thread, *result);
}

PY_EXPORT int PyObject_CallFinalizerFromDealloc(PyObject* /* f */) {
  UNIMPLEMENTED("PyObject_CallFinalizerFromDealloc");
}

PY_EXPORT PyObject* PyObject_Dir(PyObject* /* j */) {
  UNIMPLEMENTED("PyObject_Dir");
}

PY_EXPORT PyObject* PyObject_GenericGetAttr(PyObject* obj, PyObject* name) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Object object(&scope, ApiHandle::fromPyObject(obj)->asObject());
  if (!object.isHeapObject()) return nullptr;
  Object name_obj(&scope, ApiHandle::fromPyObject(name)->asObject());
  Object result(&scope, getAttribute(thread, object, name_obj));
  return result.isError() ? nullptr : ApiHandle::newReference(thread, *result);
}

PY_EXPORT int PyObject_GenericSetAttr(PyObject* obj, PyObject* name,
                                      PyObject* value) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Object object(&scope, ApiHandle::fromPyObject(obj)->asObject());
  if (!object.isHeapObject()) return -1;
  Object name_obj(&scope, ApiHandle::fromPyObject(name)->asObject());
  Object value_obj(&scope, ApiHandle::fromPyObject(value)->asObject());
  Object result(&scope, setAttribute(thread, object, name_obj, value_obj));
  return result.isError() ? -1 : 0;
}

PY_EXPORT int PyObject_GenericSetDict(PyObject* /* j */, PyObject* /* e */,
                                      void* /* t */) {
  UNIMPLEMENTED("PyObject_GenericSetDict");
}

PY_EXPORT PyObject* PyObject_GetAttr(PyObject* v, PyObject* name) {
  // TODO(miro): This does not support custom attribute getter set by
  // __getattr__ or tp_getattro slot
  return PyObject_GenericGetAttr(v, name);
}

PY_EXPORT PyObject* PyObject_GetAttrString(PyObject* pyobj, const char* name) {
  DCHECK(pyobj != nullptr, "pyobj must not be nullptr");
  DCHECK(name != nullptr, "name must not be nullptr");
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Object object(&scope, ApiHandle::fromPyObject(pyobj)->asObject());
  Object result(&scope,
                thread->runtime()->attributeAtWithCStr(thread, object, name));
  if (result.isError()) return nullptr;
  return ApiHandle::newReference(thread, *result);
}

PY_EXPORT int PyObject_HasAttr(PyObject* pyobj, PyObject* pyname) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Object obj(&scope, ApiHandle::fromPyObject(pyobj)->asObject());
  Object name(&scope, ApiHandle::fromPyObject(pyname)->asObject());
  Object result(&scope, hasAttribute(thread, obj, name));
  if (result.isBool()) return Bool::cast(*result)->value();
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
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Object object(&scope, ApiHandle::fromPyObject(obj)->asObject());
  Object result(&scope, thread->invokeMethod1(object, SymbolId::kDunderHash));
  if (result.isError()) {
    if (!thread->hasPendingException()) {
      thread->raiseTypeErrorWithCStr("unhashable type");
    }
    return -1;
  }
  DCHECK(result.isSmallInt(), "__hash__ should return small int");
  SmallInt hash(&scope, *result);
  return hash.value();
}

PY_EXPORT Py_hash_t PyObject_HashNotImplemented(PyObject* /* v */) {
  Thread* thread = Thread::currentThread();
  thread->raiseTypeErrorWithCStr("unhashable type");
  return -1;
}

PY_EXPORT PyObject* PyObject_Init(PyObject* /* p */, PyTypeObject* /* p */) {
  UNIMPLEMENTED("PyObject_Init");
}

PY_EXPORT PyVarObject* PyObject_InitVar(PyVarObject* /* p */,
                                        PyTypeObject* /* p */,
                                        Py_ssize_t /* e */) {
  UNIMPLEMENTED("PyObject_InitVar");
}

PY_EXPORT int PyObject_IsTrue(PyObject* obj) {
  DCHECK(obj != nullptr, "nullptr passed into PyObject_IsTrue");
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Object obj_obj(&scope, ApiHandle::fromPyObject(obj)->asObject());
  Frame* frame = thread->currentFrame();
  Object result(&scope, Interpreter::isTrue(thread, frame, obj_obj));
  if (result.isError()) {
    return -1;
  }
  return RawBool::cast(*result).value();
}

PY_EXPORT int PyObject_Not(PyObject* obj) {
  int res = PyObject_IsTrue(obj);
  if (res < 0) {
    return res;
  }
  return res == 0;
}

PY_EXPORT int PyObject_Print(PyObject* /* p */, FILE* /* p */, int /* s */) {
  UNIMPLEMENTED("PyObject_Print");
}

// TODO(T38571506): Handle recursive objects safely.
PY_EXPORT PyObject* PyObject_Repr(PyObject* obj) {
  Thread* thread = Thread::currentThread();
  if (obj == nullptr) {
    return ApiHandle::newReference(thread,
                                   thread->runtime()->symbols()->Null());
  }
  HandleScope scope(thread);
  Object object(&scope, ApiHandle::fromPyObject(obj)->asObject());
  Object result(&scope, thread->invokeMethod1(object, SymbolId::kDunderRepr));
  if (result.isError()) {
    return nullptr;
  }
  if (!thread->runtime()->isInstanceOfStr(*result)) {
    thread->raiseTypeErrorWithCStr("__repr__ returned non-str instance");
    return nullptr;
  }
  return ApiHandle::newReference(thread, *result);
}

PY_EXPORT PyObject* PyObject_RichCompare(PyObject* v, PyObject* w, int op) {
  DCHECK(CompareOp::LT <= op && op <= CompareOp::GE, "Bad op");
  Thread* thread = Thread::currentThread();
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

PY_EXPORT int PyObject_SetAttr(PyObject* v, PyObject* name, PyObject* w) {
  // TODO(miro): This does not support custom attribute setter set by
  // __setattr__ or tp_setattro slot
  return PyObject_GenericSetAttr(v, name, w);
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
  Thread* thread = Thread::currentThread();
  if (obj == nullptr) {
    return ApiHandle::newReference(thread,
                                   thread->runtime()->symbols()->Null());
  }
  HandleScope scope(thread);
  Object object(&scope, ApiHandle::fromPyObject(obj)->asObject());
  Object result(&scope, thread->invokeMethod1(object, SymbolId::kDunderStr));
  if (result.isError()) {
    return nullptr;
  }
  if (!thread->runtime()->isInstanceOfStr(*result)) {
    thread->raiseTypeErrorWithCStr("__str__ returned non-str instance");
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
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Object object(&scope, ApiHandle::fromPyObject(obj)->asObject());
  Object result(&scope, thread->reprEnter(object));
  if (result.isError()) {
    return -1;
  }
  return RawBool::cast(*result)->value();
}

PY_EXPORT void Py_ReprLeave(PyObject* obj) {
  Thread* thread = Thread::currentThread();
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

PY_EXPORT PyObject* _PyObject_New(PyTypeObject* /* p */) {
  UNIMPLEMENTED("_PyObject_New");
}

PY_EXPORT PyVarObject* _PyObject_NewVar(PyTypeObject* /* p */,
                                        Py_ssize_t /* s */) {
  UNIMPLEMENTED("_PyObject_NewVar");
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

}  // namespace python
