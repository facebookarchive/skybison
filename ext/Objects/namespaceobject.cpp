#include "capi-handles.h"
#include "dict-builtins.h"
#include "runtime.h"

namespace py {

PY_EXPORT PyObject* _PyNamespace_New(PyObject* kwds) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object dict_obj(&scope, NoneType::object());
  if (kwds != nullptr) {
    dict_obj = ApiHandle::fromPyObject(kwds)->asObject();
    if (!thread->runtime()->isInstanceOfDict(*dict_obj)) {
      thread->raiseBadInternalCall();
      return nullptr;
    }
  }
  Object result(&scope,
                thread->invokeFunction1(SymbolId::kBuiltins,
                                        SymbolId::kSimpleNamespace, dict_obj));
  if (result.isError()) {
    return nullptr;
  }
  return ApiHandle::newReference(thread, *result);
}

}  // namespace py
