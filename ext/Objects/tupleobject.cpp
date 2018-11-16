// tupleobject.c implementation

#include <cstdarg>

#include "objects.h"
#include "runtime.h"

namespace python {

extern "C" PyObject* PyTuple_New(Py_ssize_t length) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Handle<ObjectArray> tuple(&scope, runtime->newObjectArray(length));
  return ApiHandle::fromObject(*tuple)->asPyObject();
}

extern "C" int PyTuple_CheckExact_Func(PyObject* obj) {
  return ApiHandle::fromPyObject(obj)->asObject()->isObjectArray();
}

extern "C" int PyTuple_Check_Func(PyObject* obj) {
  if (PyTuple_CheckExact_Func(obj)) {
    return true;
  }
  return ApiHandle::fromPyObject(obj)->isSubClass(Thread::currentThread(),
                                                  LayoutId::kObjectArray);
}

extern "C" Py_ssize_t PyTuple_Size(PyObject* pytuple) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);

  Handle<Object> tupleobj(&scope, ApiHandle::fromPyObject(pytuple)->asObject());
  if (!tupleobj->isObjectArray()) {
    return -1;
  }

  Handle<ObjectArray> tuple(&scope, *tupleobj);
  return tuple->length();
}

extern "C" int PyTuple_SetItem(PyObject* pytuple, Py_ssize_t pos,
                               PyObject* pyitem) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);

  Handle<Object> tupleobj(&scope, ApiHandle::fromPyObject(pytuple)->asObject());
  if (!tupleobj->isObjectArray()) {
    thread->throwSystemErrorFromCStr("bad argument to internal function");
    return -1;
  }

  Handle<ObjectArray> tuple(&scope, *tupleobj);
  if (pos < 0 || pos >= tuple->length()) {
    thread->throwIndexErrorFromCStr("tuple assignment index out of range");
    return -1;
  }

  Handle<Object> item(&scope, ApiHandle::fromPyObject(pyitem)->asObject());
  tuple->atPut(pos, *item);
  return 0;
}

extern "C" PyObject* PyTuple_GetItem(PyObject* pytuple, Py_ssize_t pos) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);

  Handle<Object> tupleobj(&scope, ApiHandle::fromPyObject(pytuple)->asObject());
  if (!tupleobj->isObjectArray()) {
    return nullptr;
  }

  Handle<ObjectArray> tuple(&scope, *tupleobj);
  if (pos < 0 || pos >= tuple->length()) {
    return nullptr;
  }

  return ApiHandle::fromBorrowedObject(tuple->at(pos))->asPyObject();
}

extern "C" PyObject* PyTuple_Pack(Py_ssize_t n, ...) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  PyObject* item;
  va_list vargs;
  va_start(vargs, n);
  Handle<ObjectArray> tuple(&scope, runtime->newObjectArray(n));
  for (Py_ssize_t i = 0; i < n; i++) {
    item = va_arg(vargs, PyObject*);
    tuple->atPut(i, ApiHandle::fromPyObject(item)->asObject());
  }
  va_end(vargs);
  return ApiHandle::fromObject(*tuple)->asPyObject();
}

}  // namespace python
