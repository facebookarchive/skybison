#pragma once

#include "objects.h"
#include "runtime.h"

namespace python {

class Frame;
class Thread;

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

}  // namespace python
