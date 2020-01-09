#include "under-signal-module.h"

#include "frozen-modules.h"
#include "runtime.h"
#include "symbols.h"

namespace py {

const BuiltinMethod UnderSignalModule::kBuiltinMethods[] = {
    {SymbolId::kSentinelId, nullptr},
};

const char* const UnderSignalModule::kFrozenData = kUnderSignalModuleData;

}  // namespace py
