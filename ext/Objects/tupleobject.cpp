// tupleobject.c implementation

#include <cstdarg>

#include "objects.h"
#include "runtime.h"

namespace python {

PY_EXPORT PyObject* PyTuple_New(Py_ssize_t length) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  ObjectArray tuple(&scope, runtime->newObjectArray(length));
  return ApiHandle::fromObject(*tuple);
}

PY_EXPORT int PyTuple_CheckExact_Func(PyObject* obj) {
  return ApiHandle::fromPyObject(obj)->asObject()->isObjectArray();
}

PY_EXPORT int PyTuple_Check_Func(PyObject* obj) {
  if (PyTuple_CheckExact_Func(obj)) {
    return true;
  }
  return ApiHandle::fromPyObject(obj)->isSubClass(Thread::currentThread(),
                                                  LayoutId::kObjectArray);
}

PY_EXPORT Py_ssize_t PyTuple_Size(PyObject* pytuple) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);

  Object tupleobj(&scope, ApiHandle::fromPyObject(pytuple)->asObject());
  if (!tupleobj->isObjectArray()) {
    return -1;
  }

  ObjectArray tuple(&scope, *tupleobj);
  return tuple->length();
}

PY_EXPORT int PyTuple_SetItem(PyObject* pytuple, Py_ssize_t pos,
                              PyObject* pyitem) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);

  Object tupleobj(&scope, ApiHandle::fromPyObject(pytuple)->asObject());
  if (!tupleobj->isObjectArray()) {
    thread->raiseSystemErrorWithCStr("bad argument to internal function");
    return -1;
  }

  ObjectArray tuple(&scope, *tupleobj);
  if (pos < 0 || pos >= tuple->length()) {
    thread->raiseIndexErrorWithCStr("tuple assignment index out of range");
    return -1;
  }

  Object item(&scope, ApiHandle::fromPyObject(pyitem)->asObject());
  tuple->atPut(pos, *item);
  return 0;
}

PY_EXPORT PyObject* PyTuple_GetItem(PyObject* pytuple, Py_ssize_t pos) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);

  Object tupleobj(&scope, ApiHandle::fromPyObject(pytuple)->asObject());
  if (!tupleobj->isObjectArray()) {
    return nullptr;
  }

  ObjectArray tuple(&scope, *tupleobj);
  if (pos < 0 || pos >= tuple->length()) {
    return nullptr;
  }

  return ApiHandle::fromBorrowedObject(tuple->at(pos));
}

PY_EXPORT PyObject* PyTuple_Pack(Py_ssize_t n, ...) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  PyObject* item;
  va_list vargs;
  va_start(vargs, n);
  ObjectArray tuple(&scope, runtime->newObjectArray(n));
  for (Py_ssize_t i = 0; i < n; i++) {
    item = va_arg(vargs, PyObject*);
    tuple->atPut(i, ApiHandle::fromPyObject(item)->asObject());
  }
  va_end(vargs);
  return ApiHandle::fromObject(*tuple);
}

PY_EXPORT int PyTuple_ClearFreeList() { return 0; }

PY_EXPORT PyObject* PyTuple_GetSlice(PyObject* /* p */, Py_ssize_t /* i */,
                                     Py_ssize_t /* j */) {
  UNIMPLEMENTED("PyTuple_GetSlice");
}

}  // namespace python
