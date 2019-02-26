#include "imp-module.h"

#include "builtins-module.h"
#include "frame.h"
#include "globals.h"
#include "module-builtins.h"
#include "objects.h"
#include "runtime.h"

namespace python {

extern "C" struct _inittab _PyImport_Inittab[];

static Thread* import_lock_holder;
static word import_lock_count;

RawObject builtinImpAcquireLock(Thread* thread, Frame* /* frame */,
                                word /* nargs */) {
  if (import_lock_holder == nullptr) {
    import_lock_holder = thread;
    DCHECK(import_lock_count == 0, "count should be zero");
  }
  if (import_lock_holder == thread) {
    ++import_lock_count;
  } else {
    UNIMPLEMENTED("builtinImpAcquireLock(): thread switching not implemented");
  }
  return RawNoneType::object();
}

RawObject builtinImpCreateBuiltin(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object spec(&scope, args.get(0));
  Object key(&scope, runtime->symbols()->Name());
  Object name_obj(&scope, getAttribute(thread, spec, key));
  if (name_obj.isError()) {
    return thread->raiseTypeErrorWithCStr("spec has no attribute 'name'");
  }
  if (!runtime->isInstanceOfStr(*name_obj)) {
    return thread->raiseTypeErrorWithCStr(
        "spec name must be an instance of str");
  }
  Str name(&scope, *name_obj);
  for (int i = 0; _PyImport_Inittab[i].name != nullptr; i++) {
    if (name.equalsCStr(_PyImport_Inittab[i].name)) {
      PyObject* pymodule = (*_PyImport_Inittab[i].initfunc)();
      if (pymodule == nullptr) {
        if (thread->hasPendingException()) return Error::object();
        return thread->raiseSystemErrorWithCStr(
            "NULL return without exception set");
      };
      Object module_obj(&scope, ApiHandle::fromPyObject(pymodule)->asObject());
      if (!module_obj.isModule()) {
        // TODO(T39542987): Enable multi-phase module initialization
        UNIMPLEMENTED("Multi-phase module initialization");
      }
      Module module(&scope, *module_obj);
      runtime->addModule(module);
      return *module;
    }
  }
  return NoneType::object();
}

RawObject builtinImpExecBuiltin(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object module_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfModule(*module_obj)) {
    return runtime->newInt(0);
  }
  Module module(&scope, *module_obj);
  Object module_def_obj(&scope, module.def());
  if (!runtime->isInstanceOfInt(*module_def_obj)) {
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

RawObject builtinImpExecDynamic(Thread* /* thread */, Frame* /* frame */,
                                word /* nargs */) {
  UNIMPLEMENTED("exec_dynamic");
}

RawObject builtinImpExtensionSuffixes(Thread* thread, Frame* /* frame */,
                                      word /* nargs */) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  List list(&scope, runtime->newList());
  Object so(&scope, runtime->symbols()->DotSo());
  runtime->listAdd(list, so);
  return *list;
}

RawObject builtinImpFixCoFilename(Thread* /* thread */, Frame* /* frame */,
                                  word /* nargs */) {
  UNIMPLEMENTED("_fix_co_filename");
}

RawObject builtinImpGetFrozenObject(Thread* /* thread */, Frame* /* frame */,
                                    word /* nargs */) {
  UNIMPLEMENTED("get_frozen_object");
}

RawObject builtinImpIsBuiltin(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object name_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfStr(*name_obj)) {
    return thread->raiseTypeErrorWithCStr("is_builtin requires a str object");
  }
  Str name(&scope, *name_obj);

  // Special case internal runtime modules
  Symbols* symbols = runtime->symbols();
  if (name.equals(symbols->Builtins()) || name.equals(symbols->UnderThread()) ||
      name.equals(symbols->Sys()) || name.equals(symbols->UnderWeakRef())) {
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

RawObject builtinImpIsFrozen(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object name(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfStr(*name)) {
    return thread->raiseTypeErrorWithCStr("is_frozen requires a str object");
  }
  // Always return False
  return RawBool::falseObj();
}

RawObject builtinImpIsFrozenPackage(Thread* /* thread */, Frame* /* frame */,
                                    word /* nargs */) {
  UNIMPLEMENTED("is_frozen_package");
}

RawObject builtinImpReleaseLock(Thread* thread, Frame* /* frame */,
                                word /* nargs */) {
  if (import_lock_holder == nullptr) {
    return thread->raiseRuntimeErrorWithCStr("not holding the import lock");
  }
  DCHECK(import_lock_count > 0, "count should be bigger than zero");
  --import_lock_count;
  if (import_lock_count == 0) {
    import_lock_holder = nullptr;
  }
  return RawNoneType::object();
}

}  // namespace python
