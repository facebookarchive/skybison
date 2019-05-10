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
  Frame* frame = thread->currentFrame();
  Object fspath_method(&scope,
                       Interpreter::lookupMethod(thread, frame, path_obj,
                                                 SymbolId::kDunderFspath));
  if (fspath_method.isError()) {
    thread->raiseWithFmt(LayoutId::kTypeError,
                         "expected str, bytes or os.PathLike object.");
    return nullptr;
  }
  Object fspath(
      &scope, Interpreter::callMethod1(thread, frame, fspath_method, path_obj));
  if (fspath.isError()) return nullptr;

  if (!runtime->isInstanceOfStr(*fspath) &&
      !runtime->isInstanceOfBytes(*fspath)) {
    thread->raiseWithFmt(LayoutId::kTypeError,
                         "expected __fspath__ to return str or bytes");
    return nullptr;
  }
  return ApiHandle::newReference(thread, *fspath);
}

}  // namespace python
