#include "os.h"

#include <unistd.h>

#include <cstdlib>

#include "utils.h"

namespace python {

const char* OS::name() { return "linux"; }

char* OS::executablePath() {
  char* buffer = nullptr;
  for (ssize_t size = 64;; size *= 2) {
    buffer = reinterpret_cast<char*>(std::realloc(buffer, size));
    CHECK(buffer != nullptr, "out of memory");
    ssize_t res = readlink("/proc/self/exe", buffer, size);
    CHECK(res >= 0, "failed to determine executable path");
    if (res < size - 1) {
      buffer[res] = '\0';
      break;
    }
  }
  return buffer;
}

}  // namespace python
