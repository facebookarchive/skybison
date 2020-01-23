#pragma once

#include "frame.h"
#include "globals.h"
#include "modules.h"
#include "objects.h"
#include "runtime.h"

namespace py {

void importAcquireLock(Thread* thread);
bool importReleaseLock(Thread* thread);
RawObject createExtensionModule(Thread* thread, const Str& name);

class UnderImpModule {
 public:
  static void initialize(Thread* thread, const Module& module);

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
  static RawObject underCreateDynamic(Thread* thread, Frame* frame, word nargs);

 private:
  static const BuiltinFunction kBuiltinFunctions[];
};

}  // namespace py
