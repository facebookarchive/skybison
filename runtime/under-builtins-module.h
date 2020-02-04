#pragma once

#include "frame.h"
#include "globals.h"
#include "handles.h"
#include "modules.h"
#include "objects.h"
#include "runtime.h"

namespace py {

// Copy the code, entry, and interpreter info from base to patch.
void copyFunctionEntries(Thread* thread, const Function& base,
                         const Function& patch);

RawObject FUNC(_builtins, _patch)(Thread* thread, Frame* frame, word nargs);

class UnderBuiltinsModule {
 public:
  static void initialize(Thread* thread, const Module& module);

 private:
  static const SymbolId kIntrinsicIds[];
};

}  // namespace py
