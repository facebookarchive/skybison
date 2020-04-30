#include "under-signal-module.h"

#include <cerrno>
#include <csignal>

#include "builtins.h"
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

  Object sig_dfl(&scope, kDefaultHandler);
  moduleAtPutById(thread, module, ID(SIG_DFL), sig_dfl);

  Object sig_ign(&scope, kIgnoreHandler);
  moduleAtPutById(thread, module, ID(SIG_IGN), sig_ign);

  Object sigint(&scope, SmallInt::fromWord(SIGINT));
  moduleAtPutById(thread, module, ID(SIGINT), sigint);

  executeFrozenModule(thread, &kUnderSignalModuleData, module);

  thread->runtime()->initializeSignals(thread, module);
}

void handleSignal(int signum) {
  Thread* thread = Thread::current();
  int saved_errno = errno;
  thread->runtime()->setPendingSignal(thread, signum);
  errno = saved_errno;
}

RawObject FUNC(_signal, default_int_handler)(Thread* thread, Frame*, word) {
  return thread->raise(LayoutId::kKeyboardInterrupt, NoneType::object());
}

}  // namespace py
