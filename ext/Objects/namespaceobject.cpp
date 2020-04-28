#include "bytecode.h"
#include "capi-handles.h"
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
    Frame* frame = thread->currentFrame();
    frame->pushValue(*type);
    frame->pushValue(runtime->emptyTuple());
    frame->pushValue(ApiHandle::fromPyObject(kwds)->asObject());
    result =
        Interpreter::callEx(thread, frame, CallFunctionExFlag::VAR_KEYWORDS);
  }
  if (result.isError()) {
    return nullptr;
  }
  return ApiHandle::newReference(thread, *result);
}

}  // namespace py
