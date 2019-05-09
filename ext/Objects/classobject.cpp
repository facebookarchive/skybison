#include "runtime.h"

namespace python {

PY_EXPORT PyObject* PyInstanceMethod_New(PyObject* /* c */) {
  UNIMPLEMENTED("PyInstanceMethod_New");
}

PY_EXPORT int PyMethod_ClearFreeList() { return 0; }

PY_EXPORT PyObject* PyMethod_New(PyObject* callable, PyObject* self) {
  DCHECK(callable != nullptr, "callable must be initialized");
  Thread* thread = Thread::current();
  if (self == nullptr) {
    thread->raiseBadInternalCall();
    return nullptr;
  }
  HandleScope scope(thread);
  Object callable_obj(&scope, ApiHandle::fromPyObject(callable)->asObject());
  Object self_obj(&scope, ApiHandle::fromPyObject(self)->asObject());
  Object result(&scope,
                thread->runtime()->newBoundMethod(callable_obj, self_obj));
  return ApiHandle::newReference(thread, *result);
}

}  // namespace python
