#include "sys-module.h"
#include <unistd.h>

#include <cstdio>

#include "builtins-module.h"
#include "frame.h"
#include "frozen-modules.h"
#include "globals.h"
#include "handles.h"
#include "objects.h"
#include "os.h"
#include "runtime.h"
#include "thread.h"

namespace python {

extern "C" struct _inittab _PyImport_Inittab[];

void SysModule::postInitialize(Thread* thread, Runtime* runtime,
                               const Module& module) {
  HandleScope scope(thread);
  Object modules(&scope, runtime->modules_);
  runtime->moduleAddGlobal(module, SymbolId::kModules, modules);

  runtime->display_hook_ = runtime->moduleAddBuiltinFunction(
      module, SymbolId::kDisplayhook, displayhook);

  // Fill in sys...
  Object stdout_val(&scope, SmallInt::fromWord(STDOUT_FILENO));
  runtime->moduleAddGlobal(module, SymbolId::kStdout, stdout_val);

  Object stderr_val(&scope, SmallInt::fromWord(STDERR_FILENO));
  runtime->moduleAddGlobal(module, SymbolId::kStderr, stderr_val);

  Object platform(&scope, runtime->newStrFromCStr(OS::name()));
  runtime->moduleAddGlobal(module, SymbolId::kPlatform, platform);

  unique_c_ptr<char> executable_path(OS::executablePath());
  Object executable(&scope, runtime->newStrFromCStr(executable_path.get()));
  runtime->moduleAddGlobal(module, SymbolId::kExecutable, executable);

  // Count the number of modules and create a tuple
  uword num_external_modules = 0;
  while (_PyImport_Inittab[num_external_modules].name != nullptr) {
    num_external_modules++;
  }
  uword num_builtin_modules = 0;
  for (; Runtime::kBuiltinModules[num_builtin_modules].name !=
         SymbolId::kSentinelId;
       num_builtin_modules++) {
  }

  uword num_modules = num_builtin_modules + num_external_modules;
  Tuple builtins_tuple(&scope, runtime->newTuple(num_modules));

  // Add all the available builtin modules
  for (uword i = 0; i < num_builtin_modules; i++) {
    Object module_name(
        &scope, runtime->symbols()->at(Runtime::kBuiltinModules[i].name));
    builtins_tuple.atPut(i, *module_name);
  }

  // Add all the available extension builtin modules
  for (int i = 0; _PyImport_Inittab[i].name != nullptr; i++) {
    Object module_name(&scope,
                       runtime->newStrFromCStr(_PyImport_Inittab[i].name));
    builtins_tuple.atPut(num_builtin_modules + i, *module_name);
  }

  // Create builtin_module_names tuple
  Object builtins(&scope, *builtins_tuple);
  runtime->moduleAddGlobal(module, SymbolId::kBuiltinModuleNames, builtins);

  runtime->executeModule(kSysModuleData, module);
}

static void writeImpl(Thread* thread, std::ostream* stream, const char* format,
                      va_list va) {
  static constexpr int buf_size = 1001;
  char buffer[buf_size];

  HandleScope scope(thread);
  Object type(&scope, thread->pendingExceptionType());
  Object value(&scope, thread->pendingExceptionValue());
  Object tb(&scope, thread->pendingExceptionTraceback());
  thread->clearPendingException();

  // TODO(T41323917): Use sys.stdout/sys.stderr once we have stream support.
  int written = std::vsnprintf(buffer, buf_size, format, va);
  *stream << buffer;
  if (written >= buf_size) *stream << "... truncated";

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
  writeImpl(thread, builtinStdout, format, va);
}

void writeStderr(Thread* thread, const char* format, ...) {
  va_list va;
  va_start(va, format);
  writeStderrV(thread, format, va);
  va_end(va);
}

void writeStderrV(Thread* thread, const char* format, va_list va) {
  writeImpl(thread, builtinStderr, format, va);
}

RawObject SysModule::displayhook(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object obj(&scope, args.get(0));
  if (obj.isNoneType()) {
    return NoneType::object();
  }
  UNIMPLEMENTED("sys.displayhook()");
}

}  // namespace python
