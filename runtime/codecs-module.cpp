#include "codecs-module.h"
#include "frame.h"
#include "frozen-modules.h"
#include "runtime.h"

namespace python {

const BuiltinMethod UnderCodecsModule::kBuiltinMethods[] = {
    {SymbolId::kSentinelId, nullptr},
};

void UnderCodecsModule::postInitialize(Thread*, Runtime* runtime,
                                       const Module& module) {
  CHECK(!runtime->executeModule(kUnderCodecsModuleData, module).isError(),
        "Failed to initialize _codecs module");
}

}  // namespace python
