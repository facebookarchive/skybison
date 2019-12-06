#include <cstdlib>
#include <memory>

#include "capi/cpython-func.h"
#include "runtime/builtins-module.h"
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
  py::View<byte> data(reinterpret_cast<byte*>(buffer.get()), file_len);
  py::Str filename_str(&scope, runtime->newStrFromCStr(filename));
  if (delim && std::strcmp(delim, ".pyc") != 0) {
    // Interpret as .py and compile
    py::Object source(&scope, runtime->newStrWithAll(data));
    code_obj = py::compile(thread, source, filename_str, py::SymbolId::kExec,
                           /*flags=*/0, /*optimize=*/-1);
  } else {
    // Interpret as .pyc and unmarshal
    py::Marshal::Reader reader(&scope, runtime, data);
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
  // TODO(T55262429): Reduce the heap size once memory issues are fixed.
  py::Runtime runtime(128 * kMiB);
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
    py::printPendingException(thread);
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
