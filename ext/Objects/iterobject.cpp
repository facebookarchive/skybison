#include "runtime.h"

namespace python {

PY_EXPORT PyObject* PySeqIter_New(PyObject* seq) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Object seq_obj(&scope, ApiHandle::fromPyObject(seq)->asObject());
  Runtime* runtime = thread->runtime();
  if (!runtime->isSequence(thread, seq_obj)) {
    thread->raiseBadInternalCall();
    return nullptr;
  }
  return ApiHandle::newReference(thread, runtime->newSeqIterator(seq_obj));
}

PY_EXPORT PyObject* PyCallIter_New(PyObject* /* e */, PyObject* /* l */) {
  UNIMPLEMENTED("PyCallIter_New");
}

PY_EXPORT int PyIter_Check_Func(PyObject* iter) {
  return ApiHandle::fromPyObject(iter)->asObject().isSeqIterator();
}

}  // namespace python
