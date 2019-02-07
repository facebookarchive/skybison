#include <cstdlib>
#include <memory>

#include "runtime/os.h"
#include "runtime/runtime.h"
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
    buffer =
        python::Runtime::compile(python::View<char>(buffer.get(), file_len));
  }

  // TODO(T39499894): Move this code into PyErr_PrintEx(), rewrite this whole
  // function to use the C-API.
  python::HandleScope scope;
  python::Object result(&scope, runtime.run(buffer.get()));
  if (result->isError()) {
    python::Thread::currentThread()->printPendingException();
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
