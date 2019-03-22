#include "time-module.h"

#include "frame.h"
#include "frozen-modules.h"
#include "globals.h"
#include "objects.h"
#include "os.h"
#include "runtime.h"
#include "thread.h"

namespace python {

const BuiltinMethod TimeModule::kBuiltinMethods[] = {
    {SymbolId::kTime, time},
    {SymbolId::kSentinelId, nullptr},
};

const char* const TimeModule::kFrozenData = kTimeModuleData;

RawObject TimeModule::time(Thread* thread, Frame*, word) {
  return thread->runtime()->newFloat(OS::currentTime());
}

}  // namespace python
