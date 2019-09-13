#include "frame.h"

#include <cstring>

#include "handles.h"
#include "objects.h"
#include "runtime.h"

namespace python {

const char* Frame::isInvalid() {
  if (!at(kPreviousFrameOffset).isSmallInt()) {
    return "bad previousFrame field";
  }
  if (!isSentinel() && !(locals() + 1)->isFunction()) {
    return "bad function";
  }
  return nullptr;
}

RawObject frameGlobals(Thread* thread, Frame* frame) {
  HandleScope scope(thread);
  // TODO(T36407403): avoid a reverse mapping by reading the module directly
  // out of the function object or the frame.
  Object name(&scope, frame->function().module());
  Object hash(&scope, Interpreter::hash(thread, name));
  if (hash.isErrorException()) return *hash;

  Runtime* runtime = thread->runtime();
  Dict modules(&scope, runtime->modules());
  Object module_obj(&scope, runtime->dictAt(thread, modules, name, hash));
  if (module_obj.isErrorNotFound() ||
      !runtime->isInstanceOfModule(*module_obj)) {
    UNIMPLEMENTED("modules not registered in sys.modules");
  }
  Module module(&scope, *module_obj);
  return module.moduleProxy();
}

}  // namespace python
