#include "sys-module.h"

#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstring>

#include "builtins-module.h"
#include "bytes-builtins.h"
#include "dict-builtins.h"
#include "exception-builtins.h"
#include "frame.h"
#include "frozen-modules.h"
#include "globals.h"
#include "handles.h"
#include "int-builtins.h"
#include "module-builtins.h"
#include "modules.h"
#include "objects.h"
#include "os.h"
#include "runtime.h"
#include "str-builtins.h"
#include "thread.h"
#include "version.h"

namespace py {

extern "C" struct _inittab _PyImport_Inittab[];

const BuiltinFunction SysModule::kBuiltinFunctions[] = {
    {SymbolId::kExcInfo, excInfo},
    {SymbolId::kExcepthook, excepthook},
    {SymbolId::kSentinelId, nullptr},
};

void SysModule::initialize(Thread* thread, const Module& module) {
  moduleAddBuiltinFunctions(thread, module, kBuiltinFunctions);

  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object modules(&scope, runtime->modules());
  moduleAtPutById(thread, module, SymbolId::kModules, modules);

  // Fill in sys...
  Object platform(&scope, runtime->newStrFromCStr(OS::name()));
  moduleAtPutById(thread, module, SymbolId::kPlatform, platform);

  Object stderr_fd_val(&scope, SmallInt::fromWord(kStderrFd));
  moduleAtPutById(thread, module, SymbolId::kUnderStderrFd, stderr_fd_val);
  Object stdin_fd_val(&scope, SmallInt::fromWord(kStdinFd));
  moduleAtPutById(thread, module, SymbolId::kUnderStdinFd, stdin_fd_val);
  Object stdout_fd_val(&scope, SmallInt::fromWord(kStdoutFd));
  moduleAtPutById(thread, module, SymbolId::kUnderStdoutFd, stdout_fd_val);

  // TODO(T42692043): This awkwardness should go away once we freeze the
  // standard library into the binary and/or support PYTHONPATH.
  Object base_dir(&scope, runtime->newStrFromCStr(PYRO_BASEDIR));
  moduleAtPutById(thread, module, SymbolId::kUnderBaseDir, base_dir);

  // TODO(T58291784): Make getenv system agnostic
  const char* python_path_cstr = std::getenv("PYTHONPATH");
  Object python_path(&scope, NoneType::object());
  if (python_path_cstr != nullptr) {
    Str python_path_str(&scope, runtime->newStrFromCStr(python_path_cstr));
    Str sep(&scope, SmallStr::fromCStr(":"));
    python_path = strSplit(thread, python_path_str, sep, kMaxWord);
    CHECK(!python_path.isError(), "Failed to calculate PYTHONPATH");
  } else {
    python_path = runtime->newList();
  }
  moduleAtPutById(thread, module, SymbolId::kUnderPythonPath, python_path);

  Object byteorder(
      &scope,
      SmallStr::fromCStr(endian::native == endian::little ? "little" : "big"));
  moduleAtPutById(thread, module, SymbolId::kByteorder, byteorder);

  unique_c_ptr<char> executable_path(OS::executablePath());
  Object executable(&scope, runtime->newStrFromCStr(executable_path.get()));
  moduleAtPutById(thread, module, SymbolId::kExecutable, executable);

  // maxsize is defined as the largest supported length of containers which
  // would be `SmallInt::kMaxValue`. However in practice it is used to
  // determine the size of a machine word which is kMaxWord.
  Object maxsize(&scope, runtime->newInt(kMaxWord));
  moduleAtPutById(thread, module, SymbolId::kMaxsize, maxsize);

  Object maxunicode(&scope, SmallInt::fromWord(kMaxUnicode));
  moduleAtPutById(thread, module, SymbolId::kMaxunicode, maxunicode);

  // Count the number of modules and create a tuple
  uword num_external_modules = 0;
  while (_PyImport_Inittab[num_external_modules].name != nullptr) {
    num_external_modules++;
  }
  uword num_builtin_modules = 0;
  while (kBuiltinModules[num_builtin_modules].name != SymbolId::kSentinelId) {
    num_builtin_modules++;
  }

  uword num_modules = num_builtin_modules + num_external_modules;
  MutableTuple builtins_tuple(&scope, runtime->newMutableTuple(num_modules));

  // Add all the available builtin modules
  Symbols* symbols = runtime->symbols();
  for (uword i = 0; i < num_builtin_modules; i++) {
    builtins_tuple.atPut(i, symbols->at(kBuiltinModules[i].name));
  }

  // Add all the available extension builtin modules
  for (int i = 0; _PyImport_Inittab[i].name != nullptr; i++) {
    builtins_tuple.atPut(num_builtin_modules + i,
                         runtime->newStrFromCStr(_PyImport_Inittab[i].name));
  }

  // Create builtin_module_names tuple
  Object builtins(&scope, builtins_tuple.becomeImmutable());
  moduleAtPutById(thread, module, SymbolId::kBuiltinModuleNames, builtins);

  executeFrozenModule(thread, kSysModuleData, module);

  // Fill in hash_info.
  Tuple hash_info_data(&scope, runtime->newMutableTuple(9));
  hash_info_data.atPut(0, SmallInt::fromWord(SmallInt::kBits));
  hash_info_data.atPut(1, SmallInt::fromWord(kArithmeticHashModulus));
  hash_info_data.atPut(2, SmallInt::fromWord(kHashInf));
  hash_info_data.atPut(3, SmallInt::fromWord(kHashNan));
  hash_info_data.atPut(4, SmallInt::fromWord(kHashImag));
  hash_info_data.atPut(5, symbols->Siphash24());
  hash_info_data.atPut(6, SmallInt::fromWord(64));
  hash_info_data.atPut(7, SmallInt::fromWord(128));
  hash_info_data.atPut(8, SmallInt::fromWord(SmallStr::kMaxLength));
  Object hash_info(
      &scope, thread->invokeFunction1(SymbolId::kSys, SymbolId::kUnderHashInfo,
                                      hash_info_data));
  moduleAtPutById(thread, module, SymbolId::kHashInfo, hash_info);

  // Fill in version-related fields
  Str version(&scope, runtime->newStrFromCStr(versionInfo()));
  moduleAtPutById(thread, module, SymbolId::kVersion, version);

  MutableTuple version_info_data(&scope, runtime->newMutableTuple(5));
  version_info_data.atPut(0, SmallInt::fromWord(kVersionMajor));
  version_info_data.atPut(1, SmallInt::fromWord(kVersionMinor));
  version_info_data.atPut(2, SmallInt::fromWord(kVersionMicro));
  version_info_data.atPut(3, runtime->newStrFromCStr(kReleaseLevel));
  version_info_data.atPut(4, SmallInt::fromWord(kReleaseSerial));
  Object version_info(&scope, thread->invokeFunction1(
                                  SymbolId::kSys, SymbolId::kUnderVersionInfo,
                                  version_info_data));
  moduleAtPutById(thread, module, SymbolId::kVersionInfo, version_info);

  runtime->cacheSysInstances(thread, module);
}

static void writeImpl(Thread* thread, const Object& file, FILE* fallback_fp,
                      const char* format, va_list va) {
  HandleScope scope(thread);
  Object type(&scope, thread->pendingExceptionType());
  Object value(&scope, thread->pendingExceptionValue());
  Object tb(&scope, thread->pendingExceptionTraceback());
  thread->clearPendingException();

  static const char truncated[] = "... truncated";
  static constexpr int message_size = 1001;
  char buffer[message_size + sizeof(truncated) - 1];
  int written = std::vsnprintf(buffer, message_size, format, va);
  CHECK(written >= 0, "vsnprintf failed");
  if (written >= message_size) {
    std::memcpy(&buffer[message_size - 1], truncated, sizeof(truncated));
    written = message_size + sizeof(truncated) - 2;
  }
  Str str(&scope, thread->runtime()->newStrWithAll(
                      View<byte>(reinterpret_cast<byte*>(buffer), written)));
  if (file.isNoneType() ||
      thread->invokeMethod2(file, SymbolId::kWrite, str).isError()) {
    fwrite(buffer, 1, written, fallback_fp);
  }

  thread->clearPendingException();
  thread->setPendingExceptionType(*type);
  thread->setPendingExceptionValue(*value);
  thread->setPendingExceptionTraceback(*tb);
}

void writeStdout(Thread* thread, const char* format, ...) {
  va_list va;
  va_start(va, format);
  writeStdoutV(thread, format, va);
  va_end(va);
}

void writeStdoutV(Thread* thread, const char* format, va_list va) {
  HandleScope scope(thread);
  ValueCell sys_stdout_cell(&scope, thread->runtime()->sysStdout());
  Object sys_stdout(&scope, NoneType::object());
  if (!sys_stdout_cell.isUnbound()) {
    sys_stdout = sys_stdout_cell.value();
  }
  writeImpl(thread, sys_stdout, stdout, format, va);
}

void writeStderr(Thread* thread, const char* format, ...) {
  va_list va;
  va_start(va, format);
  writeStderrV(thread, format, va);
  va_end(va);
}

void writeStderrV(Thread* thread, const char* format, va_list va) {
  HandleScope scope(thread);
  ValueCell sys_stderr_cell(&scope, thread->runtime()->sysStderr());
  Object sys_stderr(&scope, NoneType::object());
  if (!sys_stderr_cell.isUnbound()) {
    sys_stderr = sys_stderr_cell.value();
  }
  writeImpl(thread, sys_stderr, stderr, format, va);
}

RawObject SysModule::excepthook(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  // type argument is ignored
  Object value(&scope, args.get(1));
  Object tb(&scope, args.get(2));
  displayException(thread, value, tb);
  return NoneType::object();
}

RawObject SysModule::excInfo(Thread* thread, Frame* /* frame */,
                             word /* nargs */) {
  HandleScope scope(thread);
  Tuple result(&scope, thread->runtime()->newTuple(3));
  if (thread->hasCaughtException()) {
    result.atPut(0, thread->caughtExceptionType());
    result.atPut(1, thread->caughtExceptionValue());
    result.atPut(2, thread->caughtExceptionTraceback());
  } else {
    result.atPut(0, RawNoneType::object());
    result.atPut(1, RawNoneType::object());
    result.atPut(2, RawNoneType::object());
  }
  return *result;
}

}  // namespace py
