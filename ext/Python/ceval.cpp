#include "cpython-data.h"
#include "cpython-func.h"

#include "capi-handles.h"
#include "dict-builtins.h"
#include "module-builtins.h"
#include "runtime.h"

namespace py {

PY_EXPORT PyObject* PyEval_GetBuiltins() {
  // TODO(T66852536): For full compatibility, try looking up on current frame
  // first and then use the Runtime-cached builtins
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Module builtins(&scope, runtime->findModuleById(ID(builtins)));
  return ApiHandle::borrowedReference(runtime, builtins.moduleProxy());
}

PY_EXPORT PyObject* PyEval_GetGlobals() { UNIMPLEMENTED("PyEval_GetGlobals"); }

PY_EXPORT void PyEval_AcquireLock() { UNIMPLEMENTED("PyEval_AcquireLock"); }

PY_EXPORT void PyEval_AcquireThread(PyThreadState* /* e */) {
  UNIMPLEMENTED("PyEval_AcquireThread");
}

PY_EXPORT PyObject* PyEval_EvalCode(PyObject* code, PyObject* globals,
                                    PyObject* locals) {
  Thread* thread = Thread::current();
  if (globals == nullptr) {
    thread->raiseWithFmt(LayoutId::kSystemError,
                         "PyEval_EvalCode: NULL globals");
    return nullptr;
  }
  // All of the below nullptr and type checks happen inside #ifdef Py_DEBUG in
  // CPython (PyFrame_New) but we check no matter what.
  if (code == nullptr) {
    thread->raiseBadInternalCall();
    return nullptr;
  }
  HandleScope scope(thread);
  Code code_code(&scope, ApiHandle::fromPyObject(code)->asObject());
  Runtime* runtime = thread->runtime();
  Object globals_obj(&scope, ApiHandle::fromPyObject(globals)->asObject());
  Object module_obj(&scope, NoneType::object());
  bool must_update_globals = false;
  if (globals_obj.isModuleProxy()) {
    module_obj = ModuleProxy::cast(*globals_obj).module();
  } else if (globals_obj.isDict()) {
    // Create a temporary module and fill it with the keys/values from globals
    Str empty_name(&scope, Str::empty());
    Module tmp_module(&scope, runtime->newModule(empty_name));
    Dict globals_dict(&scope, *globals_obj);
    Object key(&scope, NoneType::object());
    Object value(&scope, NoneType::object());
    Object result(&scope, NoneType::object());
    for (word i = 0; dictNextItem(globals_dict, &i, &key, &value);) {
      result = moduleAtPut(thread, tmp_module, key, value);
      if (result.isError()) return nullptr;
    }
    module_obj = *tmp_module;
    must_update_globals = true;
  } else if (runtime->isInstanceOfDict(*globals_obj)) {
    UNIMPLEMENTED("PyEval_EvalCode with dict subclass globals");
  } else {
    thread->raiseBadInternalCall();
    return nullptr;
  }

  Object implicit_globals(&scope, NoneType::object());
  if (locals != nullptr && globals != locals) {
    implicit_globals = ApiHandle::fromPyObject(locals)->asObject();
    if (!runtime->isMapping(thread, implicit_globals)) {
      thread->raiseBadInternalCall();
      return nullptr;
    }
  }
  Module module(&scope, *module_obj);
  Object result(&scope, thread->exec(code_code, module, implicit_globals));
  if (result.isErrorException()) return nullptr;
  if (must_update_globals) {
    // Update globals with the (potentially changed) contents of the module
    // proxy
    Dict globals_dict(&scope, *globals_obj);
    Object module_proxy(&scope, module.moduleProxy());
    if (dictMergeOverride(thread, globals_dict, module_proxy).isError()) {
      return nullptr;
    }
  }
  return ApiHandle::newReference(runtime, *result);
}

PY_EXPORT PyObject* PyEval_EvalCodeEx(PyObject* /* o */, PyObject* /* s */,
                                      PyObject* /* s */, PyObject** /* s */,
                                      int /* t */, PyObject** /* s */,
                                      int /* t */, PyObject** /* s */,
                                      int /* t */, PyObject* /* s */,
                                      PyObject* /* e */) {
  UNIMPLEMENTED("PyEval_EvalCodeEx");
}

PY_EXPORT PyObject* PyEval_EvalFrame(PyFrameObject* /* f */) {
  UNIMPLEMENTED("PyEval_EvalFrame");
}

PY_EXPORT PyObject* PyEval_EvalFrameEx(PyFrameObject* /* f */, int /* g */) {
  UNIMPLEMENTED("PyEval_EvalFrameEx");
}

PY_EXPORT PyFrameObject* PyEval_GetFrame() { UNIMPLEMENTED("PyEval_GetFrame"); }

PY_EXPORT const char* PyEval_GetFuncDesc(PyObject* /* c */) {
  UNIMPLEMENTED("PyEval_GetFuncDesc");
}

PY_EXPORT const char* PyEval_GetFuncName(PyObject* /* c */) {
  UNIMPLEMENTED("PyEval_GetFuncName");
}

PY_EXPORT PyObject* PyEval_GetLocals() { UNIMPLEMENTED("PyEval_GetLocals"); }

PY_EXPORT void PyEval_InitThreads() {
  // TODO(T66337218): Implement this when there is actual threading support.
}

PY_EXPORT void PyEval_ReInitThreads() {
  // TODO(T87097565): Implement instead of making it a noop
}

PY_EXPORT void PyEval_ReleaseLock() { UNIMPLEMENTED("PyEval_ReleaseLock"); }

PY_EXPORT void PyEval_ReleaseThread(PyThreadState* /* e */) {
  UNIMPLEMENTED("PyEval_ReleaseThread");
}

PY_EXPORT void PyEval_RestoreThread(PyThreadState* /* e */) {
  // TODO(T37761469): Implement instead of making it a noop
}

PY_EXPORT PyThreadState* PyEval_SaveThread() {
  // TODO(T37761469): Implement instead of making it a noop
  return nullptr;
}

PY_EXPORT int Py_AddPendingCall(int (*/* func */)(void*), void* /* g */) {
  UNIMPLEMENTED("Py_AddPendingCall");
}

PY_EXPORT int Py_GetRecursionLimit() {
  return Thread::current()->recursionLimit();
}

PY_EXPORT int Py_EnterRecursiveCall_Func(const char* where) {
  Thread* thread = Thread::current();
  if (thread->recursionEnter() >= thread->recursionLimit()) {
    thread->raiseWithFmt(LayoutId::kRecursionError,
                         "maximum recursion depth exceeded %s", where);
    return -1;
  }
  return 0;
}

PY_EXPORT void Py_LeaveRecursiveCall_Func() {
  Thread::current()->recursionLeave();
}

PY_EXPORT int Py_MakePendingCalls() { UNIMPLEMENTED("Py_MakePendingCalls"); }

PY_EXPORT int PyEval_MergeCompilerFlags(PyCompilerFlags* flags) {
  Thread* thread = Thread::current();
  Frame* frame = thread->currentFrame();
  if (!frame->isSentinel()) {
    word code_flags = Code::cast(frame->function().code()).flags();
    int compiler_flags = code_flags & PyCF_MASK;
    flags->cf_flags |= compiler_flags;
  }
  return flags->cf_flags != 0;
}

PY_EXPORT void Py_SetRecursionLimit(int limit) {
  Thread::current()->setRecursionLimit(limit);
}

PY_EXPORT int _Py_CheckRecursiveCall(const char* /* where */) {
  // We don't implement this function because this recursion checking is left
  // up to the runtime.
  return 0;
}

PY_EXPORT void PyEval_SetProfile(Py_tracefunc /* c */, PyObject* /* g */) {
  UNIMPLEMENTED("PyEval_SetProfile");
}

PY_EXPORT void PyEval_SetTrace(Py_tracefunc /* c */, PyObject* /* g */) {
  UNIMPLEMENTED("PyEval_SetTrace");
}

PY_EXPORT PyObject* PyEval_CallObjectWithKeywords(PyObject* callable,
                                                  PyObject* args,
                                                  PyObject* kwargs) {
  // PyEval_CallObjectWithKeywords() must not be called with an exception
  // set. It raises a new exception if parameters are invalid or if
  // PyTuple_New() fails, and so the original exception is lost.
  Thread* thread = Thread::current();
  DCHECK(!thread->hasPendingException(),
         "must not be called with an exception set");

  HandleScope scope(thread);
  word flags = 0;
  thread->stackPush(ApiHandle::fromPyObject(callable)->asObject());

  Runtime* runtime = thread->runtime();
  if (args != nullptr) {
    Object args_obj(&scope, ApiHandle::fromPyObject(args)->asObject());
    if (!runtime->isInstanceOfTuple(*args_obj)) {
      thread->raiseWithFmt(LayoutId::kTypeError,
                           "argument list must be a tuple");
      return nullptr;
    }
    thread->stackPush(*args_obj);
  } else {
    thread->stackPush(runtime->emptyTuple());
  }

  if (kwargs != nullptr) {
    Object kwargs_obj(&scope, ApiHandle::fromPyObject(kwargs)->asObject());
    if (!runtime->isInstanceOfDict(*kwargs_obj)) {
      thread->raiseWithFmt(LayoutId::kTypeError,
                           "keyword list must be a dictionary");
      return nullptr;
    }
    thread->stackPush(*kwargs_obj);
    flags |= CallFunctionExFlag::VAR_KEYWORDS;
  }

  // TODO(T30925218): Protect against native stack overflow.
  Object result(&scope, Interpreter::callEx(thread, flags));
  if (result.isError()) return nullptr;
  return ApiHandle::newReference(runtime, *result);
}

PY_EXPORT PyObject* _PyEval_EvalFrameDefault(PyFrameObject* /* f */,
                                             int /* g */) {
  UNIMPLEMENTED("_PyEval_EvalFrameDefault");
}

}  // namespace py
