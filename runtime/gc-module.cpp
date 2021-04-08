#include "builtins.h"
#include "runtime.h"

namespace py {

RawObject FUNC(gc, immortalize_heap)(Thread* thread, Arguments) {
  thread->runtime()->immortalizeCurrentHeapObjects();
  return NoneType::object();
}

RawObject FUNC(gc, _is_immortal)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object obj(&scope, args.get(0));

  return Bool::fromBool(
      obj.isHeapObject() &&
      thread->runtime()->heap()->isImmortal(HeapObject::cast(*obj).address()));
}

}  // namespace py
