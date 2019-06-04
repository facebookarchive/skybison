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
  bool cache_enabled = false;
  const char* enable_cache = std::getenv("PYRO_ENABLE_CACHE");
  if (enable_cache != nullptr && enable_cache[0] != '\0') {
    cache_enabled = true;
  }
  // TODO(T43657667) Add code that can decide what we can cache and remove this.
  python::Runtime runtime(cache_enabled);
  python::Thread* thread = python::Thread::current();
  runtime.setArgv(thread, argc, argv);
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
  python::HandleScope scope(thread);
  python::Object result(&scope, runtime.run(buffer.get()));
  if (result.isError()) {
    python::printPendingException(python::Thread::current());
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
