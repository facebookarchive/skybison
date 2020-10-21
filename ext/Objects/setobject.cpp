#include "cpython-func.h"

#include "capi-handles.h"
#include "runtime.h"
#include "set-builtins.h"

namespace py {

PY_EXPORT int PyFrozenSet_Check_Func(PyObject* obj) {
  DCHECK(obj != nullptr, "obj must not be nullptr");
  return Thread::current()->runtime()->isInstanceOfFrozenSet(
      ApiHandle::fromPyObject(obj)->asObject());
}

PY_EXPORT int PyFrozenSet_CheckExact_Func(PyObject* obj) {
  DCHECK(obj != nullptr, "obj must not be nullptr");
  return ApiHandle::fromPyObject(obj)->asObject().isFrozenSet();
}

PY_EXPORT PyObject* PyFrozenSet_New(PyObject* iterable) {
  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  if (iterable == nullptr) {
    return ApiHandle::newReference(thread, runtime->emptyFrozenSet());
  }
  HandleScope scope(thread);
  Object obj(&scope, ApiHandle::fromPyObject(iterable)->asObject());
  FrozenSet set(&scope, runtime->newFrozenSet());
  Object result(&scope, setUpdate(thread, set, obj));
  if (result.isError()) {
    return nullptr;
  }
  return ApiHandle::newReference(thread, *set);
}

PY_EXPORT PyTypeObject* PyFrozenSet_Type_Ptr() {
  Thread* thread = Thread::current();
  return reinterpret_cast<PyTypeObject*>(ApiHandle::borrowedReference(
      thread, thread->runtime()->typeAt(LayoutId::kFrozenSet)));
}

PY_EXPORT PyTypeObject* PySetIter_Type_Ptr() {
  Thread* thread = Thread::current();
  return reinterpret_cast<PyTypeObject*>(ApiHandle::borrowedReference(
      thread, thread->runtime()->typeAt(LayoutId::kSetIterator)));
}

PY_EXPORT int PySet_Add(PyObject* anyset, PyObject* key) {
  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Object set_obj(&scope, ApiHandle::fromPyObject(anyset)->asObject());

  // TODO(T28454727): add FrozenSet
  if (!runtime->isInstanceOfSet(*set_obj)) {
    thread->raiseBadInternalCall();
    return -1;
  }

  Set set(&scope, *set_obj);
  Object key_obj(&scope, ApiHandle::fromPyObject(key)->asObject());
  Object hash_obj(&scope, Interpreter::hash(thread, key_obj));
  if (hash_obj.isErrorException()) {
    return -1;
  }
  word hash = SmallInt::cast(*hash_obj).value();

  setAdd(thread, set, key_obj, hash);
  return 0;
}

PY_EXPORT int PySet_Check_Func(PyObject* obj) {
  return Thread::current()->runtime()->isInstanceOfSet(
      ApiHandle::fromPyObject(obj)->asObject());
}

PY_EXPORT int _PySet_NextEntry(PyObject* pyset, Py_ssize_t* ppos,
                               PyObject** pkey, Py_hash_t* phash) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object set_obj(&scope, ApiHandle::fromPyObject(pyset)->asObject());
  if (!thread->runtime()->isInstanceOfSetBase(*set_obj)) {
    thread->raiseBadInternalCall();
    return -1;
  }
  SetBase set(&scope, *set_obj);
  Object value(&scope, NoneType::object());
  DCHECK(phash != nullptr, "phash must not be null");
  DCHECK(pkey != nullptr, "pkey must not be null");
  if (!setNextItemHash(set, ppos, &value, phash)) {
    return false;
  }
  *pkey = ApiHandle::borrowedReference(thread, *value);
  return true;
}

PY_EXPORT int PySet_Clear(PyObject* anyset) {
  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Object set_obj(&scope, ApiHandle::fromPyObject(anyset)->asObject());
  if (!runtime->isInstanceOfSetBase(*set_obj)) {
    thread->raiseBadInternalCall();
    return -1;
  }
  SetBase set(&scope, *set_obj);
  set.setNumItems(0);
  set.setData(runtime->emptyTuple());
  return 0;
}

PY_EXPORT int PySet_Contains(PyObject* anyset, PyObject* key) {
  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Object set_obj(&scope, ApiHandle::fromPyObject(anyset)->asObject());

  if (!runtime->isInstanceOfSetBase(*set_obj)) {
    thread->raiseBadInternalCall();
    return -1;
  }

  SetBase set(&scope, *set_obj);
  Object key_obj(&scope, ApiHandle::fromPyObject(key)->asObject());
  Object hash_obj(&scope, Interpreter::hash(thread, key_obj));
  if (hash_obj.isErrorException()) {
    return -1;
  }
  word hash = SmallInt::cast(*hash_obj).value();
  return setIncludes(thread, set, key_obj, hash);
}

PY_EXPORT int PySet_Discard(PyObject* pyset, PyObject* pykey) {
  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Object set_obj(&scope, ApiHandle::fromPyObject(pyset)->asObject());
  if (!runtime->isInstanceOfSet(*set_obj)) {
    thread->raiseBadInternalCall();
    return -1;
  }
  Set set(&scope, *set_obj);
  Object key(&scope, ApiHandle::fromPyObject(pykey)->asObject());
  Object hash_obj(&scope, Interpreter::hash(thread, key));
  if (hash_obj.isErrorException()) {
    return -1;
  }
  word hash = SmallInt::cast(*hash_obj).value();
  return setRemove(thread, set, key, hash);
}

PY_EXPORT PyObject* PySet_New(PyObject* iterable) {
  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  if (iterable == nullptr) {
    return ApiHandle::newReference(thread, runtime->newSet());
  }

  HandleScope scope(thread);
  Object obj(&scope, ApiHandle::fromPyObject(iterable)->asObject());
  Set set(&scope, runtime->newSet());

  Object result(&scope, setUpdate(thread, set, obj));
  if (result.isError()) {
    return nullptr;
  }

  return ApiHandle::newReference(thread, *set);
}

PY_EXPORT PyObject* PySet_Pop(PyObject* pyset) {
  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Object set_obj(&scope, ApiHandle::fromPyObject(pyset)->asObject());
  if (!runtime->isInstanceOfSet(*set_obj)) {
    thread->raiseBadInternalCall();
    return nullptr;
  }
  Set set(&scope, *set_obj);
  Object result(&scope, setPop(thread, set));
  if (result.isErrorException()) return nullptr;
  return ApiHandle::newReference(thread, *result);
}

PY_EXPORT Py_ssize_t PySet_Size(PyObject* anyset) {
  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Object set_obj(&scope, ApiHandle::fromPyObject(anyset)->asObject());
  if (!runtime->isInstanceOfSetBase(*set_obj)) {
    thread->raiseBadInternalCall();
    return -1;
  }

  SetBase set(&scope, *set_obj);
  return set.numItems();
}

PY_EXPORT PyTypeObject* PySet_Type_Ptr() {
  Thread* thread = Thread::current();
  return reinterpret_cast<PyTypeObject*>(ApiHandle::borrowedReference(
      thread, thread->runtime()->typeAt(LayoutId::kSet)));
}

}  // namespace py
