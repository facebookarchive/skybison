#include "weakref-module.h"

#include "frame.h"
#include "frozen-modules.h"
#include "globals.h"
#include "objects.h"

namespace py {

const BuiltinType UnderWeakrefModule::kBuiltinTypes[] = {
    {SymbolId::kRef, LayoutId::kWeakRef},
    {SymbolId::kSentinelId, LayoutId::kSentinelId},
};

const char* const UnderWeakrefModule::kFrozenData = kUnderWeakrefModuleData;

}  // namespace py
