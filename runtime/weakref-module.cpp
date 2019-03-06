#include "weakref-module.h"

#include "frame.h"
#include "frozen-modules.h"
#include "globals.h"
#include "objects.h"

namespace python {

const BuiltinType UnderWeakrefModule::kBuiltinTypes[] = {
    {SymbolId::kRef, LayoutId::kWeakRef},
    {SymbolId::kSentinelId, LayoutId::kSentinelId},
};

void UnderWeakrefModule::postInitialize(Thread*, Runtime* runtime,
                                        const Module& module) {
  CHECK(!runtime->executeModule(kUnderWeakrefModuleData, module).isError(),
        "Failed to initialize _weakref module");
}

}  // namespace python
