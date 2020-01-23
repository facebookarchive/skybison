#include "under-imp-module.h"

#include "builtins-module.h"
#include "capi-handles.h"
#include "frame.h"
#include "frozen-modules.h"
#include "globals.h"
#include "module-builtins.h"
#include "modules.h"
#include "objects.h"
#include "os.h"
#include "runtime.h"

namespace py {

const BuiltinFunction UnderImpModule::kBuiltinFunctions[] = {
    {SymbolId::kAcquireLock, acquireLock},
    {SymbolId::kCreateBuiltin, createBuiltin},
    {SymbolId::kExecBuiltin, execBuiltin},
    {SymbolId::kExecDynamic, execDynamic},
    {SymbolId::kExtensionSuffixes, extensionSuffixes},
    {SymbolId::kFixCoFilename, fixCoFilename},
    {SymbolId::kGetFrozenObject, getFrozenObject},
    {SymbolId::kIsBuiltin, isBuiltin},
    {SymbolId::kIsFrozen, isFrozen},
    {SymbolId::kIsFrozenPackage, isFrozenPackage},
    {SymbolId::kReleaseLock, releaseLock},
    {SymbolId::kUnderCreateDynamic, underCreateDynamic},
    {SymbolId::kSentinelId, nullptr},
};

void UnderImpModule::initialize(Thread* thread, const Module& module) {
  moduleAddBuiltinFunctions(thread, module, kBuiltinFunctions);
  executeFrozenModule(thread, kUnderImpModuleData, module);
}

extern "C" struct _inittab _PyImport_Inittab[];

static Thread* import_lock_holder;
static word import_lock_count;

void importAcquireLock(Thread* thread) {
  if (import_lock_holder == nullptr) {
    import_lock_holder = thread;
    DCHECK(import_lock_count == 0, "count should be zero");
  }
  if (import_lock_holder == thread) {
    ++import_lock_count;
  } else {
    UNIMPLEMENTED("builtinImpAcquireLock(): thread switching not implemented");
  }
}

bool importReleaseLock(Thread* thread) {
  if (import_lock_holder != thread) {
    return false;
  }
  DCHECK(import_lock_count > 0, "count should be bigger than zero");
  --import_lock_count;
  if (import_lock_count == 0) {
    import_lock_holder = nullptr;
  }
  return true;
}

RawObject createExtensionModule(Thread* thread, const Str& name) {
  for (int i = 0; _PyImport_Inittab[i].name != nullptr; i++) {
    if (!name.equalsCStr(_PyImport_Inittab[i].name)) continue;

    HandleScope scope(thread);
    PyObject* pymodule = (*_PyImport_Inittab[i].initfunc)();
    if (pymodule == nullptr) {
      if (thread->hasPendingException()) return Error::exception();
      return thread->raiseWithFmt(LayoutId::kSystemError,
                                  "NULL return without exception set");
    };
    Runtime* runtime = thread->runtime();
    Object module_obj(&scope, ApiHandle::fromPyObject(pymodule)->asObject());
    if (!runtime->isInstanceOfModule(*module_obj)) {
      // TODO(T39542987): Enable multi-phase module initialization
      UNIMPLEMENTED("Multi-phase module initialization");
    }
    Module module(&scope, *module_obj);
    runtime->addModule(module);
    return *module;
  }
  return NoneType::object();
}

RawObject UnderImpModule::acquireLock(Thread* thread, Frame*, word) {
  importAcquireLock(thread);
  return NoneType::object();
}

RawObject UnderImpModule::createBuiltin(Thread* thread, Frame* frame,
                                        word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object spec(&scope, args.get(0));
  Object key(&scope, runtime->symbols()->Name());
  Object name_obj(&scope, getAttribute(thread, spec, key));
  DCHECK(thread->isErrorValueOk(*name_obj), "error/exception mismatch");
  if (name_obj.isError()) {
    thread->clearPendingException();
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "spec has no attribute 'name'");
  }
  if (!runtime->isInstanceOfStr(*name_obj)) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "spec name must be an instance of str");
  }
  Object existing_module(&scope, runtime->findModule(name_obj));
  if (!existing_module.isNoneType()) {
    return *existing_module;
  }

  Str name(&scope, strUnderlying(*name_obj));
  return createExtensionModule(thread, name);
}

RawObject UnderImpModule::execBuiltin(Thread* thread, Frame* frame,
                                      word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object module_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfModule(*module_obj)) {
    return runtime->newInt(0);
  }
  Module module(&scope, *module_obj);
  Object module_def_obj(&scope, module.def());
  if (!module_def_obj.isInt()) {
    CHECK(!runtime->isInstanceOfInt(*module_def_obj),
          "module_def must be an exact int as it's a C Ptr");
    return runtime->newInt(0);
  }
  Int module_def(&scope, *module_def_obj);
  PyModuleDef* def = static_cast<PyModuleDef*>(module_def.asCPtr());
  if (def == nullptr) {
    return runtime->newInt(0);
  }
  ApiHandle* mod_handle = ApiHandle::borrowedReference(thread, *module);
  if (mod_handle->cache() != nullptr) {
    return runtime->newInt(0);
  }
  return runtime->newInt(execDef(thread, module, def));
}

RawObject UnderImpModule::execDynamic(Thread* /* thread */, Frame* /* frame */,
                                      word /* nargs */) {
  UNIMPLEMENTED("exec_dynamic");
}

RawObject UnderImpModule::extensionSuffixes(Thread* thread, Frame* /* frame */,
                                            word /* nargs */) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  List list(&scope, runtime->newList());
  Object so(&scope, runtime->symbols()->DotSo());
  runtime->listAdd(thread, list, so);
  return *list;
}

RawObject UnderImpModule::fixCoFilename(Thread* /* thread */,
                                        Frame* /* frame */, word /* nargs */) {
  UNIMPLEMENTED("_fix_co_filename");
}

RawObject UnderImpModule::getFrozenObject(Thread* /* thread */,
                                          Frame* /* frame */,
                                          word /* nargs */) {
  UNIMPLEMENTED("get_frozen_object");
}

RawObject UnderImpModule::isBuiltin(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object name_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfStr(*name_obj)) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "is_builtin requires a str object");
  }
  Str name(&scope, *name_obj);

  // Special case internal runtime modules
  Symbols* symbols = runtime->symbols();
  if (name.equals(symbols->Builtins()) || name.equals(symbols->UnderThread()) ||
      name.equals(symbols->Sys()) || name.equals(symbols->UnderWeakref()) ||
      name.equals(symbols->UnderWarnings()) ||
      name.equals(symbols->Marshal())) {
    return RawSmallInt::fromWord(-1);
  }

  // Iterate the list of runtime and extension builtin modules
  for (int i = 0; _PyImport_Inittab[i].name != nullptr; i++) {
    if (name.equalsCStr(_PyImport_Inittab[i].name)) {
      return RawSmallInt::fromWord(1);
    }
  }
  return RawSmallInt::fromWord(0);
}

RawObject UnderImpModule::isFrozen(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object name(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfStr(*name)) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "is_frozen requires a str object");
  }
  // Always return False
  return RawBool::falseObj();
}

RawObject UnderImpModule::isFrozenPackage(Thread* /* thread */,
                                          Frame* /* frame */,
                                          word /* nargs */) {
  UNIMPLEMENTED("is_frozen_package");
}

RawObject UnderImpModule::releaseLock(Thread* thread, Frame*, word) {
  if (!importReleaseLock(thread)) {
    return thread->raiseWithFmt(LayoutId::kRuntimeError,
                                "not holding the import lock");
  }
  return RawNoneType::object();
}

RawObject UnderImpModule::underCreateDynamic(Thread* thread, Frame* frame,
                                             word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object name_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfStr(*name_obj)) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "create_dynamic requires a str object");
  }
  Str name(&scope, *name_obj);
  Object path_obj(&scope, args.get(1));
  if (!runtime->isInstanceOfStr(*path_obj)) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "create_dynamic requires a str object");
  }
  Str path(&scope, *path_obj);

  // Load shared object
  const char* error_msg = nullptr;
  unique_c_ptr<char> path_cstr(path.toCStr());
  void* handle = OS::openSharedObject(path_cstr.get(), &error_msg);
  if (handle == nullptr) {
    return thread->raiseWithFmt(
        LayoutId::kImportError, "dlerror: '%s' importing: '%S' from '%S'",
        error_msg != nullptr ? error_msg : "", &name, &path);
  }

  // Load PyInit_module
  const word funcname_len = 258;
  char buffer[funcname_len] = {};
  unique_c_ptr<char> name_cstr(name.toCStr());
  std::snprintf(buffer, sizeof(buffer), "PyInit_%s", name_cstr.get());
  auto init_func = reinterpret_cast<ApiHandle* (*)()>(
      OS::sharedObjectSymbolAddress(handle, buffer));
  if (init_func == nullptr) {
    return thread->raiseWithFmt(LayoutId::kImportError,
                                "dlsym error: dynamic module '%S' does not "
                                "define export function: 'PyInit_%s'",
                                &name, buffer);
  }

  // Call PyInit_module
  ApiHandle* module = init_func();
  if (module == nullptr) {
    if (!thread->hasPendingException()) {
      return thread->raiseWithFmt(
          LayoutId::kSystemError,
          "Initialization of 'PyInit_%s' failed without raising", buffer);
    }
    return Error::exception();
  }
  return module->asObject();
}

}  // namespace py
