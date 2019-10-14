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
  byte buf[128];
  pos_str.copyTo(buf, 127 < pos_str.charLength() ? 127 : pos_str.charLength());
  buf[127] = 0;
  CALLGRIND_DUMP_STATS_AT(buf);
  return NoneType::object();
}

}  // namespace python
