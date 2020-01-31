#include "cpython-data.h"

#include "capi-handles.h"
#include "runtime.h"

namespace py {

PY_EXPORT int PyMemoryView_Check_Func(PyObject* obj) {
  return ApiHandle::fromPyObject(obj)->asObject().isMemoryView();
}

PY_EXPORT PyObject* PyMemoryView_FromMemory(char* memory, Py_ssize_t size,
                                            int flags) {
  DCHECK(memory != nullptr, "memory must not be null");
  DCHECK(flags == PyBUF_READ || flags == PyBUF_WRITE,
         "flags must be either PyBUF_READ or PyBUF_WRITE");
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  MemoryView view(&scope, runtime->newMemoryViewFromCPtr(
                              thread, memory, size,
                              flags == PyBUF_READ ? ReadOnly::ReadOnly
                                                  : ReadOnly::ReadWrite));
  return ApiHandle::newReference(thread, *view);
}

PY_EXPORT PyObject* PyMemoryView_FromObject(PyObject* obj) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object object(&scope, ApiHandle::fromPyObject(obj)->asObject());
  Object result(&scope,
                thread->invokeFunction1(ID(builtins), ID(memoryview), object));
  if (result.isError()) {
    return nullptr;
  }
  return ApiHandle::newReference(thread, *result);
}

PY_EXPORT PyObject* PyMemoryView_GetContiguous(PyObject* /* j */, int /* e */,
                                               char /* r */) {
  UNIMPLEMENTED("PyMemoryView_GetContiguous");
}

}  // namespace py
