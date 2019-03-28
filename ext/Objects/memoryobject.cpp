#include "runtime.h"

namespace python {

PY_EXPORT int PyMemoryView_Check_Func(PyObject* obj) {
  return ApiHandle::fromPyObject(obj)->asObject().isMemoryView();
}

PY_EXPORT PyObject* PyMemoryView_FromMemory(char* /* m */, Py_ssize_t /* e */,
                                            int /* s */) {
  UNIMPLEMENTED("PyMemoryView_FromMemory");
}

PY_EXPORT PyObject* PyMemoryView_FromObject(PyObject* obj) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object object(&scope, ApiHandle::fromPyObject(obj)->asObject());
  Object result(&scope, thread->invokeFunction1(SymbolId::kBuiltins,
                                                SymbolId::kMemoryView, object));
  if (result.isError()) {
    return nullptr;
  }
  return ApiHandle::newReference(thread, *result);
}

PY_EXPORT PyObject* PyMemoryView_GetContiguous(PyObject* /* j */, int /* e */,
                                               char /* r */) {
  UNIMPLEMENTED("PyMemoryView_GetContiguous");
}

}  // namespace python
