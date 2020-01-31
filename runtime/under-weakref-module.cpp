#include "under-weakref-module.h"

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
  executeFrozenModule(thread, kUnderWeakrefModuleData, module);
}

}  // namespace py
