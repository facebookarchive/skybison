#include <cstdlib>
#include <memory>

#include "runtime/exception-builtins.h"
#include "runtime/os.h"
#include "runtime/runtime.h"
#include "runtime/thread.h"
#include "runtime/view.h"

int main(int argc, const char** argv) {
  if (argc < 2) {
    return EXIT_FAILURE;
  }
  python::Runtime runtime;
  runtime.setArgv(argc, argv);
  const char* file_name = argv[1];
  word file_len;
  std::unique_ptr<char[]> buffer(python::OS::readFile(file_name, &file_len));
  if (buffer == nullptr) std::exit(EXIT_FAILURE);

  // Interpret as bytecode iff file name suffix is '.pyc':
  const char* delim = std::strrchr(file_name, '.');
  if (delim && std::strcmp(delim, ".pyc") != 0) {
    buffer = python::Runtime::compile(
        python::View<char>(buffer.get(), file_len), file_name);
  }

  // TODO(T39499894): Rewrite this whole function to use the C-API.
  python::HandleScope scope;
  python::Object result(&scope, runtime.run(buffer.get()));
  if (result.isError()) {
    python::printPendingException(python::Thread::current());
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
