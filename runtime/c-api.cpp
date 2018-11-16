#include "c-api.h"

#include "runtime.h"

namespace python {

bool Type_IsBuiltin(PyObject*);

Object* ApiHandle::asObject() {
  // Fast path
  if (reference_ != nullptr) {
    return static_cast<Object*>(reference_);
  }

  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();

  // Create runtime instance from extension object
  if (type() && !Type_IsBuiltin(type())) {
    return runtime->newExtensionInstance(this);
  }

  UNIMPLEMENTED("Could not materialize a runtime object from the ApiHandle");
}

} // namespace python
