#include "codecs-module.h"
#include "frame.h"
#include "frozen-modules.h"
#include "runtime.h"

namespace python {

const BuiltinMethod UnderCodecsModule::kBuiltinMethods[] = {
    {SymbolId::kSentinelId, nullptr},
};

const char* const UnderCodecsModule::kFrozenData = kUnderCodecsModuleData;

}  // namespace python
