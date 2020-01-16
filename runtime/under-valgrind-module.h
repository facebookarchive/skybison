#pragma once

#include "frame.h"
#include "modules.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace py {

class UnderValgrindModule
    : public ModuleBase<UnderValgrindModule, SymbolId::kUnderValgrind> {
 public:
  static RawObject callgrindDumpStats(Thread* thread, Frame* frame, word nargs);
  static RawObject callgrindStartInstrumentation(Thread* thread, Frame* frame,
                                                 word nargs);
  static RawObject callgrindStopInstrumentation(Thread* thread, Frame* frame,
                                                word nargs);
  static RawObject callgrindZeroStats(Thread* thread, Frame* frame, word nargs);

  static const BuiltinMethod kBuiltinMethods[];
  static const char* const kFrozenData;
};

}  // namespace py
