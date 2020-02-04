#include "under-signal-module.h"

#include <signal.h>

#include "frozen-modules.h"
#include "module-builtins.h"
#include "modules.h"
#include "os.h"
#include "runtime.h"
#include "symbols.h"

namespace py {

void UnderSignalModule::initialize(Thread* thread, const Module& module) {
  HandleScope scope(thread);
  Object nsig(&scope, SmallInt::fromWord(OS::kNumSignals));
  moduleAtPutById(thread, module, ID(NSIG), nsig);

  Object sig_dfl(&scope, SmallInt::fromWord(reinterpret_cast<word>(SIG_DFL)));
  moduleAtPutById(thread, module, ID(SIG_DFL), sig_dfl);

  Object sig_ign(&scope, SmallInt::fromWord(reinterpret_cast<word>(SIG_IGN)));
  moduleAtPutById(thread, module, ID(SIG_IGN), sig_ign);

  executeFrozenModule(thread, &kUnderSignalModuleData, module);
}

}  // namespace py
