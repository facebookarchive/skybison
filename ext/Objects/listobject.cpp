// listobject.c implementation

#include "cpython-func.h"
#include "handles.h"
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
  if (PyList_CheckExact_Func(obj)) {
    return true;
  }
  return ApiHandle::fromPyObject(obj)->isSubclass(Thread::currentThread(),
                                                  LayoutId::kList);
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

PY_EXPORT int PyList_Reverse(PyObject* /* v */) {
  UNIMPLEMENTED("PyList_Reverse");
}

PY_EXPORT int PyList_SetItem(PyObject* /* p */, Py_ssize_t /* i */,
                             PyObject* /* m */) {
  UNIMPLEMENTED("PyList_SetItem");
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

PY_EXPORT PyObject* PyList_GetSlice(PyObject* /* a */, Py_ssize_t /* w */,
                                    Py_ssize_t /* h */) {
  UNIMPLEMENTED("PyList_GetSlice");
}

PY_EXPORT int PyList_Insert(PyObject* /* p */, Py_ssize_t /* e */,
                            PyObject* /* m */) {
  UNIMPLEMENTED("PyList_Insert");
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

PY_EXPORT int PyList_Sort(PyObject* /* v */) { UNIMPLEMENTED("PyList_Sort"); }

}  // namespace python
