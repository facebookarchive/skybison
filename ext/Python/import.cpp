#include "cpython-func.h"

#include "capi-handles.h"
#include "dict-builtins.h"
#include "int-builtins.h"
#include "module-builtins.h"
#include "object-builtins.h"
#include "runtime.h"
#include "under-imp-module.h"

namespace py {

PY_EXPORT PyObject* PyImport_GetModule(PyObject* name) {
  DCHECK(name != nullptr, "name is expected to be non null");
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object modules(&scope, runtime->modules());
  Object name_obj(&scope, ApiHandle::fromPyObject(name)->asObject());
  if (modules.isDict()) {
    Dict modules_dict(&scope, *modules);
    Object hash_obj(&scope, Interpreter::hash(thread, name_obj));
    if (hash_obj.isErrorException()) return nullptr;
    word hash = SmallInt::cast(*hash_obj).value();
    Object result(&scope, dictAt(thread, modules_dict, name_obj, hash));
    if (result.isError()) {
      return nullptr;
    }
    return ApiHandle::newReference(runtime, *result);
  }
  Object result(&scope, objectGetItem(thread, modules, name_obj));
  if (result.isErrorException()) {
    if (thread->pendingExceptionMatches(LayoutId::kKeyError)) {
      thread->clearPendingException();
    }
    return nullptr;
  }
  return ApiHandle::newReference(runtime, *result);
}

PY_EXPORT PyObject* PyImport_GetModuleDict(void) {
  Runtime* runtime = Thread::current()->runtime();
  return ApiHandle::borrowedReference(runtime, runtime->modules());
}

PY_EXPORT PyObject* PyImport_ImportModuleLevelObject(PyObject* name,
                                                     PyObject* globals,
                                                     PyObject* locals,
                                                     PyObject* fromlist,
                                                     int level) {
  Thread* thread = Thread::current();
  if (name == nullptr) {
    thread->raiseWithFmt(LayoutId::kValueError, "Empty module name");
    return nullptr;
  }
  if (level < 0) {
    thread->raiseWithFmt(LayoutId::kValueError, "level must be >= 0");
    return nullptr;
  }
  if (globals == nullptr) {
    thread->raiseWithFmt(LayoutId::kKeyError, "'__name__' not in globals");
    return nullptr;
  }
  HandleScope scope(thread);
  Object globals_obj(&scope, ApiHandle::fromPyObject(globals)->asObject());
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfDict(*globals_obj)) {
    thread->raiseWithFmt(LayoutId::kTypeError, "globals must be a dict");
    return nullptr;
  }

  Object level_obj(&scope, SmallInt::fromWord(level));
  Object fromlist_obj(&scope,
                      fromlist == nullptr
                          ? runtime->emptyTuple()
                          : ApiHandle::fromPyObject(fromlist)->asObject());

  Object name_obj(&scope, ApiHandle::fromPyObject(name)->asObject());
  Object locals_obj(&scope, locals == nullptr
                                ? NoneType::object()
                                : ApiHandle::fromPyObject(locals)->asObject());
  Object result(&scope, thread->invokeFunction5(
                            ID(_frozen_importlib), ID(__import__), name_obj,
                            globals_obj, locals_obj, fromlist_obj, level_obj));
  if (result.isError()) return nullptr;
  return ApiHandle::newReference(runtime, *result);
}

PY_EXPORT PyObject* PyImport_AddModule(const char* name) {
  PyObject* pyname = PyUnicode_FromString(name);
  if (!pyname) return nullptr;
  PyObject* module = PyImport_AddModuleObject(pyname);
  Py_DECREF(pyname);
  return module;
}

PY_EXPORT PyObject* PyImport_AddModuleObject(PyObject* name) {
  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Dict modules_dict(&scope, runtime->modules());
  Object name_obj(&scope, ApiHandle::fromPyObject(name)->asObject());
  Object hash_obj(&scope, Interpreter::hash(thread, name_obj));
  if (hash_obj.isErrorException()) return nullptr;
  word hash = SmallInt::cast(*hash_obj).value();
  Object module(&scope, dictAt(thread, modules_dict, name_obj, hash));
  if (!module.isErrorNotFound()) {
    return ApiHandle::borrowedReference(runtime, *module);
  }

  Module new_module(&scope, runtime->newModule(name_obj));
  Object modules(&scope, runtime->modules());
  if (objectSetItem(thread, modules, name_obj, new_module).isErrorException()) {
    return nullptr;
  }
  return ApiHandle::borrowedReference(runtime, *new_module);
}

PY_EXPORT void PyImport_Cleanup() { UNIMPLEMENTED("PyImport_Cleanup"); }

PY_EXPORT PyObject* PyImport_ExecCodeModule(const char* /* e */,
                                            PyObject* /* o */) {
  UNIMPLEMENTED("PyImport_ExecCodeModule");
}

PY_EXPORT PyObject* PyImport_ExecCodeModuleEx(const char* /* e */,
                                              PyObject* /* o */,
                                              const char* /* e */) {
  UNIMPLEMENTED("PyImport_ExecCodeModuleEx");
}

PY_EXPORT PyObject* PyImport_ExecCodeModuleObject(PyObject* /* e */,
                                                  PyObject* /* o */,
                                                  PyObject* /* e */,
                                                  PyObject* /* e */) {
  UNIMPLEMENTED("PyImport_ExecCodeModuleObject");
}

PY_EXPORT PyObject* PyImport_ExecCodeModuleWithPathnames(const char* /* e */,
                                                         PyObject* /* o */,
                                                         const char* /* e */,
                                                         const char* /* e */) {
  UNIMPLEMENTED("PyImport_ExecCodeModuleWithPathnames");
}

PY_EXPORT long PyImport_GetMagicNumber() {
  PyObject* importlib = PyImport_ImportModule("_frozen_importlib_external");
  if (importlib == nullptr) return -1;
  PyObject* pyc_magic = PyObject_GetAttrString(importlib, "_RAW_MAGIC_NUMBER");
  Py_DECREF(importlib);
  if (pyc_magic == nullptr) return -1;
  long res = PyLong_AsLong(pyc_magic);
  Py_DECREF(pyc_magic);
  return res;
}

PY_EXPORT const char* PyImport_GetMagicTag() {
  UNIMPLEMENTED("PyImport_GetMagicTag");
}

PY_EXPORT PyObject* PyImport_Import(PyObject* module_name) {
  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Object name_obj(&scope, ApiHandle::fromPyObject(module_name)->asObject());
  Object globals_obj(&scope, NoneType::object());
  Frame* current_frame = thread->currentFrame();
  if (current_frame->isSentinel()) {
    globals_obj = runtime->newDict();
  } else {
    Module module(&scope, current_frame->function().moduleObject());
    globals_obj = module.moduleProxy();
  }
  Object fromlist_obj(&scope, runtime->emptyTuple());
  Object level_obj(&scope, SmallInt::fromWord(0));
  Object result(&scope, thread->invokeFunction5(
                            ID(builtins), ID(__import__), name_obj, globals_obj,
                            globals_obj, fromlist_obj, level_obj));
  if (result.isError()) return nullptr;
  // PyImport_GetModule, inlined
  Object modules(&scope, thread->runtime()->modules());
  if (modules.isDict()) {
    Dict modules_dict(&scope, *modules);
    Object hash_obj(&scope, Interpreter::hash(thread, name_obj));
    if (hash_obj.isErrorException()) return nullptr;
    word hash = SmallInt::cast(*hash_obj).value();
    result = dictAt(thread, modules_dict, name_obj, hash);
    // dictAt can return Error::notFound, and we should not raise an exception
    // in that case
    if (result.isError()) return nullptr;
    return ApiHandle::newReference(runtime, *result);
  }
  result = objectGetItem(thread, modules, name_obj);
  // Don't filter out KeyError; PyImport_GetModule clears them, but
  // PyImport_Import re-raises
  if (result.isErrorException()) return nullptr;
  return ApiHandle::newReference(runtime, *result);
}

PY_EXPORT int PyImport_ImportFrozenModule(const char* name) {
  // CPython's and Pyro's frozen modules do not match. Instead, just treat
  // this C-API as PyImport_ImportModule
  PyObject* result = PyImport_ImportModule(name);
  Py_XDECREF(result);
  return result == nullptr ? -1 : 0;
}

PY_EXPORT int PyImport_ImportFrozenModuleObject(PyObject* name) {
  // CPython's and Pyro's frozen modules do not match. Instead, just treat
  // this C-API as PyImport_ImportModule
  PyObject* result = PyImport_Import(name);
  Py_XDECREF(result);
  return result == nullptr ? -1 : 0;
}

PY_EXPORT PyObject* PyImport_ImportModule(const char* name) {
  PyObject* name_obj = PyUnicode_FromString(name);
  if (name_obj == nullptr) return nullptr;
  PyObject* result = PyImport_Import(name_obj);
  Py_DECREF(name_obj);
  return result;
}

PY_EXPORT PyObject* PyImport_ImportModuleLevel(const char* name,
                                               PyObject* globals,
                                               PyObject* locals,
                                               PyObject* fromlist, int level) {
  PyObject* name_obj = PyUnicode_FromString(name);
  if (name_obj == nullptr) return nullptr;
  PyObject* result = PyImport_ImportModuleLevelObject(name_obj, globals, locals,
                                                      fromlist, level);
  Py_DECREF(name_obj);
  return result;
}

PY_EXPORT PyObject* PyImport_ImportModuleNoBlock(const char* name) {
  // Deprecated in favor of PyImport_ImportModule. From the docs:
  // "Since Python 3.3, this function's special behaviour isn't needed anymore"
  return PyImport_ImportModule(name);
}

PY_EXPORT PyObject* PyImport_ReloadModule(PyObject* /* m */) {
  UNIMPLEMENTED("PyImport_ReloadModule");
}

PY_EXPORT void _PyImport_AcquireLock() { importAcquireLock(Thread::current()); }

PY_EXPORT void _PyImport_ReInitLock() {
  // TODO(T87097565): Implement instead of making it a noop
}

PY_EXPORT int _PyImport_ReleaseLock() {
  return importReleaseLock(Thread::current()) ? 1 : -1;
}

}  // namespace py
