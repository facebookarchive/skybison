#include <cstdlib>
#include <memory>

#include "runtime/os.h"
#include "runtime/runtime.h"

int main(int argc, const char** argv) {
  if (argc < 2) {
    return EXIT_FAILURE;
  }
  python::Runtime runtime;
  runtime.setArgv(argc, argv);
  const char* file_name = argv[1];
  std::unique_ptr<char[]> buffer(python::OS::readFile(file_name));
  if (buffer.get() == nullptr) {
    std::exit(EXIT_FAILURE);
  }
  bool is_source(true);

  // Interpret as bytecode iff file name suffix is '.pyc':
  const char* delim = strrchr(file_name, '.');
  if (delim) {
    const char* ext = delim + 1;
    if (strcmp(ext, "pyc") == 0) {
      is_source = false;
    }
  }
  if (is_source) {
    buffer = python::Runtime::compile(buffer.get());
  }
  runtime.run(buffer.get());
  return EXIT_SUCCESS;
}
