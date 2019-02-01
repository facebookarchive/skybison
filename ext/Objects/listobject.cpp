// listobject.c implementation

#include "cpython-func.h"
#include "handles.h"
#include "list-builtins.h"
#include "objects.h"
#include "runtime.h"

namespace python {

PY_EXPORT PyObject* PyList_New(Py_ssize_t size) {
  if (size < 0) {
    return nullptr;
  }

  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  List list(&scope, runtime->newList());
  Tuple items(&scope, runtime->newTuple(size));
  list->setNumItems(size);
  list->setItems(*items);

  return ApiHandle::newReference(thread, *list);
}

PY_EXPORT int PyList_CheckExact_Func(PyObject* obj) {
  return ApiHandle::fromPyObject(obj)->asObject()->isList();
}

PY_EXPORT int PyList_Check_Func(PyObject* obj) {
  return Thread::currentThread()->runtime()->isInstanceOfList(
      ApiHandle::fromPyObject(obj)->asObject());
}

PY_EXPORT PyObject* PyList_AsTuple(PyObject* pylist) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  if (pylist == nullptr) {
    thread->raiseBadInternalCall();
    return nullptr;
  }
  Object list_obj(&scope, ApiHandle::fromPyObject(pylist)->asObject());
  if (!runtime->isInstanceOfList(*list_obj)) {
    thread->raiseBadInternalCall();
    return nullptr;
  }
  List list(&scope, *list_obj);
  Tuple tuple(&scope, runtime->newTuple(list->numItems()));
  for (Py_ssize_t i = 0; i < list->numItems(); i++) {
    tuple->atPut(i, list->at(i));
  }
  return ApiHandle::newReference(thread, *tuple);
}

PY_EXPORT PyObject* PyList_GetItem(PyObject* pylist, Py_ssize_t i) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Object list_obj(&scope, ApiHandle::fromPyObject(pylist)->asObject());
  if (!runtime->isInstanceOfList(*list_obj)) {
    thread->raiseBadInternalCall();
    return nullptr;
  }
  List list(&scope, *list_obj);
  if (i >= list->numItems()) {
    thread->raiseIndexErrorWithCStr("index out of bounds in PyList_GetItem");
    return nullptr;
  }
  Object value(&scope, list->at(i));
  return ApiHandle::borrowedReference(thread, *value);
}

PY_EXPORT int PyList_Reverse(PyObject* pylist) {
  Thread* thread = Thread::currentThread();
  if (pylist == nullptr) {
    thread->raiseBadInternalCall();
    return -1;
  }
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Object list_obj(&scope, ApiHandle::fromPyObject(pylist)->asObject());
  if (!runtime->isInstanceOfList(*list_obj)) {
    thread->raiseBadInternalCall();
    return -1;
  }
  List list(&scope, *list_obj);
  listReverse(thread, list);
  return 0;
}

PY_EXPORT int PyList_SetItem(PyObject* pylist, Py_ssize_t i, PyObject* item) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Object list_obj(&scope, ApiHandle::fromPyObject(pylist)->asObject());
  if (!runtime->isInstanceOfList(*list_obj)) {
    thread->raiseBadInternalCall();
    return -1;
  }
  List list(&scope, *list_obj);
  if (i >= list->numItems()) {
    thread->raiseIndexErrorWithCStr("index out of bounds in PyList_SetItem");
    return -1;
  }
  list->atPut(i, ApiHandle::fromPyObject(item)->asObject());
  return 0;
}

PY_EXPORT int PyList_Append(PyObject* op, PyObject* newitem) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  if (newitem == nullptr) {
    thread->raiseBadInternalCall();
    return -1;
  }
  Object value(&scope, ApiHandle::fromPyObject(newitem)->asObject());

  Object list_obj(&scope, ApiHandle::fromPyObject(op)->asObject());
  if (!runtime->isInstanceOfList(*list_obj)) {
    thread->raiseBadInternalCall();
    return -1;
  }
  List list(&scope, *list_obj);

  runtime->listAdd(list, value);
  Py_INCREF(newitem);
  return 0;
}

PY_EXPORT int PyList_ClearFreeList() { return 0; }

PY_EXPORT PyObject* PyList_GetSlice(PyObject* pylist, Py_ssize_t low,
                                    Py_ssize_t high) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Object list_obj(&scope, ApiHandle::fromPyObject(pylist)->asObject());
  if (!runtime->isInstanceOfList(*list_obj)) {
    thread->raiseBadInternalCall();
    return nullptr;
  }
  List list(&scope, *list_obj);
  if (low < 0) {
    low = 0;
  } else if (low > list->numItems()) {
    low = list->numItems();
  }
  if (high < low) {
    high = low;
  } else if (high > list->numItems()) {
    high = list->numItems();
  }
  Slice slice(&scope, runtime->newSlice());
  slice->setStart(runtime->newInt(low));
  slice->setStop(runtime->newInt(high));
  slice->setStep(SmallInt::fromWord(1));
  return ApiHandle::newReference(thread, listSlice(thread, list, slice));
}

PY_EXPORT int PyList_Insert(PyObject* pylist, Py_ssize_t where,
                            PyObject* item) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  if (item == nullptr) {
    thread->raiseBadInternalCall();
    return -1;
  }
  Object list_obj(&scope, ApiHandle::fromPyObject(pylist)->asObject());
  if (!runtime->isInstanceOfList(*list_obj)) {
    thread->raiseBadInternalCall();
    return -1;
  }
  List list(&scope, *list_obj);
  if (list->numItems() == kMaxWord) {
    thread->raiseSystemErrorWithCStr("cannot add more objects to list");
    return -1;
  }
  Object item_obj(&scope, ApiHandle::fromPyObject(item)->asObject());
  listInsert(thread, list, item_obj, where);
  return 0;
}

PY_EXPORT int PyList_SetSlice(PyObject* /* a */, Py_ssize_t /* w */,
                              Py_ssize_t /* h */, PyObject* /* v */) {
  UNIMPLEMENTED("PyList_SetSlice");
}

PY_EXPORT Py_ssize_t PyList_Size(PyObject* p) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Object list_obj(&scope, ApiHandle::fromPyObject(p)->asObject());
  if (!runtime->isInstanceOfList(*list_obj)) {
    thread->raiseBadInternalCall();
    return -1;
  }

  List list(&scope, *list_obj);
  return list->numItems();
}

PY_EXPORT int PyList_Sort(PyObject* pylist) {
  Thread* thread = Thread::currentThread();
  if (pylist == nullptr) {
    thread->raiseBadInternalCall();
    return -1;
  }
  HandleScope scope(thread);
  Object list_obj(&scope, ApiHandle::fromPyObject(pylist)->asObject());
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfList(*list_obj)) {
    thread->raiseBadInternalCall();
    return -1;
  }
  List list(&scope, *list_obj);
  if (listSort(thread, list).isError()) {
    return -1;
  }
  return 0;
}

}  // namespace python
