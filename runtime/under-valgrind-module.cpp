#include "under-valgrind-module.h"

#include "valgrind/callgrind.h"

#include "frame.h"
#include "frozen-modules.h"
#include "handles.h"
#include "runtime.h"
#include "thread.h"

namespace py {

const BuiltinMethod UnderValgrindModule::kBuiltinMethods[] = {
    {SymbolId::kCallgrindDumpStats, callgrindDumpStats},
    {SymbolId::kCallgrindStartInstrumentation, callgrindStartInstrumentation},
    {SymbolId::kCallgrindStopInstrumentation, callgrindStopInstrumentation},
    {SymbolId::kCallgrindZeroStats, callgrindZeroStats},
    {SymbolId::kSentinelId, nullptr},
};

const char* const UnderValgrindModule::kFrozenData = kUnderValgrindModuleData;

RawObject UnderValgrindModule::callgrindDumpStats(Thread* thread, Frame* frame,
                                                  word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object description(&scope, args.get(0));
  if (description.isNoneType()) {
    CALLGRIND_DUMP_STATS;
    return NoneType::object();
  }
  if (!thread->runtime()->isInstanceOfStr(*description)) {
    return thread->raiseRequiresType(description, SymbolId::kStr);
  }
  Str description_str(&scope, strUnderlying(*description));
  unique_c_ptr<char> description_cstr(description_str.toCStr());
  CALLGRIND_DUMP_STATS_AT(description_cstr.get());
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

}  // namespace py
