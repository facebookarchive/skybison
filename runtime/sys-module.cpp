#include "sys-module.h"

#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstring>

#include "cpython-types.h"

#include "builtins-module.h"
#include "builtins.h"
#include "bytes-builtins.h"
#include "capi.h"
#include "dict-builtins.h"
#include "exception-builtins.h"
#include "file.h"
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

void FUNC(sys, __init_module__)(Thread* thread, const Module& module,
                                View<byte> bytecode) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object modules(&scope, runtime->modules());
  moduleAtPutById(thread, module, ID(modules), modules);

  // Fill in sys...
  Object platform(&scope, runtime->newStrFromCStr(OS::name()));
  moduleAtPutById(thread, module, ID(platform), platform);

  Object stderr_fd_val(&scope, SmallInt::fromWord(File::kStderr));
  moduleAtPutById(thread, module, ID(_stderr_fd), stderr_fd_val);
  Object stdin_fd_val(&scope, SmallInt::fromWord(File::kStdin));
  moduleAtPutById(thread, module, ID(_stdin_fd), stdin_fd_val);
  Object stdout_fd_val(&scope, SmallInt::fromWord(File::kStdout));
  moduleAtPutById(thread, module, ID(_stdout_fd), stdout_fd_val);

  Object byteorder(
      &scope,
      SmallStr::fromCStr(endian::native == endian::little ? "little" : "big"));
  moduleAtPutById(thread, module, ID(byteorder), byteorder);

  // maxsize is defined as the largest supported length of containers which
  // would be `SmallInt::kMaxValue`. However in practice it is used to
  // determine the size of a machine word which is kMaxWord.
  Object maxsize(&scope, runtime->newInt(kMaxWord));
  moduleAtPutById(thread, module, ID(maxsize), maxsize);

  // Count the number of builtin modules and create a tuple
  uword num_extension_modules = 0;
  while (PyImport_Inittab[num_extension_modules].name != nullptr) {
    num_extension_modules++;
  }
  word num_builtin_modules = kNumFrozenModules + num_extension_modules;
  MutableTuple builtin_module_names(
      &scope, runtime->newMutableTuple(num_builtin_modules));
  for (word i = 0; i < kNumFrozenModules; i++) {
    builtin_module_names.atPut(
        i, Runtime::internStrFromCStr(thread, kFrozenModules[i].name));
  }
  for (int i = 0; PyImport_Inittab[i].name != nullptr; i++) {
    builtin_module_names.atPut(
        kNumFrozenModules + i,
        Runtime::internStrFromCStr(thread, PyImport_Inittab[i].name));
  }
  Tuple builtin_module_names_tuple(&scope,
                                   builtin_module_names.becomeImmutable());
  moduleAtPutById(thread, module, ID(builtin_module_names),
                  builtin_module_names_tuple);

  // Fill in version-related fields.
  Int hexversion(&scope, SmallInt::fromWord(kVersionHex));
  moduleAtPutById(thread, module, ID(hexversion), hexversion);
  Str version(&scope, runtime->newStrFromCStr(kVersionInfo));
  moduleAtPutById(thread, module, ID(version), version);
  Object release_level(&scope, runtime->newStrFromCStr(kReleaseLevel));
  moduleAtPutById(thread, module, ID(_version_releaselevel), release_level);

  executeFrozenModule(thread, module, bytecode);

  // Fill in hash_info.
  Object hash_width(&scope, SmallInt::fromWord(SmallInt::kBits));
  Object hash_modulus(&scope, SmallInt::fromWord(kArithmeticHashModulus));
  Object hash_inf(&scope, SmallInt::fromWord(kHashInf));
  Object hash_nan(&scope, SmallInt::fromWord(kHashNan));
  Object hash_imag(&scope, SmallInt::fromWord(kHashImag));
  Object hash_algorithm(&scope, runtime->symbols()->at(ID(siphash24)));
  Object hash_bits(&scope, SmallInt::fromWord(64));
  Object hash_seed_bits(&scope, SmallInt::fromWord(128));
  Object hash_cutoff(&scope, SmallInt::fromWord(SmallStr::kMaxLength));
  Tuple hash_info_data(&scope, runtime->newTupleWithN(
                                   9, &hash_width, &hash_modulus, &hash_inf,
                                   &hash_nan, &hash_imag, &hash_algorithm,
                                   &hash_bits, &hash_seed_bits, &hash_cutoff));
  Object hash_info(
      &scope, thread->invokeFunction1(ID(sys), ID(_HashInfo), hash_info_data));
  moduleAtPutById(thread, module, ID(hash_info), hash_info);

  runtime->cacheSysInstances(thread, module);
}

int flushStdFiles() {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  int return_value = 0;

  // PyErr_Fetch
  Object exc(&scope, thread->pendingExceptionType());
  Object val(&scope, thread->pendingExceptionValue());
  Object tb(&scope, thread->pendingExceptionTraceback());
  thread->clearPendingException();

  Runtime* runtime = thread->runtime();
  Module sys(&scope, runtime->findModuleById(ID(sys)));
  Object stdout_obj(&scope, moduleAtById(thread, sys, ID(stdout)));
  if (!stdout_obj.isErrorNotFound()) {
    if (thread->invokeMethod1(stdout_obj, ID(flush)).isErrorException()) {
      thread->clearPendingException();
      return_value = -1;
    }
  }
  Object stderr_obj(&scope, moduleAtById(thread, sys, ID(stderr)));
  if (!stderr_obj.isErrorNotFound()) {
    if (thread->invokeMethod1(stderr_obj, ID(flush)).isErrorException()) {
      thread->clearPendingException();
      return_value = -1;
    }
  }

  // PyErr_Restore
  thread->setPendingExceptionType(*exc);
  thread->setPendingExceptionValue(*val);
  thread->setPendingExceptionTraceback(*tb);
  return return_value;
}

void initializeRuntimePaths(Thread* thread) {
  HandleScope scope(thread);
  Object result(&scope, thread->invokeFunction0(ID(sys), ID(_calculate_path)));
  if (result.isError()) {
    thread->clearPendingException();
    thread->raiseBadInternalCall();
    return;
  }
  CHECK(result.isTuple(), "sys._calculate_path must return tuple");
  Tuple paths(&scope, *result);

  Str prefix(&scope, paths.at(0));
  word prefix_codepoints = prefix.codePointLength();
  std::unique_ptr<wchar_t[]> prefix_wstr(new wchar_t[prefix_codepoints + 1]);
  strCopyToWCStr(prefix_wstr.get(), prefix_codepoints + 1, prefix);
  Runtime::setPrefix(prefix_wstr.get());

  Str exec_prefix(&scope, paths.at(1));
  word exec_prefix_codepoints = exec_prefix.codePointLength();
  std::unique_ptr<wchar_t[]> exec_prefix_wstr(
      new wchar_t[exec_prefix_codepoints + 1]);
  strCopyToWCStr(exec_prefix_wstr.get(), exec_prefix_codepoints + 1,
                 exec_prefix);
  Runtime::setExecPrefix(exec_prefix_wstr.get());

  Str module_search_path(&scope, paths.at(2));
  word module_search_path_codepoints = module_search_path.codePointLength();
  std::unique_ptr<wchar_t[]> module_search_path_wstr(
      new wchar_t[module_search_path_codepoints + 1]);
  strCopyToWCStr(module_search_path_wstr.get(),
                 module_search_path_codepoints + 1, module_search_path);
  Runtime::setModuleSearchPath(module_search_path_wstr.get());
}

RawObject initializeSys(Thread* thread, const Str& executable,
                        const List& python_path, const Tuple& flags_data,
                        const List& warnoptions,
                        bool extend_python_path_with_stdlib) {
  HandleScope scope(thread);
  Object extend_python_path_with_stdlib_obj(
      &scope, Bool::fromBool(extend_python_path_with_stdlib));
  return thread->invokeFunction5(ID(sys), ID(_init), executable, python_path,
                                 flags_data, warnoptions,
                                 extend_python_path_with_stdlib_obj);
}

void setPycachePrefix(Thread* thread, const Object& pycache_prefix) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Module module(&scope, runtime->findModuleById(ID(sys)));
  moduleAtPutById(thread, module, ID(pycache_prefix), pycache_prefix);
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
      thread->invokeMethod2(file, ID(write), str).isError()) {
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

RawObject FUNC(sys, _getframe)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object depth_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfInt(*depth_obj)) {
    return thread->raiseRequiresType(depth_obj, ID(int));
  }
  word depth = intUnderlying(*depth_obj).asWordSaturated();
  if (depth < 0) {
    depth = 0;
  }
  // Increment the requested depth to skip the frame for sys._getframe itself.
  // TODO(T64005113): This should be deleted.
  depth++;
  Object result(&scope, thread->heapFrameAtDepth(depth));
  if (result.isNoneType()) {
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "call stack is not deep enough");
  }
  return *result;
}

RawObject FUNC(sys, _program_name)(Thread* thread, Arguments) {
  return newStrFromWideChar(thread, Runtime::programName());
}

RawObject FUNC(sys, excepthook)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  // type argument is ignored
  Object value(&scope, args.get(1));
  Object tb(&scope, args.get(2));
  return displayException(thread, value, tb);
}

RawObject FUNC(sys, exc_info)(Thread* thread, Arguments) {
  HandleScope scope(thread);
  Object caught_exc_state_obj(&scope, thread->topmostCaughtExceptionState());
  if (caught_exc_state_obj.isNoneType()) {
    Object type(&scope, NoneType::object());
    Object value(&scope, NoneType::object());
    Object traceback(&scope, NoneType::object());
    return thread->runtime()->newTupleWith3(type, value, traceback);
  }
  ExceptionState caught_exc_state(&scope, *caught_exc_state_obj);
  Object type(&scope, caught_exc_state.type());
  Object value(&scope, caught_exc_state.value());
  Object traceback(&scope, caught_exc_state.traceback());
  return thread->runtime()->newTupleWith3(type, value, traceback);
}

RawObject FUNC(sys, intern)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object string(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfStr(*string)) {
    return thread->raiseRequiresType(string, ID(str));
  }
  if (!string.isStr()) {
    return thread->raiseWithFmt(LayoutId::kTypeError, "can't intern %T",
                                &string);
  }
  return Runtime::internStr(thread, string);
}

RawObject FUNC(sys, getrecursionlimit)(Thread* thread, Arguments) {
  return thread->runtime()->newInt(thread->recursionLimit());
}

RawObject FUNC(sys, is_finalizing)(Thread* thread, Arguments) {
  return Bool::fromBool(thread->runtime()->isFinalizing());
}

RawObject FUNC(sys, setrecursionlimit)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Int limit(&scope, args.get(0));
  OptInt<int> opt_val = limit.asInt<int>();
  if (opt_val.error != CastError::None) {
    return thread->raiseWithFmt(LayoutId::kOverflowError,
                                "Python int too large to convert to C int");
  }

  // TODO(T62600497) Raise RecursionError if new limit is too low at current
  // recursion depth

  thread->setRecursionLimit(opt_val.value);
  return NoneType::object();
}

RawObject FUNC(sys, set_asyncgen_hooks)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object finalizer(&scope, args.get(1));
  Runtime* runtime = thread->runtime();
  Object first_iter(&scope, args.get(0));
  if (!first_iter.isUnbound()) {
    if (!first_iter.isNoneType() && !runtime->isCallable(thread, first_iter)) {
      return thread->raiseWithFmt(LayoutId::kTypeError,
                                  "callable firstiter expected, got %T",
                                  &first_iter);
    }
    thread->setAsyncgenHooksFirstIter(*first_iter);
  }
  if (!finalizer.isUnbound()) {
    if (!finalizer.isNoneType() && !runtime->isCallable(thread, finalizer)) {
      return thread->raiseWithFmt(LayoutId::kTypeError,
                                  "callable finalizer expected, got %T",
                                  &finalizer);
    }
    thread->setAsyncgenHooksFinalizer(*finalizer);
  }
  return NoneType::object();
}

}  // namespace py
