#include <cstdlib>
#include <memory>

#include "runtime/os.h"
#include "runtime/runtime.h"

int main(int argc, char** argv) {
  if (argc != 2) {
    return EXIT_FAILURE;
  }
  python::Runtime runtime;
  std::unique_ptr<char[]> buffer(python::OS::readFile(argv[1]));
  if (buffer.get() == nullptr) {
    std::exit(EXIT_FAILURE);
  }
  runtime.run(buffer.get());
  return EXIT_SUCCESS;
}
