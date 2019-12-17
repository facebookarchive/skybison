#include "os.h"

#include <dlfcn.h>
#include <mach-o/dyld.h>

#include <cstdint>
#include <cstdlib>

#include "utils.h"

namespace py {

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
