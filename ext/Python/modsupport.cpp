#include "handles.h"
#include "objects.h"
#include "runtime.h"

namespace python {

extern "C" int PyModule_AddIntConstant(PyObject* pymodule, const char* name,
                                       long value) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Handle<Object> integer(&scope, runtime->newInteger(value));
  if (!integer->isInteger()) {
    // TODO(cshapiro): throw a MemoryError
    return -1;
  }
  Handle<Object> module_obj(&scope,
                            ApiHandle::fromPyObject(pymodule)->asObject());
  if (!module_obj->isModule()) {
    // TODO(cshapiro): throw a TypeError
    return -1;
  }
  if (name == nullptr) {
    // TODO(cshapiro): throw a TypeError
    return -1;
  }
  Handle<Object> key(&scope, runtime->newStringFromCString(name));
  if (!key->isString()) {
    // TODO(cshapiro): throw a MemoryError
    return -1;
  }
  Handle<Module> module(&scope, *module_obj);
  runtime->moduleAtPut(module, key, integer);
  return 0;
}

}  // namespace python
