#include "capi-handles.h"
#include "runtime.h"

namespace python {

PY_EXPORT PyObject* PyOS_FSPath(PyObject* path) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object path_obj(&scope, ApiHandle::fromPyObject(path)->asObject());
  Runtime* runtime = thread->runtime();
  if (runtime->isInstanceOfStr(*path_obj) ||
      runtime->isInstanceOfBytes(*path_obj)) {
    ApiHandle* handle = ApiHandle::fromPyObject(path);
    handle->incref();
    return handle;
  }
  Object result(&scope,
                thread->invokeFunction1(SymbolId::kUnderIo,
                                        SymbolId::kUnderFspath, path_obj));
  if (result.isError()) {
    CHECK(result.isErrorException(), "there was a problem calling _io._fspath");
    return nullptr;
  }
  return ApiHandle::newReference(thread, *result);
}

}  // namespace python
