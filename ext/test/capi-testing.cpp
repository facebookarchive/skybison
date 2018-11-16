#include "capi-testing.h"

#include "test-utils.h"

namespace python {
namespace testing {

bool exceptionMessageMatches(const char* message) {
  Thread* thread = Thread::currentThread();
  if (!thread->hasPendingException()) {
    return false;
  }
  HandleScope scope(thread);
  Handle<Object> value(&scope, thread->exceptionValue());
  if (!value->isStr()) {
    UNIMPLEMENTED("Handle non string exception objects");
  }
  Handle<Str> exception(&scope, *value);
  return exception->equalsCStr(message);
}

PyObject* moduleGet(const char* module, const char* name) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Handle<Module> mod(&scope, findModule(runtime, module));
  Handle<Object> var(&scope, moduleAt(runtime, mod, name));
  return ApiHandle::fromObject(*var);
}

bool isBorrowed(PyObject* pyobj) {
  return ApiHandle::fromPyObject(pyobj)->isBorrowed();
}

}  // namespace testing
}  // namespace python
