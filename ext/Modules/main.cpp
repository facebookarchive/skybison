#include <getopt.h>
#include <cstdio>
#include <cstdlib>
#include <cwchar>
#include <memory>

#include "builtins-module.h"
#include "cpython-data.h"
#include "cpython-func.h"
#include "exception-builtins.h"
#include "marshal.h"
#include "os.h"
#include "runtime.h"
#include "thread.h"
#include "view.h"

namespace py {

static const char* const kInteractiveHelp =
    R"(Type "help", "copyright", "credits" or "license" for more information.)";

static const char* const kSupportedOpts = "+bBc:dEhiIm:OqsSuvVW:xX:";
static const struct option kSupportedLongOpts[] = {
    {"help", no_argument, nullptr, 'h'},
    {"version", no_argument, nullptr, 'V'},
    {nullptr, 0, nullptr, 0}};

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
  bool print_version = false;
  bool print_help = false;

  int option;
  while ((option = getopt_long(argc, argv, kSupportedOpts, kSupportedLongOpts,
                               nullptr)) != -1) {
    // -c and -m mark the end of interpreter options - all further
    // arguments are passed to the script
    if (option == 'c') {
      UNIMPLEMENTED("Program passed in as string");
      break;
    }
    if (option == 'm') {
      UNIMPLEMENTED("Run library module as a script");
      break;
    }

    wchar_t* woptarg;
    switch (option) {
      case 'b':
        Py_BytesWarningFlag++;
        UNIMPLEMENTED("Bytes Warning Flag");
        break;
      case 'd':
        Py_DebugFlag++;
        UNIMPLEMENTED("Parser Debug Flag");
        break;
      case 'i':
        Py_InspectFlag++;
        Py_InteractiveFlag++;
        UNIMPLEMENTED("Inspect Interactively Flag");
        break;
      case 'I':
        Py_IsolatedFlag++;
        Py_NoUserSiteDirectory++;
        Py_IgnoreEnvironmentFlag++;
        UNIMPLEMENTED("Isolate Flag");
        break;
      case 'O':
        Py_OptimizeFlag++;
        UNIMPLEMENTED("Optimize Flag");
        break;
      case 'B':
        Py_DontWriteBytecodeFlag++;
        UNIMPLEMENTED("Don't Write Bytecode Flag");
        break;
      case 's':
        Py_NoUserSiteDirectory++;
        UNIMPLEMENTED("No User Site Directory");
        break;
      case 'S':
        Py_NoSiteFlag++;
        UNIMPLEMENTED("No site flag");
        break;
      case 'E':
        Py_IgnoreEnvironmentFlag++;
        UNIMPLEMENTED("Ignore PYTHON* environment variables");
        break;
      case 'u':
        Py_UnbufferedStdioFlag = 1;
        UNIMPLEMENTED("Unbuffered stdio flag");
        break;
      case 'v':
        Py_VerboseFlag++;
        UNIMPLEMENTED("Verbose flag");
        break;
      case 'x':
        UNIMPLEMENTED("skip first line");
        break;
      case 'h':
        print_help = true;
        break;
      case '?':
        UNIMPLEMENTED("Help for invalid option");
        break;
      case 'V':
        print_version = true;
        break;
      case 'W':
        UNIMPLEMENTED("Warning options");
        break;
      case 'X':
        woptarg = Py_DecodeLocale(optarg, nullptr);
        PySys_AddXOption(woptarg);
        PyMem_RawFree(woptarg);
        woptarg = nullptr;
        break;
      case 'q':
        Py_QuietFlag++;
        UNIMPLEMENTED("Quiet Flag");
        break;
      default:
        UNREACHABLE("Unexpected value returned from getopt_long()");
    }
  }

  if (print_help) {
    UNIMPLEMENTED("command line help");
  }

  if (print_version) {
    // TODO(T58173807): Version reporting which is decoupled from runtime init
    std::printf("Python 3.6.8+\n");
    return 0;
  }

  char* filename = nullptr;
  if (optind < argc && std::strcmp(argv[optind], "-") != 0) {
    filename = argv[optind];
  }

  bool is_interactive = Py_FdIsInteractive(stdin, nullptr);

  Py_Initialize();

  if (!Py_QuietFlag &&
      (Py_VerboseFlag || (filename == nullptr && is_interactive))) {
    std::fprintf(stderr, "Python %s on %s\n", Py_GetVersion(),
                 Py_GetPlatform());
    if (!Py_NoSiteFlag) {
      std::fprintf(stderr, "%s\n", kInteractiveHelp);
    }
  }

  wchar_t** wargv =
      static_cast<wchar_t**>(PyMem_RawCalloc(argc - optind, sizeof(*wargv)));
  decodeArgv(argc - optind, argv + optind, wargv);
  // TODO(T58637222): Use PySys_SetArgv once pyro is packaged with library
  PySys_SetArgvEx(argc - optind, wargv, 0);
  for (int i = 0; i < argc - optind; i++) {
    PyMem_RawFree(wargv[i]);
  }
  PyMem_RawFree(wargv);

  int returncode = EXIT_SUCCESS;

  if (filename == nullptr) {
    PyCompilerFlags flags;
    flags.cf_flags = 0;
    returncode = PyRun_AnyFileExFlags(stdin, "<stdin>", /*closeit=*/0, &flags);
  } else {
    // TODO(T39499894): Rewrite this to use the C-API.
    Thread* thread = Thread::current();
    HandleScope scope(thread);
    Object result(&scope, runFile(thread, filename));
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
