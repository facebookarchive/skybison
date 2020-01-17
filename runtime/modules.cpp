#include "modules.h"

#include "faulthandler-module.h"
#include "frozen-modules.h"
#include "globals.h"
#include "marshal-module.h"
#include "runtime.h"
#include "under-codecs-module.h"
#include "under-imp-module.h"
#include "under-io-module.h"
#include "under-os-module.h"
#include "under-valgrind-module.h"
#include "under-warnings-module.h"
#include "under-weakref-module.h"

namespace py {

const BuiltinMethod ModuleBaseBase::kBuiltinMethods[] = {
    {SymbolId::kSentinelId, nullptr},
};
const BuiltinType ModuleBaseBase::kBuiltinTypes[] = {
    {SymbolId::kSentinelId, LayoutId::kSentinelId},
};

template <SymbolId Name, const char* Data>
static void initializeFrozenModule(Thread* thread) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Module module(&scope, runtime->createModule(thread, Name));
  runtime->executeFrozenModule(thread, Data, module);
}

const ModuleInitializer kBuiltinModules[] = {
    {SymbolId::kUnderCodecs, &UnderCodecsModule::initialize},
    {SymbolId::kUnderImp, &UnderImpModule::initialize},
    {SymbolId::kUnderOs, &UnderOsModule::initialize},
    {SymbolId::kUnderSignal,
     &initializeFrozenModule<SymbolId::kUnderSignal, kUnderSignalModuleData>},
    {SymbolId::kUnderWeakref, &UnderWeakrefModule::initialize},
    {SymbolId::kUnderThread,
     &initializeFrozenModule<SymbolId::kUnderThread, kUnderThreadModuleData>},
    {SymbolId::kUnderIo, &UnderIoModule::initialize},
    {SymbolId::kUnderStrMod,
     &initializeFrozenModule<SymbolId::kUnderStrMod,
                             kUnderStrUnderModModuleData>},
    {SymbolId::kUnderValgrind, &UnderValgrindModule::initialize},
    {SymbolId::kFaulthandler, &FaulthandlerModule::initialize},
    {SymbolId::kMarshal, &MarshalModule::initialize},
    {SymbolId::kUnderWarnings, &UnderWarningsModule::initialize},
    {SymbolId::kOperator,
     &initializeFrozenModule<SymbolId::kOperator, kOperatorModuleData>},
    {SymbolId::kWarnings,
     &initializeFrozenModule<SymbolId::kWarnings, kWarningsModuleData>},
    {SymbolId::kSentinelId, nullptr},
};

}  // namespace py
