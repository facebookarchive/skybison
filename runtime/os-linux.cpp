#include "os.h"

#include <dlfcn.h>
#include <signal.h>
#include <unistd.h>

#include <cstdlib>

#include "utils.h"

namespace py {

const word OS::kNumSignals = _NSIG;

const int OS::kRtldGlobal = RTLD_GLOBAL;
const int OS::kRtldLocal = RTLD_LOCAL;
const int OS::kRtldNow = RTLD_NOW;

const char* OS::name() { return "linux"; }

char* OS::executablePath() {
  char* buffer = readLink("/proc/self/exe");
  CHECK(buffer != nullptr, "failed to determine executable path");
  return buffer;
}

void* OS::openSharedObject(const char* filename, int mode,
                           const char** error_msg) {
  void* result = ::dlopen(filename, mode);
  if (result == nullptr) {
    *error_msg = ::dlerror();
  }
  return result;
}

SignalHandler OS::setSignalHandler(int signum, SignalHandler handler) {
  struct sigaction new_context, old_context;
  new_context.sa_handler = handler;
  sigemptyset(&new_context.sa_mask);
  new_context.sa_flags = 0;
  if (::sigaction(signum, &new_context, &old_context) == -1) {
    return SIG_ERR;
  }
  return old_context.sa_handler;
}

SignalHandler OS::signalHandler(int signum) {
  struct sigaction context;
  if (::sigaction(signum, nullptr, &context) == -1) {
    return SIG_ERR;
  }
  return context.sa_handler;
}

void* OS::sharedObjectSymbolAddress(void* handle, const char* symbol,
                                    const char** error_msg) {
  void* result = ::dlsym(handle, symbol);
  if (result == nullptr && error_msg != nullptr) {
    *error_msg = ::dlerror();
  }
  return result;
}

}  // namespace py
