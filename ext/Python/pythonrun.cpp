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
  return ApiHandle::fromObject(runtime->runFromCStr(str))->asPyObject();
}

}  // namespace python
