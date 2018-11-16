// tupleobject.c implementation

#include <cstdarg>

#include "objects.h"
#include "runtime.h"

namespace python {

void PyTuple_Type_Init() {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();

  ApiTypeHandle* pytype_type =
      runtime->builtinTypeHandles(ExtensionTypes::kType);
  ApiTypeHandle* pytuple_type =
      ApiTypeHandle::newTypeHandle("tuple", pytype_type);

  runtime->addBuiltinTypeHandle(pytuple_type);
}

extern "C" PyTypeObject* PyTuple_Type_Ptr() {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  return static_cast<PyTypeObject*>(
      runtime->builtinTypeHandles(ExtensionTypes::kTuple));
}

extern "C" PyObject* PyTuple_New(Py_ssize_t length) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Handle<ObjectArray> tuple(&scope, runtime->newObjectArray(length));
  return ApiHandle::fromObject(*tuple)->asPyObject();
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
    thread->throwSystemErrorFromCString("bad argument to internal function");
    return -1;
  }

  Handle<ObjectArray> tuple(&scope, *tupleobj);
  if (pos < 0 || pos >= tuple->length()) {
    thread->throwIndexErrorFromCString("tuple assignment index out of range");
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
