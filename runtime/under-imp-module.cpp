#include "under-imp-module.h"

#include "builtins-module.h"
#include "builtins.h"
#include "capi-handles.h"
#include "capi.h"
#include "frame.h"
#include "globals.h"
#include "module-builtins.h"
#include "modules.h"
#include "objects.h"
#include "os.h"
#include "runtime.h"

namespace py {

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

RawObject FUNC(_imp, _create_dynamic)(Thread* thread, Frame* frame,
                                      word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object name_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfStr(*name_obj)) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "create_dynamic requires a str object");
  }
  Str name(&scope, strUnderlying(*name_obj));
  Object path_obj(&scope, args.get(1));
  if (!runtime->isInstanceOfStr(*path_obj)) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "create_dynamic requires a str object");
  }
  Str path(&scope, strUnderlying(*path_obj));

  return moduleLoadDynamicExtension(thread, name, path);
}

RawObject FUNC(_imp, acquire_lock)(Thread* thread, Frame*, word) {
  importAcquireLock(thread);
  return NoneType::object();
}

RawObject FUNC(_imp, create_builtin)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object spec(&scope, args.get(0));
  Object key(&scope, runtime->symbols()->at(ID(name)));
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
  Str name(&scope, strUnderlying(*name_obj));
  name = Runtime::internStr(thread, name);
  Object result(&scope, ensureBuiltinModule(thread, name));
  if (result.isErrorNotFound()) return NoneType::object();
  return *result;
}

RawObject FUNC(_imp, exec_builtin)(Thread* thread, Frame* frame, word nargs) {
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

RawObject FUNC(_imp, is_builtin)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object name_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfStr(*name_obj)) {
    return thread->raiseRequiresType(name_obj, ID(str));
  }
  Str name(&scope, strUnderlying(*name_obj));
  name = Runtime::internStr(thread, name);
  bool result = isFrozenModule(name) || isBuiltinExtensionModule(name);
  return SmallInt::fromWord(result ? 1 : 0);
}

RawObject FUNC(_imp, is_frozen)(Thread* thread, Frame* frame, word nargs) {
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

RawObject FUNC(_imp, lock_held)(Thread*, Frame*, word) {
  return Bool::fromBool(import_lock_holder != nullptr);
}

RawObject FUNC(_imp, release_lock)(Thread* thread, Frame*, word) {
  if (!importReleaseLock(thread)) {
    return thread->raiseWithFmt(LayoutId::kRuntimeError,
                                "not holding the import lock");
  }
  return RawNoneType::object();
}

RawObject FUNC(_imp, source_hash)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object key_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfInt(*key_obj)) {
    return thread->raiseRequiresType(key_obj, ID(int));
  }
  Int key_int(&scope, intUnderlying(*key_obj));
  if (key_int.numDigits() > 1) {
    return thread->raiseWithFmt(LayoutId::kOverflowError,
                                "Python int too large to convert to C long");
  }
  word key = key_int.asWord();
  Object source_obj(&scope, args.get(1));
  if (!runtime->isByteslike(*source_obj)) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "a bytes-like object is required, not '%T'",
                                &source_obj);
  }
  if (!runtime->isInstanceOfBytes(*source_obj)) {
    // TODO(T38246066): support bytes-like objects other than bytes
    UNIMPLEMENTED("bytes-like other than bytes");
  }
  Bytes source(&scope, bytesUnderlying(*source_obj));
  uint64_t hash = Runtime::hashWithKey(source, key);
  if (endian::native == endian::big) {
    hash = __builtin_bswap64(hash);
  }
  return runtime->newBytesWithAll(
      View<byte>(reinterpret_cast<byte*>(&hash), sizeof(hash)));
}

}  // namespace py
