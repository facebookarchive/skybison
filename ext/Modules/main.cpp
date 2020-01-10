#include <getopt.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cwchar>
#include <memory>

#include "cpython-data.h"
#include "cpython-func.h"

#include "builtins-module.h"
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

static int runFile(FILE* fp, const char* filename, PyCompilerFlags* flags) {
  bool is_stdin = filename == nullptr;
  const char* file_or_stdin = is_stdin ? "<stdin>" : filename;
  return PyRun_AnyFileExFlags(fp, file_or_stdin, !is_stdin, flags) != 0;
}

static void runInteractiveHook() {
  Thread* thread = Thread::current();
  RawObject result =
      thread->invokeFunction0(SymbolId::kSys, SymbolId::kDunderInteractiveHook);
  if (result.isErrorException()) {
    std::fprintf(stderr, "Failed calling sys.__interactivehook__\n");
    printPendingExceptionWithSysLastVars(thread);
    thread->clearPendingException();
  }
}

static int runModule(const char* modname_cstr, bool set_argv0) {
  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Str runpy(&scope, runtime->symbols()->at(SymbolId::kRunpy));
  Object result(&scope,
                thread->invokeFunction1(SymbolId::kBuiltins,
                                        SymbolId::kDunderImport, runpy));
  if (result.isError()) {
    std::fprintf(stderr, "Could not import runpy module\n");
    printPendingException(thread);
    return -1;
  }

  runtime->findOrCreateMainModule();
  Str modname(&scope, runtime->newStrFromCStr(modname_cstr));
  Bool alter_argv(&scope, Bool::fromBool(set_argv0));
  result = thread->invokeFunction2(
      SymbolId::kRunpy, SymbolId::kUnderRunModuleAsMain, modname, alter_argv);
  if (result.isError()) {
    printPendingException(thread);
    return -1;
  }
  return 0;
}

static void runStartupFile(PyCompilerFlags* cf) {
  const char* startupfile = std::getenv("PYTHONSTARTUP");
  if (startupfile == nullptr || startupfile[0] == '\0') return;
  FILE* fp = std::fopen(startupfile, "r");
  if (fp != nullptr) {
    PyRun_SimpleFileExFlags(fp, startupfile, 0, cf);
    std::fclose(fp);
  } else {
    int saved_errno = errno;
    PySys_WriteStderr("Could not open PYTHONSTARTUP\n");
    errno = saved_errno;
    PyErr_SetFromErrnoWithFilename(PyExc_IOError, startupfile);
    PyErr_Print();
  }
  PyErr_Clear();
}

PY_EXPORT int Py_BytesMain(int argc, char** argv) {
  bool print_version = false;
  bool print_help = false;
  const char* command = nullptr;
  const char* module = nullptr;

  int option;
  while ((option = getopt_long(argc, argv, kSupportedOpts, kSupportedLongOpts,
                               nullptr)) != -1) {
    // -c and -m mark the end of interpreter options - all further
    // arguments are passed to the script
    if (option == 'c') {
      command = optarg;
      break;
    }
    if (option == 'm') {
      module = optarg;
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
  if (command == nullptr && module == nullptr && optind < argc &&
      std::strcmp(argv[optind], "-") != 0) {
    filename = argv[optind];
  }

  bool is_interactive = Py_FdIsInteractive(stdin, nullptr);

  Py_Initialize();

  if (!Py_QuietFlag &&
      (Py_VerboseFlag || (command == nullptr && filename == nullptr &&
                          module == nullptr && is_interactive))) {
    std::fprintf(stderr, "Python %s on %s\n", Py_GetVersion(),
                 Py_GetPlatform());
    if (!Py_NoSiteFlag) {
      std::fprintf(stderr, "%s\n", kInteractiveHelp);
    }
  }

  int wargc;
  wchar_t** wargv;
  if (command != nullptr || module != nullptr) {
    // Start arg list with "-c" or "-m" and omit command/module arg
    wargc = argc - optind + 1;
    const char** argv_copy =
        static_cast<const char**>(PyMem_RawCalloc(wargc, sizeof(*argv_copy)));
    argv_copy[0] = command != nullptr ? "-c" : "-m";
    for (int i = optind; i < argc; i++) {
      argv_copy[i - optind + 1] = argv[i];
    }
    wargv = static_cast<wchar_t**>(PyMem_RawCalloc(wargc, sizeof(*wargv)));
    decodeArgv(wargc, argv_copy, wargv);
    PyMem_RawFree(argv_copy);
  } else {
    wargc = argc - optind;
    wargv = static_cast<wchar_t**>(PyMem_RawCalloc(wargc, sizeof(*wargv)));
    decodeArgv(wargc, argv + optind, wargv);
  }
  // TODO(T58637222): Use PySys_SetArgv once pyro is packaged with library
  PySys_SetArgvEx(wargc, wargv, 0);
  for (int i = 0; i < wargc; i++) {
    PyMem_RawFree(wargv[i]);
  }
  PyMem_RawFree(wargv);

  PyCompilerFlags flags;
  flags.cf_flags = 0;

  int returncode = EXIT_SUCCESS;

  if (command != nullptr) {
    returncode = PyRun_SimpleStringFlags(command, &flags) != 0;
  } else if (module != nullptr) {
    returncode = runModule(module, true) != 0;
  } else {
    if (filename == nullptr && is_interactive) {
      Py_InspectFlag = 0;  // do exit on SystemExit
      runStartupFile(&flags);
      runInteractiveHook();
    }

    FILE* fp = stdin;
    if (filename != nullptr) {
      fp = std::fopen(filename, "r");
      if (fp == nullptr) {
        std::fprintf(stderr, "%s: can't open file '%s': [Errno %d] %s\n",
                     argv[0], filename, errno, std::strerror(errno));
        return 2;
      }
    }

    returncode = runFile(fp, filename, &flags);
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
