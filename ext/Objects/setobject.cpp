#include "capi-handles.h"
#include "cpython-func.h"
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
  HandleScope scope(thread);
  if (iterable == nullptr) {
    return ApiHandle::newReference(thread, runtime->emptyFrozenSet());
  }
  Object obj(&scope, ApiHandle::fromPyObject(iterable)->asObject());
  FrozenSet set(&scope, runtime->newFrozenSet());
  Object result(&scope, runtime->setUpdate(thread, set, obj));
  if (result.isError()) {
    return nullptr;
  }
  return ApiHandle::newReference(thread, *set);
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
  Object key_obj_hash(&scope, Interpreter::hash(thread, key_obj));
  if (key_obj_hash.isErrorException()) {
    return -1;
  }

  runtime->setAdd(thread, set, key_obj, key_obj_hash);
  return 0;
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
  Tuple set_data(&scope, set.data());
  if (!SetBase::Bucket::nextItem(*set_data, ppos)) {
    return false;
  }
  DCHECK(pkey != nullptr, "pkey must not be null");
  DCHECK(phash != nullptr, "phash must not be null");
  *pkey = ApiHandle::borrowedReference(
      thread, SetBase::Bucket::value(*set_data, *ppos));
  *phash = SmallInt::cast(SetBase::Bucket::hash(*set_data, *ppos)).value();
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

PY_EXPORT int PySet_ClearFreeList() { return 0; }

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
  Object key_hash(&scope, Interpreter::hash(thread, key_obj));
  if (key_hash.isErrorException()) {
    return -1;
  }
  return runtime->setIncludes(thread, set, key_obj, key_hash);
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
  Object key_hash(&scope, Interpreter::hash(thread, key));
  if (key_hash.isErrorException()) {
    return -1;
  }
  return runtime->setRemove(thread, set, key, key_hash);
}

PY_EXPORT PyObject* PySet_New(PyObject* iterable) {
  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  if (iterable == nullptr) {
    return ApiHandle::newReference(thread, runtime->newSet());
  }

  Object obj(&scope, ApiHandle::fromPyObject(iterable)->asObject());
  Set set(&scope, runtime->newSet());

  Object result(&scope, runtime->setUpdate(thread, set, obj));
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

}  // namespace py
