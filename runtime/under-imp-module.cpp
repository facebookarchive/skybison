#include "under-imp-module.h"

#include "builtins-module.h"
#include "builtins.h"
#include "capi.h"
#include "frame.h"
#include "globals.h"
#include "marshal.h"
#include "module-builtins.h"
#include "modules.h"
#include "objects.h"
#include "os.h"
#include "runtime.h"
#include "str-builtins.h"

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

RawObject FUNC(_imp, _import_fastpath)(Thread* thread, Arguments args) {
  // This functions attempts to shortcut the import mechanics for modules that
  // have already been loaded. It is called directly from
  // _frozen_import_lib.__import__ and reproduces the functionality for the
  // case when the module is present in sys.modules.
  HandleScope scope(thread);
  Object level(&scope, args.get(2));
  // Only handle top level imports to avoid doing import name resolution.
  if (level != SmallInt::fromWord(0)) {
    return NoneType::object();
  }
  Object name(&scope, args.get(0));
  if (!name.isStr()) {
    return NoneType::object();
  }
  Str name_str(&scope, *name);
  Runtime* runtime = thread->runtime();

  Object result(&scope, runtime->findModule(name_str));
  if (!result.isModule()) {
    return NoneType::object();
  }

  // For imports with the style `from module import x,y,z` we need to verify
  // that x, y, and z have all been loaded. This is the same checks as in
  // _frozen_importlib._handle_fromlist.
  Object fromlist_obj(&scope, args.get(1));
  if (!fromlist_obj.isNoneType()) {
    word fromlist_length;
    if (fromlist_obj.isList()) {
      fromlist_length = List::cast(*fromlist_obj).numItems();
      fromlist_obj = List::cast(*fromlist_obj).items();
    } else if (fromlist_obj.isTuple()) {
      fromlist_length = Tuple::cast(*fromlist_obj).length();
    } else {
      return NoneType::object();
    }
    Tuple fromlist(&scope, *fromlist_obj);

    if (fromlist_length > 0) {
      Module module(&scope, *result);
      Object attribute_name(&scope, NoneType::object());
      Object attribute(&scope, NoneType::object());
      for (word i = 0; i < fromlist_length; i++) {
        attribute_name = fromlist.at(i);
        if (!attribute_name.isStr()) {
          return NoneType::object();
        }
        attribute_name = Runtime::internStr(thread, attribute_name);
        attribute = moduleAt(module, attribute_name);
        if (attribute.isErrorNotFound()) {
          return NoneType::object();
        }
      }
      return *result;
    }
  }
  // If fromlist is empty return the top level package.
  // ie: `__import__("module.x")` will return `module`
  word index = strFindAsciiChar(name_str, '.');
  if (index == -1) {
    return *result;
  }
  name = strSubstr(thread, name_str, 0, index);
  result = runtime->findModule(name);
  if (!result.isModule()) {
    return NoneType::object();
  }
  return *result;
}

RawObject FUNC(_imp, _create_dynamic)(Thread* thread, Arguments args) {
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

RawObject FUNC(_imp, acquire_lock)(Thread* thread, Arguments) {
  importAcquireLock(thread);
  return NoneType::object();
}

RawObject FUNC(_imp, create_builtin)(Thread* thread, Arguments args) {
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

RawObject FUNC(_imp, exec_builtin)(Thread* thread, Arguments args) {
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
  if (objectHasHandleCache(thread, *module)) {
    return runtime->newInt(0);
  }
  return runtime->newInt(moduleExecDef(thread, module, def));
}

RawObject FUNC(_imp, get_frozen_object)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object name_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfStr(*name_obj)) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "get_frozen_object requires a str object");
  }
  Str name(&scope, strUnderlying(*name_obj));
  const FrozenModule* frozen_module = frozenModuleByName(name);
  if (frozen_module == nullptr) {
    return thread->raiseWithFmt(LayoutId::kImportError,
                                "No such frozen object named '%S'", &name);
  }
  DCHECK(frozen_module->marshalled_code != nullptr,
         "null code in frozen module");
  word size = frozen_module->marshalled_code_length;
  Marshal::Reader reader(&scope, thread,
                         View<byte>(frozen_module->marshalled_code, size));
  // We don't write pyc headers for frozen modules because it would make
  // bootstrapping tricky. Don't read the pyc header.
  return reader.readObject();
}

RawObject FUNC(_imp, is_builtin)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object name_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfStr(*name_obj)) {
    return thread->raiseRequiresType(name_obj, ID(str));
  }
  Str name(&scope, strUnderlying(*name_obj));
  name = Runtime::internStr(thread, name);
  if (isFrozenPackage(name)) return SmallInt::fromWord(0);
  bool result = isFrozenModule(name) || isBuiltinExtensionModule(name);
  return SmallInt::fromWord(result ? 1 : 0);
}

RawObject FUNC(_imp, is_frozen)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object name_obj(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfStr(*name_obj)) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "is_frozen requires a str object");
  }
  Str name(&scope, strUnderlying(*name_obj));
  name = Runtime::internStr(thread, name);
  return Bool::fromBool(isFrozenModule(name));
}

RawObject FUNC(_imp, is_frozen_package)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object name_obj(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfStr(*name_obj)) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "is_frozen requires a str object");
  }
  Str name(&scope, strUnderlying(*name_obj));
  name = Runtime::internStr(thread, name);
  return Bool::fromBool(isFrozenPackage(name));
}

RawObject FUNC(_imp, lock_held)(Thread*, Arguments) {
  return Bool::fromBool(import_lock_holder != nullptr);
}

RawObject FUNC(_imp, release_lock)(Thread* thread, Arguments) {
  if (!importReleaseLock(thread)) {
    return thread->raiseWithFmt(LayoutId::kRuntimeError,
                                "not holding the import lock");
  }
  return RawNoneType::object();
}

RawObject FUNC(_imp, source_hash)(Thread* thread, Arguments args) {
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
