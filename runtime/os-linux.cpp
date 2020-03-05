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

void* OS::sharedObjectSymbolAddress(void* handle, const char* symbol) {
  return ::dlsym(handle, symbol);
}

}  // namespace py
