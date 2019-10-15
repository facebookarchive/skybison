#include "under-valgrind-module.h"

#include "valgrind/callgrind.h"

#include "frame.h"
#include "frozen-modules.h"
#include "handles.h"
#include "int-builtins.h"
#include "runtime.h"
#include "thread.h"

namespace python {

const BuiltinMethod UnderValgrindModule::kBuiltinMethods[] = {
    {SymbolId::kCallgrindDumpStatsAt, callgrindDumpStatsAt},
    {SymbolId::kCallgrindStartInstrumentation, callgrindStartInstrumentation},
    {SymbolId::kCallgrindStopInstrumentation, callgrindStopInstrumentation},
    {SymbolId::kCallgrindZeroStats, callgrindZeroStats},
    {SymbolId::kSentinelId, nullptr},
};

const char* const UnderValgrindModule::kFrozenData = kUnderValgrindModuleData;

RawObject UnderValgrindModule::callgrindDumpStatsAt(Thread* thread,
                                                    Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  if (!args.get(0).isStr()) {
    CALLGRIND_DUMP_STATS;
    return NoneType::object();
  }
  HandleScope scope(thread);
  Str pos_str(&scope, args.get(0));
  byte buf[128] = {0};
  pos_str.copyTo(buf, 127 < pos_str.charLength() ? 127 : pos_str.charLength());
  CALLGRIND_DUMP_STATS_AT(buf);
  return NoneType::object();
}

RawObject UnderValgrindModule::callgrindStartInstrumentation(Thread*, Frame*,
                                                             word) {
  CALLGRIND_START_INSTRUMENTATION;
  return NoneType::object();
}

RawObject UnderValgrindModule::callgrindStopInstrumentation(Thread*, Frame*,
                                                            word) {
  CALLGRIND_STOP_INSTRUMENTATION;
  return NoneType::object();
}

RawObject UnderValgrindModule::callgrindZeroStats(Thread*, Frame*, word) {
  CALLGRIND_ZERO_STATS;
  return NoneType::object();
}

}  // namespace python
