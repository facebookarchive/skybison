// tupleobject.c implementation

#include <cstdarg>

#include "api-handle.h"
#include "objects.h"
#include "runtime.h"
#include "tuple-builtins.h"

namespace py {

PY_EXPORT PyTypeObject* PyTupleIter_Type_Ptr() {
  Runtime* runtime = Thread::current()->runtime();
  return reinterpret_cast<PyTypeObject*>(ApiHandle::borrowedReference(
      runtime, runtime->typeAt(LayoutId::kTupleIterator)));
}

PY_EXPORT PyObject* PyTuple_New(Py_ssize_t length) {
  Runtime* runtime = Thread::current()->runtime();
  if (length == 0) {
    return ApiHandle::newReferenceWithManaged(runtime, runtime->emptyTuple());
  }
  RawMutableTuple tuple = MutableTuple::cast(runtime->newMutableTuple(length));
  tuple.fill(NoneType::object());
  return ApiHandle::newReferenceWithManaged(runtime, tuple.becomeImmutable());
}

PY_EXPORT int PyTuple_CheckExact_Func(PyObject* obj) {
  return ApiHandle::fromPyObject(obj)->asObject().isTuple();
}

PY_EXPORT int PyTuple_Check_Func(PyObject* obj) {
  return Thread::current()->runtime()->isInstanceOfTuple(
      ApiHandle::fromPyObject(obj)->asObject());
}

PY_EXPORT Py_ssize_t PyTuple_Size(PyObject* pytuple) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  Object tupleobj(&scope, ApiHandle::fromPyObject(pytuple)->asObject());
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfTuple(*tupleobj)) {
    thread->raiseBadInternalCall();
    return -1;
  }

  Tuple tuple(&scope, tupleUnderlying(*tupleobj));
  return tuple.length();
}

PY_EXPORT int PyTuple_SET_ITEM_Func(PyObject* pytuple, Py_ssize_t pos,
                                    PyObject* pyitem) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  Object tupleobj(&scope, ApiHandle::fromPyObject(pytuple)->asObject());
  Runtime* runtime = thread->runtime();
  Object newitem(&scope, pyitem == nullptr ? NoneType::object()
                                           : ApiHandle::stealReference(pyitem));
  if (!runtime->isInstanceOfTuple(*tupleobj)) {
    thread->raiseBadInternalCall();
    return -1;
  }

  Tuple tuple(&scope, tupleUnderlying(*tupleobj));
  if (pos < 0 || pos >= tuple.length()) {
    thread->raiseWithFmt(LayoutId::kIndexError,
                         "tuple assignment index out of range");
    return -1;
  }

  tuple.atPut(pos, *newitem);
  return 0;
}

PY_EXPORT int PyTuple_SetItem(PyObject* pytuple, Py_ssize_t pos,
                              PyObject* pyitem) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  Object tupleobj(&scope, ApiHandle::fromPyObject(pytuple)->asObject());
  Runtime* runtime = thread->runtime();
  Object newitem(&scope, pyitem == nullptr ? NoneType::object()
                                           : ApiHandle::stealReference(pyitem));
  if (!runtime->isInstanceOfTuple(*tupleobj)) {
    thread->raiseBadInternalCall();
    return -1;
  }

  Tuple tuple(&scope, tupleUnderlying(*tupleobj));
  if (pos < 0 || pos >= tuple.length()) {
    thread->raiseWithFmt(LayoutId::kIndexError,
                         "tuple assignment index out of range");
    return -1;
  }

  tuple.atPut(pos, *newitem);
  return 0;
}

PY_EXPORT PyTypeObject* PyTuple_Type_Ptr() {
  Runtime* runtime = Thread::current()->runtime();
  return reinterpret_cast<PyTypeObject*>(
      ApiHandle::borrowedReference(runtime, runtime->typeAt(LayoutId::kTuple)));
}

PY_EXPORT PyObject* PyTuple_GetItem(PyObject* pytuple, Py_ssize_t pos) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  Object tupleobj(&scope, ApiHandle::fromPyObject(pytuple)->asObject());
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfTuple(*tupleobj)) {
    return nullptr;
  }

  Tuple tuple(&scope, tupleUnderlying(*tupleobj));
  if (pos < 0 || pos >= tuple.length()) {
    return nullptr;
  }

  return ApiHandle::borrowedReference(runtime, tuple.at(pos));
}

PY_EXPORT PyObject* PyTuple_Pack(Py_ssize_t n, ...) {
  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  if (n == 0) {
    return ApiHandle::newReferenceWithManaged(runtime, runtime->emptyTuple());
  }

  HandleScope scope(thread);
  va_list vargs;
  va_start(vargs, n);
  MutableTuple tuple(&scope, runtime->newMutableTuple(n));
  for (Py_ssize_t i = 0; i < n; i++) {
    PyObject* item = va_arg(vargs, PyObject*);
    tuple.atPut(i, ApiHandle::fromPyObject(item)->asObject());
  }
  va_end(vargs);
  return ApiHandle::newReferenceWithManaged(runtime, tuple.becomeImmutable());
}

PY_EXPORT PyObject* PyTuple_GetSlice(PyObject* pytuple, Py_ssize_t low,
                                     Py_ssize_t high) {
  Thread* thread = Thread::current();
  if (pytuple == nullptr) {
    thread->raiseBadInternalCall();
    return nullptr;
  }
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Object tuple_obj(&scope, ApiHandle::fromPyObject(pytuple)->asObject());
  if (!runtime->isInstanceOfTuple(*tuple_obj)) {
    thread->raiseBadInternalCall();
    return nullptr;
  }
  Tuple tuple(&scope, tupleUnderlying(*tuple_obj));
  word len = tuple.length();
  if (low < 0) {
    low = 0;
  } else if (low > len) {
    low = len;
  }
  if (high < low) {
    high = low;
  } else if (high > len) {
    high = len;
  }
  if (low == 0 && high == len && tuple_obj.isTuple()) {
    return ApiHandle::newReferenceWithManaged(runtime, *tuple);
  }
  return ApiHandle::newReferenceWithManaged(
      runtime, runtime->tupleSubseq(thread, tuple, low, high - low));
}

}  // namespace py
