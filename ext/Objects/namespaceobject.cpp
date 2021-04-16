#include "api-handle.h"
#include "bytecode.h"
#include "dict-builtins.h"
#include "interpreter.h"
#include "runtime.h"

namespace py {

PY_EXPORT PyObject* _PyNamespace_New(PyObject* kwds) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object result(&scope, NoneType::object());
  if (kwds == nullptr) {
    result = thread->invokeFunction0(ID(builtins), ID(SimpleNamespace));
  } else {
    Object type(&scope, runtime->lookupNameInModule(thread, ID(builtins),
                                                    ID(SimpleNamespace)));
    thread->stackPush(*type);
    thread->stackPush(runtime->emptyTuple());
    thread->stackPush(ApiHandle::fromPyObject(kwds)->asObject());
    result = Interpreter::callEx(thread, CallFunctionExFlag::VAR_KEYWORDS);
  }
  if (result.isError()) {
    return nullptr;
  }
  return ApiHandle::newReference(runtime, *result);
}

}  // namespace py
