#include "under-valgrind-module.h"

#include "valgrind/callgrind.h"

#include "frame.h"
#include "frozen-modules.h"
#include "handles.h"
#include "modules.h"
#include "runtime.h"
#include "thread.h"

namespace py {

const BuiltinFunction UnderValgrindModule::kBuiltinFunctions[] = {
    {ID(callgrind_dump_stats), callgrindDumpStats},
    {ID(callgrind_start_instrumentation), callgrindStartInstrumentation},
    {ID(callgrind_stop_instrumentation), callgrindStopInstrumentation},
    {ID(callgrind_zero_stats), callgrindZeroStats},
    {SymbolId::kSentinelId, nullptr},
};

void UnderValgrindModule::initialize(Thread* thread, const Module& module) {
  moduleAddBuiltinFunctions(thread, module, kBuiltinFunctions);
  executeFrozenModule(thread, kUnderValgrindModuleData, module);
}

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
    return thread->raiseRequiresType(description, ID(str));
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
