#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"

namespace python {

void importAcquireLock(Thread* thread);
bool importReleaseLock(Thread* thread);

class UnderImpModule : public ModuleBase<UnderImpModule, SymbolId::kUnderImp> {
 public:
  static RawObject acquireLock(Thread* thread, Frame* frame, word nargs);
  static RawObject createBuiltin(Thread* thread, Frame* frame, word nargs);
  static RawObject execBuiltin(Thread* thread, Frame* frame, word nargs);
  static RawObject execDynamic(Thread* thread, Frame* frame, word nargs);
  static RawObject extensionSuffixes(Thread* thread, Frame* frame, word nargs);
  static RawObject fixCoFilename(Thread* thread, Frame* frame, word nargs);
  static RawObject getFrozenObject(Thread* thread, Frame* frame, word nargs);
  static RawObject isBuiltin(Thread* thread, Frame* frame, word nargs);
  static RawObject isFrozen(Thread* thread, Frame* frame, word nargs);
  static RawObject isFrozenPackage(Thread* thread, Frame* frame, word nargs);
  static RawObject releaseLock(Thread* thread, Frame* frame, word nargs);

  static const BuiltinMethod kBuiltinMethods[];
  static const char* const kFrozenData;
};

}  // namespace python
