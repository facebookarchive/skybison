#include "capi-handles.h"
#include "runtime.h"

namespace python {

PY_EXPORT int PyWeakref_Check_Func(PyObject* obj) {
  return ApiHandle::fromPyObject(obj)->asObject().isWeakRef();
}

PY_EXPORT void PyObject_ClearWeakRefs(PyObject* /* obj */) {
  // Do nothing and delegated to the garbage collector.
}

PY_EXPORT void _PyWeakref_ClearRef(PyWeakReference* self) {
  DCHECK(self != nullptr, "_PyWeakref_ClearRef expects self to be not null");
  HandleScope scope;
  Object self_obj(
      &scope,
      ApiHandle::fromPyObject(reinterpret_cast<PyObject*>(self))->asObject());
  DCHECK(self_obj.isWeakRef(),
         "_PyWeakref_ClearRef expects self to be a weakref object");
  WeakRef ref(&scope, *self_obj);
  ref.setReferent(NoneType::object());
}

static PyObject* getObject(Thread* thread, const Object& obj) {
  // ref is assumed to be a WeakRef already
  HandleScope scope(thread);
  WeakRef ref(&scope, *obj);
  return ApiHandle::borrowedReference(thread, ref.referent());
}

PY_EXPORT PyObject* PyWeakref_GET_OBJECT_Func(PyObject* ref) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object obj(&scope, ApiHandle::fromPyObject(ref)->asObject());
  return getObject(thread, obj);
}

PY_EXPORT PyObject* PyWeakref_GetObject(PyObject* ref) {
  Thread* thread = Thread::current();
  if (ref == nullptr) {
    thread->raiseWithFmt(LayoutId::kSystemError,
                         "PyWeakref_GetObject expected non-null ref");
    return nullptr;
  }
  HandleScope scope(thread);
  Object obj(&scope, ApiHandle::fromPyObject(ref)->asObject());
  if (!obj.isWeakRef()) {
    thread->raiseWithFmt(LayoutId::kSystemError,
                         "PyWeakref_GetObject expected weakref");
    return nullptr;
  }
  return getObject(thread, obj);
}

PY_EXPORT PyObject* PyWeakref_NewProxy(PyObject* ob, PyObject* callback) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object referent(&scope, ApiHandle::fromPyObject(ob)->asObject());
  Object callback_obj(&scope, NoneType::object());
  if (callback != nullptr) {
    callback_obj = ApiHandle::fromPyObject(callback)->asObject();
  }
  Object result_obj(
      &scope, thread->invokeFunction2(SymbolId::kUnderWeakRef, SymbolId::kProxy,
                                      referent, callback_obj));
  if (result_obj.isError()) {
    return nullptr;
  }
  return ApiHandle::newReference(thread, *result_obj);
}

PY_EXPORT PyObject* PyWeakref_NewRef(PyObject* obj, PyObject* callback) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object referent(&scope, ApiHandle::fromPyObject(obj)->asObject());
  Object callback_obj(&scope, NoneType::object());
  if (callback != nullptr) {
    callback_obj = ApiHandle::fromPyObject(callback)->asObject();
  }
  Runtime* runtime = thread->runtime();
  if (callback_obj.isNoneType() || runtime->isCallable(thread, callback_obj)) {
    return ApiHandle::newReference(
        thread, runtime->newWeakRef(thread, referent, callback_obj));
  }
  thread->raiseWithFmt(LayoutId::kTypeError, "callback is not callable");
  return nullptr;
}

}  // namespace python
