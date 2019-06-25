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

  const char* delim = std::strrchr(file_name, '.');
  python::HandleScope scope(thread);
  python::Object code_obj(&scope, python::NoneType::object());
  if (delim && std::strcmp(delim, ".pyc") != 0) {
    // Interpret as .py and compile
    std::unique_ptr<char[]> buffer_cstr(new char[file_len + 1]);
    if (buffer_cstr == nullptr) std::exit(EXIT_FAILURE);
    memcpy(buffer_cstr.get(), buffer.get(), file_len);
    buffer_cstr[file_len] = '\0';
    code_obj = python::compileFromCStr(buffer_cstr.get(), file_name);
  } else {
    // Interpret as .pyc and unmarshal
    code_obj = bytecodeToCode(thread, buffer.get());
  }

  // TODO(T39499894): Rewrite this whole function to use the C-API.
  python::Code code(&scope, *code_obj);
  python::Module main_module(&scope, runtime.findOrCreateMainModule());
  python::Object result(&scope, runtime.executeModule(code, main_module));
  if (result.isError()) {
    python::printPendingException(python::Thread::current());
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
