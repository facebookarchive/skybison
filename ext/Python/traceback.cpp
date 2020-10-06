#include "capi-handles.h"
#include "runtime.h"

namespace py {

struct PyFrameObject;

PY_EXPORT int PyTraceBack_Check_Func(PyObject* obj) {
  return ApiHandle::fromPyObject(obj)->asObject().isTraceback();
}

PY_EXPORT int PyTraceBack_Here(PyFrameObject* frame) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  FrameProxy proxy(
      &scope,
      ApiHandle::fromPyObject(reinterpret_cast<PyObject*>(frame))->asObject());
  Traceback new_tb(&scope, thread->runtime()->newTraceback());
  new_tb.setFunction(proxy.function());
  new_tb.setLasti(proxy.lasti());
  new_tb.setNext(thread->pendingExceptionTraceback());
  thread->setPendingExceptionTraceback(*new_tb);
  return 0;
}

PY_EXPORT int PyTraceBack_Print(PyObject* /* v */, PyObject* /* f */) {
  UNIMPLEMENTED("PyTraceBack_Print");
}

PY_EXPORT void _PyTraceback_Add(const char* /* funcname */,
                                const char* /* filename */, int /* lineno */) {
  UNIMPLEMENTED("_PyTraceback_Add");
}

}  // namespace py
