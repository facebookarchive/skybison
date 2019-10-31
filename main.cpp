#include <cstdlib>
#include <memory>

#include "capi/cpython-func.h"
#include "runtime/compile.h"
#include "runtime/exception-builtins.h"
#include "runtime/marshal.h"
#include "runtime/os.h"
#include "runtime/runtime.h"
#include "runtime/thread.h"
#include "runtime/view.h"

static py::RawObject runFile(py::Thread* thread, const char* filename) {
  py::HandleScope scope(thread);
  py::Runtime* runtime = thread->runtime();

  word file_len;
  std::unique_ptr<char[]> buffer(py::OS::readFile(filename, &file_len));
  if (buffer == nullptr) {
    fprintf(stderr, "Could not read file '%s'\n", filename);
    std::exit(EXIT_FAILURE);
  }

  py::Object code_obj(&scope, py::NoneType::object());
  const char* delim = std::strrchr(filename, '.');
  if (delim && std::strcmp(delim, ".pyc") != 0) {
    // Interpret as .py and compile
    std::unique_ptr<char[]> buffer_cstr(new char[file_len + 1]);
    if (buffer_cstr == nullptr) std::exit(EXIT_FAILURE);
    std::memcpy(buffer_cstr.get(), buffer.get(), file_len);
    buffer_cstr[file_len] = '\0';
    code_obj = py::compileFromCStr(buffer_cstr.get(), filename);
  } else {
    // Interpret as .pyc and unmarshal
    py::View<byte> data(reinterpret_cast<byte*>(buffer.get()), file_len);
    py::Marshal::Reader reader(&scope, runtime, data);
    py::Str filename_str(&scope, runtime->newStrFromCStr(filename));
    if (reader.readPycHeader(filename_str).isErrorException()) {
      return py::Error::exception();
    }
    code_obj = reader.readObject();
  }
  if (code_obj.isErrorException()) return *code_obj;

  // TODO(T39499894): Rewrite this whole function to use the C-API.
  py::Code code(&scope, *code_obj);
  py::Module main_module(&scope, runtime->findOrCreateMainModule());
  return runtime->executeModule(code, main_module);
}

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
    PyCompilerFlags flags;
    flags.cf_flags = 0;
    return PyRun_AnyFileExFlags(stdin, "<stdin>", /*closeit=*/0, &flags);
  }

  py::HandleScope scope(thread);
  py::Object result(&scope, runFile(thread, argv[1]));
  if (result.isErrorException()) {
    py::printPendingException(py::Thread::current());
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
