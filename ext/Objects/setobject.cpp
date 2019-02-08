#include "cpython-func.h"
#include "runtime.h"
#include "set-builtins.h"

namespace python {

PY_EXPORT PyObject* PyFrozenSet_New(PyObject* iterable) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  if (iterable == nullptr) {
    return ApiHandle::newReference(thread, runtime->emptyFrozenSet());
  }
  Object obj(&scope, ApiHandle::fromPyObject(iterable)->asObject());
  FrozenSet set(&scope, runtime->newFrozenSet());
  Object result(&scope, runtime->setUpdate(thread, set, obj));
  if (result.isError()) {
    thread->raiseTypeErrorWithCStr("PyFrozenSet_New requires an iterable");
    return nullptr;
  }
  return ApiHandle::newReference(thread, *set);
}

PY_EXPORT int PySet_Add(PyObject* anyset, PyObject* key) {
  Thread* thread = Thread::currentThread();
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

  if (setAdd(thread, set, key_obj)->isError()) {
    return -1;
  }

  return 0;
}

PY_EXPORT int PySet_Clear(PyObject* anyset) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Object set_obj(&scope, ApiHandle::fromPyObject(anyset)->asObject());
  if (!runtime->isInstanceOfSetBase(*set_obj)) {
    thread->raiseBadInternalCall();
    return -1;
  }
  SetBase set(&scope, *set_obj);
  set.setNumItems(0);
  set.setData(runtime->newTuple(0));
  return 0;
}

PY_EXPORT int PySet_ClearFreeList() { return 0; }

PY_EXPORT int PySet_Contains(PyObject* anyset, PyObject* key) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Object set_obj(&scope, ApiHandle::fromPyObject(anyset)->asObject());

  if (!runtime->isInstanceOfSetBase(*set_obj)) {
    thread->raiseBadInternalCall();
    return -1;
  }

  SetBase set(&scope, *set_obj);
  Object key_obj(&scope, ApiHandle::fromPyObject(key)->asObject());

  return runtime->setIncludes(set, key_obj);
}

PY_EXPORT int PySet_Discard(PyObject* pyset, PyObject* pykey) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Object set_obj(&scope, ApiHandle::fromPyObject(pyset)->asObject());
  if (!runtime->isInstanceOfSet(*set_obj)) {
    thread->raiseBadInternalCall();
    return -1;
  }
  Set set(&scope, *set_obj);
  Object key(&scope, ApiHandle::fromPyObject(pykey)->asObject());
  // TODO(T36757907): Create and use setRemoveWithHash
  return runtime->setRemove(set, key);
}

PY_EXPORT PyObject* PySet_New(PyObject* iterable) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  if (iterable == nullptr) {
    return ApiHandle::newReference(thread, runtime->newSet());
  }

  Object obj(&scope, ApiHandle::fromPyObject(iterable)->asObject());
  Set set(&scope, runtime->newSet());

  Object result(&scope, runtime->setUpdate(thread, set, obj));
  if (result.isError()) {
    thread->raiseTypeErrorWithCStr("PySet_New requires an iterable");
    return nullptr;
  }

  return ApiHandle::newReference(thread, *set);
}

PY_EXPORT PyObject* PySet_Pop(PyObject* pyset) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Object set_obj(&scope, ApiHandle::fromPyObject(pyset)->asObject());
  if (!runtime->isInstanceOfSet(*set_obj)) {
    thread->raiseBadInternalCall();
    return nullptr;
  }
  Set set(&scope, *set_obj);
  Object result(&scope, setPop(thread, set));
  if (thread->hasPendingException()) {
    return nullptr;
  }
  return ApiHandle::newReference(thread, *result);
}

PY_EXPORT Py_ssize_t PySet_Size(PyObject* anyset) {
  Thread* thread = Thread::currentThread();
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

}  // namespace python
