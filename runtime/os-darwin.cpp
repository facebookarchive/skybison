#include "os.h"

#include <dlfcn.h>
#include <mach-o/dyld.h>
#include <signal.h>

#include <cstdint>
#include <cstdlib>

#include "utils.h"

namespace py {

const word OS::kNumSignals = NSIG;

const int OS::kRtldGlobal = RTLD_GLOBAL;
const int OS::kRtldLocal = RTLD_LOCAL;
const int OS::kRtldNow = RTLD_NOW;

const char* OS::name() { return "darwin"; }

char* OS::executablePath() {
  uint32_t buf_len = 0;
  int res = _NSGetExecutablePath(nullptr, &buf_len);
  CHECK(res == -1, "expected buffer too small");
  char* path = reinterpret_cast<char*>(std::malloc(buf_len));
  CHECK(path != nullptr, "out of memory");
  res = _NSGetExecutablePath(path, &buf_len);
  CHECK(res == 0, "failed to determine executable path");
  // TODO(matthiasb): The executable or parts of its path may be a symlink
  // that should be resolved.
  return path;
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
