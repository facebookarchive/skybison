#include "capi-testing.h"

#include "test-utils.h"

namespace python {
namespace testing {

int exceptionMessageMatches(const char* message) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);

  Handle<Object> exception_obj(&scope, thread->pendingException());
  if (exception_obj->isNone()) {
    return 0;
  }
  if (!exception_obj->isStr()) {
    UNIMPLEMENTED("Handle non string exception objects");
  }
  Handle<Str> exception(&scope, *exception_obj);

  return exception->equalsCStr(message);
}

PyObject* moduleGet(const char* module, const char* name) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Handle<Module> mod(&scope, findModule(runtime, module));
  Handle<Object> var(&scope, moduleAt(runtime, mod, name));
  return ApiHandle::fromObject(*var)->asPyObject();
}

int isBorrowed(PyObject* pyobj) {
  return ApiHandle::fromPyObject(pyobj)->isBorrowed();
}

}  // namespace testing
}  // namespace python
