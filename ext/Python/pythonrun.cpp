#include "runtime.h"

namespace python {

namespace testing {
Object* findModule(Runtime*, const char*);
Object* moduleAt(Runtime*, const Handle<Module>&, const char*);
}  // namespace testing

extern "C" PyObject* PyRun_SimpleStringFlags(const char* str,
                                             PyCompilerFlags* flags) {
  // TODO: Implement the usage of flags
  if (flags != nullptr) {
    UNIMPLEMENTED("Can't specify compiler flags");
  }
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  return ApiHandle::fromObject(runtime->runFromCString(str))->asPyObject();
}

extern "C" PyObject* _PyModuleGet(const char* module, const char* name) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Handle<Module> mod(&scope, testing::findModule(runtime, module));
  Handle<Object> var(&scope, testing::moduleAt(runtime, mod, name));
  return ApiHandle::fromObject(*var)->asPyObject();
}

}  // namespace python
