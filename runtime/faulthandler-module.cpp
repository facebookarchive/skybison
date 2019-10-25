#include "faulthandler-module.h"

#include "frozen-modules.h"
#include "int-builtins.h"
#include "runtime.h"
#include "sys-module.h"

namespace python {

const char* const FaulthandlerModule::kFrozenData = kFaulthandlerModuleData;

const BuiltinMethod FaulthandlerModule::kBuiltinMethods[] = {
    {SymbolId::kDumpTraceback, dumpTraceback},
    {SymbolId::kSentinelId, nullptr},
};

static RawObject getFileno(Thread* thread, const Object& file) {
  Runtime* runtime = thread->runtime();
  if (file.isNoneType()) {
    return SmallInt::fromWord(kStderrFd);
  }
  if (runtime->isInstanceOfInt(*file)) {
    RawInt fd = RawInt::cast(intUnderlying(thread, file));
    if (fd.isNegative() || fd.isLargeInt()) {
      return thread->raiseWithFmt(LayoutId::kValueError,
                                  "file is not a valid file descriptor");
    }
    return fd.isSmallInt() ? fd : convertBoolToInt(fd);
  }

  HandleScope scope(thread);
  Object fileno(&scope, thread->invokeMethod1(file, SymbolId::kFileno));
  if (fileno.isError()) {
    if (fileno.isErrorNotFound()) {
      return thread->raiseWithFmt(LayoutId::kAttributeError,
                                  "'%T' object has no attribute 'fileno'",
                                  &file);
    }
    return *fileno;
  }

  if (!runtime->isInstanceOfInt(*fileno)) {
    return thread->raiseWithFmt(LayoutId::kRuntimeError,
                                "file.fileno() is not a valid file descriptor");
  }
  Int fd(&scope, intUnderlying(thread, fileno));
  if (fd.isNegative() || fd.isLargeInt()) {
    return thread->raiseWithFmt(LayoutId::kRuntimeError,
                                "file.fileno() is not a valid file descriptor");
  }

  Object flush_result(&scope, thread->invokeMethod1(file, SymbolId::kFlush));
  if (flush_result.isErrorException()) {
    thread->clearPendingException();
  }
  return fd.isSmallInt() ? *fd : convertBoolToInt(*fd);
}

RawObject FaulthandlerModule::dumpTraceback(Thread* thread, Frame* frame,
                                            word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object file(&scope, args.get(0));
  Object all_threads(&scope, args.get(1));

  if (!thread->runtime()->isInstanceOfInt(*all_threads)) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "an integer is required (got type %T)",
                                &all_threads);
  }

  Object fileno(&scope, getFileno(thread, file));
  if (fileno.isError()) return *fileno;
  SmallInt fd(&scope, *fileno);
  Int all_threads_int(&scope, intUnderlying(thread, all_threads));
  Utils::printTracebackToFd(fd.value(), !all_threads_int.isZero());

  // TODO(wmeehan): call Pyro-equivalent to PyErr_CheckSignals
  return NoneType::object();
}

}  // namespace python
