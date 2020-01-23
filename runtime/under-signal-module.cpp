#include "under-signal-module.h"

#include "frozen-modules.h"
#include "modules.h"
#include "runtime.h"
#include "symbols.h"

namespace py {

const BuiltinFunction UnderSignalModule::kBuiltinFunctions[] = {
    {SymbolId::kSentinelId, nullptr},
};

}  // namespace py
