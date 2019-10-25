#include <cstdlib>
#include <memory>

#include "runtime/compile.h"
#include "runtime/exception-builtins.h"
#include "runtime/marshal.h"
#include "runtime/os.h"
#include "runtime/runtime.h"
#include "runtime/thread.h"
#include "runtime/view.h"

int main(int argc, const char** argv) {
  bool cache_enabled = true;
  const char* enable_cache = std::getenv("PYRO_ENABLE_CACHE");
  if (enable_cache != nullptr && enable_cache[0] == '0') {
    cache_enabled = false;
  }
  // TODO(T43657667): Add code that can decide what we can cache and remove
  // this.
  // TODO(T55262429): Reduce the heap size once memory issues are fixed.
  py::Runtime runtime(1 * kGiB, cache_enabled);
  py::Thread* thread = py::Thread::current();
  runtime.setArgv(thread, argc, argv);
  if (argc < 2) {
    return py::runInteractive(stdin);
  }
  const char* file_name = argv[1];
  word file_len;
  std::unique_ptr<char[]> buffer(py::OS::readFile(file_name, &file_len));
  if (buffer == nullptr) std::exit(EXIT_FAILURE);

  const char* delim = std::strrchr(file_name, '.');
  py::HandleScope scope(thread);
  py::Object code_obj(&scope, py::NoneType::object());
  if (delim && std::strcmp(delim, ".pyc") != 0) {
    // Interpret as .py and compile
    std::unique_ptr<char[]> buffer_cstr(new char[file_len + 1]);
    if (buffer_cstr == nullptr) std::exit(EXIT_FAILURE);
    memcpy(buffer_cstr.get(), buffer.get(), file_len);
    buffer_cstr[file_len] = '\0';
    code_obj = py::compileFromCStr(buffer_cstr.get(), file_name);
  } else {
    // Interpret as .pyc and unmarshal
    code_obj = bytecodeToCode(thread, buffer.get());
  }

  // TODO(T39499894): Rewrite this whole function to use the C-API.
  py::Code code(&scope, *code_obj);
  py::Module main_module(&scope, runtime.findOrCreateMainModule());
  py::Object result(&scope, runtime.executeModule(code, main_module));
  if (result.isError()) {
    py::printPendingException(py::Thread::current());
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
