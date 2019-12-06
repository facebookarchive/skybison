#include <cstdlib>
#include <cwchar>
#include <memory>

#include "builtins-module.h"
#include "cpython-func.h"
#include "exception-builtins.h"
#include "marshal.h"
#include "os.h"
#include "runtime.h"
#include "thread.h"
#include "view.h"

namespace py {

static void failArgConversion(const char* message, int argi) {
  std::fprintf(stderr, "Fatal python error: %s #%i\n", message, argi);
  std::abort();
}

static void decodeArgv(int argc, const char* const* argv, wchar_t** wargv) {
  for (int i = 0; i < argc; i++) {
    wargv[i] = Py_DecodeLocale(argv[i], nullptr);
    if (wargv[i] == nullptr) {
      failArgConversion("unable to decode the command line argument", i + 1);
    }
  }
}

static void encodeWargv(int argc, const wchar_t* const* wargv, char** argv) {
  for (int i = 0; i < argc; i++) {
    argv[i] = Py_EncodeLocale(wargv[i], nullptr);
    if (argv[i] == nullptr) {
      failArgConversion("unable to encode the command line argument", i + 1);
    }
  }
}

// TODO(T39499894): Rewrite this whole function to use the C-API.
static RawObject runFile(Thread* thread, const char* filename) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();

  // TODO(T58704877): Don't read the whole file into memory all at once
  word file_len;
  std::unique_ptr<char[]> buffer(OS::readFile(filename, &file_len));
  if (buffer == nullptr) {
    std::fprintf(stderr, "Could not read file '%s'\n", filename);
    std::exit(EXIT_FAILURE);
  }

  Object code_obj(&scope, NoneType::object());
  const char* delim = std::strrchr(filename, '.');
  View<byte> data(reinterpret_cast<byte*>(buffer.get()), file_len);
  Str filename_str(&scope, runtime->newStrFromCStr(filename));
  if (delim && std::strcmp(delim, ".pyc") != 0) {
    // Interpret as .py and compile
    Object source(&scope, runtime->newStrWithAll(data));
    code_obj = compile(thread, source, filename_str, SymbolId::kExec,
                       /*flags=*/0, /*optimize=*/-1);
  } else {
    // Interpret as .pyc and unmarshal
    Marshal::Reader reader(&scope, runtime, data);
    if (reader.readPycHeader(filename_str).isErrorException()) {
      return Error::exception();
    }
    code_obj = reader.readObject();
  }
  if (code_obj.isErrorException()) return *code_obj;

  Code code(&scope, *code_obj);
  Module main_module(&scope, runtime->findOrCreateMainModule());
  return runtime->executeModule(code, main_module);
}

PY_EXPORT int Py_BytesMain(int argc, char** argv) {
  Py_Initialize();

  // TODO(tylerk): Parse and handle arguments as CPython does
  wchar_t** wargv =
      static_cast<wchar_t**>(PyMem_RawCalloc(argc - 1, sizeof(*wargv)));
  decodeArgv(argc - 1, argv + 1, wargv);  // skip the first argument
  // TODO(T58637222): Use PySys_SetArgv once pyro is packaged with library
  PySys_SetArgvEx(argc - 1, wargv, 0);
  for (int i = 0; i < argc - 1; i++) {
    PyMem_RawFree(wargv[i]);
  }
  PyMem_RawFree(wargv);

  int returncode = EXIT_SUCCESS;

  if (argc < 2) {
    PyCompilerFlags flags;
    flags.cf_flags = 0;
    returncode = PyRun_AnyFileExFlags(stdin, "<stdin>", /*closeit=*/0, &flags);
  } else {
    // TODO(T39499894): Rewrite this to use the C-API.
    Thread* thread = Thread::current();
    HandleScope scope(thread);
    Object result(&scope, runFile(thread, argv[1]));

    if (result.isErrorException()) {
      printPendingException(thread);
      returncode = EXIT_FAILURE;
    }
  }

  Py_Finalize();

  return returncode;
}

PY_EXPORT int Py_Main(int argc, wchar_t** wargv) {
  std::fprintf(stderr,
               "Py_Main(int, wchar_t**) is intended for Windows applications; "
               "consider using Py_BytesMain(int, char**) on POSIX");
  char** argv = static_cast<char**>(PyMem_RawCalloc(argc, sizeof(*argv)));
  encodeWargv(argc, wargv, argv);
  int res = Py_BytesMain(argc, argv);
  for (int i = 0; i < argc; i++) {
    PyMem_Free(argv[i]);
  }
  PyMem_RawFree(argv);
  return res;
}

}  // namespace py
