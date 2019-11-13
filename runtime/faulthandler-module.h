#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"

namespace py {

class FaulthandlerModule
    : public ModuleBase<FaulthandlerModule, SymbolId::kFaulthandler> {
 public:
  static RawObject underReadNull(Thread* thread, Frame* frame, word nargs);
  static RawObject underSigabrt(Thread* thread, Frame* frame, word nargs);
  static RawObject underSigfpe(Thread* thread, Frame* frame, word nargs);
  static RawObject underSigsegv(Thread* thread, Frame* frame, word nargs);
  static RawObject disable(Thread* thread, Frame* frame, word nargs);
  static RawObject dumpTraceback(Thread* thread, Frame* frame, word nargs);
  static RawObject enable(Thread* thread, Frame* frame, word nargs);
  static RawObject isEnabled(Thread* thread, Frame* frame, word nargs);

  static const BuiltinMethod kBuiltinMethods[];
  static const char* const kFrozenData;
};

}  // namespace py
