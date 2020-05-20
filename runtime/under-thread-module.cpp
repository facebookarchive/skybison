#include "under-thread-module.h"

#include "frozen-modules.h"
#include "globals.h"
#include "handles.h"
#include "module-builtins.h"
#include "runtime.h"
#include "thread.h"

namespace py {

void initializeUnderThreadModule(Thread* thread, const Module& module) {
  executeFrozenModule(thread, &kUnderThreadModuleData, module);
}

}  // namespace py
