#include "builtins.h"
#include "frame.h"
#include "globals.h"
#include "modules.h"
#include "objects.h"
#include "runtime.h"

namespace py {

RawObject FUNC(_weakref, _weakref_hash)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfWeakRef(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(weakref));
  }
  WeakRef self(&scope, args.get(0));
  return self.hash();
}

RawObject FUNC(_weakref, _weakref_set_hash)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  WeakRef self(&scope, args.get(0));
  self.setHash(args.get(1));
  return NoneType::object();
}

}  // namespace py
