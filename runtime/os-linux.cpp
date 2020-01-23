#include "os.h"

#include <dlfcn.h>
#include <signal.h>
#include <unistd.h>

#include <cstdlib>

#include "utils.h"

namespace py {

const word OS::kNumSignals = _NSIG;

const char* OS::name() { return "linux"; }

char* OS::executablePath() {
  char* buffer = readLink("/proc/self/exe");
  CHECK(buffer != nullptr, "failed to determine executable path");
  return buffer;
}

void* OS::openSharedObject(const char* filename, const char** error_msg) {
  void* result = ::dlopen(filename, RTLD_NOW);
  if (result == nullptr) {
    *error_msg = ::dlerror();
  }
  return result;
}

void* OS::sharedObjectSymbolAddress(void* handle, const char* symbol) {
  return ::dlsym(handle, symbol);
}

}  // namespace py
