#include "under-weakref-module.h"

#include "builtins.h"
#include "frame.h"
#include "frozen-modules.h"
#include "globals.h"
#include "modules.h"
#include "objects.h"

namespace py {

const BuiltinType UnderWeakrefModule::kBuiltinTypes[] = {
    {ID(ref), LayoutId::kWeakRef},
    {SymbolId::kSentinelId, LayoutId::kSentinelId},
};

void UnderWeakrefModule::initialize(Thread* thread, const Module& module) {
  moduleAddBuiltinTypes(thread, module, kBuiltinTypes);
  executeFrozenModule(thread, &kUnderWeakrefModuleData, module);
}

RawObject FUNC(_weakref, _weakref_hash)(Thread* thread, Frame* frame,
                                        word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfWeakRef(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(ref));
  }
  WeakRef self(&scope, args.get(0));
  return self.hash();
}

RawObject FUNC(_weakref, _weakref_set_hash)(Thread* thread, Frame* frame,
                                            word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  WeakRef self(&scope, args.get(0));
  self.setHash(args.get(1));
  return NoneType::object();
}

}  // namespace py
