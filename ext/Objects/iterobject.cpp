#include "capi-handles.h"
#include "runtime.h"

namespace py {

PY_EXPORT PyObject* PySeqIter_New(PyObject* seq) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object seq_obj(&scope, ApiHandle::fromPyObject(seq)->asObject());
  Runtime* runtime = thread->runtime();
  if (!runtime->isSequence(thread, seq_obj)) {
    thread->raiseBadInternalCall();
    return nullptr;
  }
  return ApiHandle::newReference(runtime, runtime->newSeqIterator(seq_obj));
}

PY_EXPORT PyTypeObject* PySeqIter_Type_Ptr() {
  Runtime* runtime = Thread::current()->runtime();
  return reinterpret_cast<PyTypeObject*>(ApiHandle::borrowedReference(
      runtime, runtime->typeAt(LayoutId::kSeqIterator)));
}

PY_EXPORT PyObject* PyCallIter_New(PyObject* pycallable, PyObject* pysentinel) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object callable(&scope, ApiHandle::fromPyObject(pycallable)->asObject());
  Object sentinel(&scope, ApiHandle::fromPyObject(pysentinel)->asObject());
  Object result(&scope,
                thread->invokeFunction2(ID(builtins), ID(callable_iterator),
                                        callable, sentinel));
  if (result.isErrorException()) return nullptr;
  return ApiHandle::newReference(thread->runtime(), *result);
}

PY_EXPORT int PyIter_Check_Func(PyObject* iter) {
  DCHECK(iter != nullptr, "expected iter to be non-null");
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object iterator(&scope, ApiHandle::fromPyObject(iter)->asObject());
  return thread->runtime()->isIterator(thread, iterator);
}

}  // namespace py
