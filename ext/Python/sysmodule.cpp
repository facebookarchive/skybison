#include <cstdarg>

#include "capi-handles.h"
#include "cpython-data.h"
#include "module-builtins.h"
#include "runtime.h"
#include "sys-module.h"

namespace py {

PY_EXPORT size_t _PySys_GetSizeOf(PyObject* o) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object obj(&scope, ApiHandle::fromPyObject(o)->asObject());
  Object result_obj(&scope, thread->invokeFunction1(SymbolId::kSys,
                                                    SymbolId::kGetSizeOf, obj));
  if (result_obj.isError()) {
    // Pass through a pending exception if any exists.
    return static_cast<size_t>(-1);
  }
  DCHECK(thread->runtime()->isInstanceOfInt(*result_obj),
         "sys.getsizeof() should return an int");
  Int result(&scope, intUnderlying(*result_obj));
  return static_cast<size_t>(result.asWord());
}

PY_EXPORT PyObject* PySys_GetObject(const char* name) {
  DCHECK(name != nullptr, "name must not be nullptr");
  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Module module(&scope, runtime->findModuleById(SymbolId::kSys));
  Object name_obj(&scope, Runtime::internStrFromCStr(thread, name));
  Object result(&scope, moduleAt(thread, module, name_obj));
  if (result.isErrorNotFound()) return nullptr;
  return ApiHandle::borrowedReference(thread, *result);
}

PY_EXPORT void PySys_AddWarnOption(const wchar_t* /* s */) {
  UNIMPLEMENTED("PySys_AddWarnOption");
}

PY_EXPORT void PySys_AddWarnOptionUnicode(PyObject* /* n */) {
  UNIMPLEMENTED("PySys_AddWarnOptionUnicode");
}

PY_EXPORT void PySys_AddXOption(const wchar_t* /* s */) {
  UNIMPLEMENTED("PySys_AddXOption");
}

PY_EXPORT void PySys_FormatStderr(const char* /* t */, ...) {
  UNIMPLEMENTED("PySys_FormatStderr");
}

PY_EXPORT void PySys_FormatStdout(const char* /* t */, ...) {
  UNIMPLEMENTED("PySys_FormatStdout");
}

PY_EXPORT PyObject* PySys_GetXOptions() { UNIMPLEMENTED("PySys_GetXOptions"); }

PY_EXPORT int PySys_HasWarnOptions() { UNIMPLEMENTED("PySys_HasWarnOptions"); }

PY_EXPORT void PySys_ResetWarnOptions() {
  UNIMPLEMENTED("PySys_ResetWarnOptions");
}

PY_EXPORT void PySys_SetArgv(int argc, wchar_t** argv) {
  PySys_SetArgvEx(argc, argv, Py_IsolatedFlag == 0);
}

PY_EXPORT void PySys_SetArgvEx(int argc, wchar_t** argv, int updatepath) {
  CHECK(argc >= 0, "Unexpected argc");

  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  List args(&scope, runtime->newList());

  Str arg(&scope, Str::empty());
  if (argc == 0 || argv == nullptr) {
    // Ensure at least one (empty) argument is seen
    runtime->listAdd(thread, args, arg);
  } else {
    for (int i = 0; i < argc; i++) {
      char* arg_cstr = Py_EncodeLocale(argv[i], nullptr);
      arg = runtime->newStrFromCStr(arg_cstr);
      PyMem_Free(arg_cstr);
      runtime->listAdd(thread, args, arg);
    }
  }
  Module sys_module(&scope, runtime->findModuleById(SymbolId::kSys));
  moduleAtPutById(thread, sys_module, SymbolId::kArgv, args);

  if (updatepath == 0) {
    return;
  }

  arg = args.at(0);
  Object result(&scope, thread->invokeFunction1(
                            SymbolId::kSys, SymbolId::kUnderUpdatePath, arg));
  CHECK(!result.isError(), "Error updating sys.path from argv[0]");
}

PY_EXPORT int PySys_SetObject(const char* /* e */, PyObject* /* v */) {
  UNIMPLEMENTED("PySys_SetObject");
}

PY_EXPORT void PySys_SetPath(const wchar_t* /* h */) {
  UNIMPLEMENTED("PySys_SetPath");
}

PY_EXPORT void PySys_WriteStderr(const char* format, ...) {
  va_list va;
  va_start(va, format);
  writeStderrV(Thread::current(), format, va);
  va_end(va);
}

PY_EXPORT void PySys_WriteStdout(const char* format, ...) {
  va_list va;
  va_start(va, format);
  writeStdoutV(Thread::current(), format, va);
  va_end(va);
}

}  // namespace py
