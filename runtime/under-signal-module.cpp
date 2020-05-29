#include "under-signal-module.h"

#include <cerrno>
#include <csignal>

#include "builtins.h"
#include "module-builtins.h"
#include "modules.h"
#include "os.h"
#include "runtime.h"
#include "symbols.h"
#include "type-builtins.h"

namespace py {

void FUNC(_signal, __init_module__)(Thread* thread, const Module& module,
                                    View<byte> bytecode) {
  HandleScope scope(thread);
  Object nsig(&scope, SmallInt::fromWord(OS::kNumSignals));
  moduleAtPutById(thread, module, ID(NSIG), nsig);

  Object sig_dfl(&scope, kDefaultHandler);
  moduleAtPutById(thread, module, ID(SIG_DFL), sig_dfl);

  Object sig_ign(&scope, kIgnoreHandler);
  moduleAtPutById(thread, module, ID(SIG_IGN), sig_ign);

  Object signum(&scope, NoneType::object());
  for (const OS::Signal* signal = OS::kStandardSignals; signal->name != nullptr;
       signal++) {
    signum = SmallInt::fromWord(signal->signum);
    moduleAtPutByCStr(thread, module, signal->name, signum);
  }
  for (const OS::Signal* signal = OS::kPlatformSignals; signal->name != nullptr;
       signal++) {
    signum = SmallInt::fromWord(signal->signum);
    moduleAtPutByCStr(thread, module, signal->name, signum);
  }

  executeFrozenModule(thread, module, bytecode);

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

RawObject FUNC(_signal, getsignal)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Runtime* runtime = thread->runtime();
  Object obj(&scope, args.get(0));
  if (!runtime->isInstanceOfInt(*obj)) {
    return thread->raiseRequiresType(obj, ID(int));
  }
  word signum = intUnderlying(*obj).asWord();
  if (signum < 1 || signum >= OS::kNumSignals) {
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "signal number out of range");
  }
  return runtime->signalCallback(signum);
}

RawObject FUNC(_signal, signal)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Runtime* runtime = thread->runtime();
  Object obj(&scope, args.get(0));
  if (!runtime->isInstanceOfInt(*obj)) {
    return thread->raiseRequiresType(obj, ID(int));
  }

  if (!thread->isMainThread()) {
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "signal only works in main thread");
  }

  word signum = intUnderlying(*obj).asWord();
  if (signum < 1 || signum >= OS::kNumSignals) {
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "signal number out of range");
  }

  Object callback(&scope, args.get(1));
  SignalHandler handler;
  if (callback == kIgnoreHandler) {
    handler = SIG_IGN;
  } else if (callback == kDefaultHandler) {
    handler = SIG_DFL;
  } else {
    Type type(&scope, runtime->typeOf(*callback));
    if (typeLookupInMroById(thread, type, ID(__call__)).isErrorNotFound()) {
      return thread->raiseWithFmt(LayoutId::kTypeError,
                                  "signal handler must be signal.SIG_IGN, "
                                  "signal.SIG_DFL, or a callable object");
    }
    handler = handleSignal;
  }

  Object err(&scope, runtime->handlePendingSignals(thread));
  if (err.isErrorException()) {
    return *err;
  }
  if (OS::setSignalHandler(signum, handler) == SIG_ERR) {
    return thread->raise(LayoutId::kOSError, NoneType::object());
  }
  return runtime->setSignalCallback(signum, callback);
}

}  // namespace py
