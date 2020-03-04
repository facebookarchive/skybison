#include "modules.h"

#include "array-module.h"
#include "builtins-module.h"
#include "builtins.h"
#include "capi-handles.h"
#include "frozen-modules.h"
#include "globals.h"
#include "marshal.h"
#include "module-builtins.h"
#include "runtime.h"
#include "symbols.h"
#include "sys-module.h"
#include "type-builtins.h"
#include "under-builtins-module.h"
#include "under-io-module.h"
#include "under-signal-module.h"
#include "under-weakref-module.h"

namespace py {

extern "C" struct _inittab _PyImport_Inittab[];

template <const FrozenModule* data>
static void initializeFrozenModule(Thread* thread, const Module& module) {
  executeFrozenModule(thread, data, module);
}

const ModuleInitializer kBuiltinModules[] = {
    {ID(_builtins), &UnderBuiltinsModule::initialize},
    {ID(_codecs), &initializeFrozenModule<&kUnderCodecsModuleData>},
    {ID(_frozen_importlib),
     &initializeFrozenModule<&kUnderBootstrapModuleData>},
    {ID(_frozen_importlib_external),
     &initializeFrozenModule<&kUnderBootstrapExternalModuleData>},
    {ID(_imp), &initializeFrozenModule<&kUnderImpModuleData>},
    {ID(_io), &UnderIoModule::initialize},
    {ID(_os), &initializeFrozenModule<&kUnderOsModuleData>},
    {ID(_path), &initializeFrozenModule<&kUnderPathModuleData>},
    {ID(_signal), &UnderSignalModule::initialize},
    {ID(_str_mod), &initializeFrozenModule<&kUnderStrModModuleData>},
    {ID(_thread), &initializeFrozenModule<&kUnderThreadModuleData>},
    {ID(_valgrind), &initializeFrozenModule<&kUnderValgrindModuleData>},
    {ID(_warnings), &initializeFrozenModule<&kUnderWarningsModuleData>},
    {ID(_weakref), &UnderWeakrefModule::initialize},
    {ID(array), &ArrayModule::initialize},
    {ID(builtins), &BuiltinsModule::initialize},
    {ID(faulthandler), &initializeFrozenModule<&kFaulthandlerModuleData>},
    {ID(marshal), &initializeFrozenModule<&kMarshalModuleData>},
    {ID(operator), &initializeFrozenModule<&kOperatorModuleData>},
    {ID(readline), &initializeFrozenModule<&kReadlineModuleData>},
    {ID(sys), &SysModule::initialize},
    {ID(warnings), &initializeFrozenModule<&kWarningsModuleData>},
    {SymbolId::kSentinelId, nullptr},
};

static void checkBuiltinTypeDeclarations(Thread* thread, const Module& module) {
  // Ensure builtin types have been declared.
  HandleScope scope(thread);
  List values(&scope, moduleValues(thread, module));
  Object value(&scope, NoneType::object());
  Runtime* runtime = thread->runtime();
  for (word i = 0, num_items = values.numItems(); i < num_items; i++) {
    value = values.at(i);
    if (!runtime->isInstanceOfType(*value)) continue;
    Type type(&scope, *value);
    if (!type.isBuiltin()) continue;
    // Check whether __doc__ exists as a signal that the type was declared.
    if (!typeAtById(thread, type, ID(__doc__)).isErrorNotFound()) {
      continue;
    }
    Str name(&scope, type.name());
    unique_c_ptr<char> name_cstr(name.toCStr());
    Str module_name(&scope, module.name());
    unique_c_ptr<char> module_name_cstr(module_name.toCStr());
    DCHECK(false, "Builtin type %s.%s not defined", module_name_cstr.get(),
           name_cstr.get());
  }
}

static word extensionModuleIndex(const Str& name) {
  for (word i = 0; _PyImport_Inittab[i].name != nullptr; i++) {
    if (name.equalsCStr(_PyImport_Inittab[i].name)) {
      return i;
    }
  }
  return -1;
}

static word builtinModuleIndex(Thread* thread, const Str& name) {
  DCHECK(Runtime::isInternedStr(thread, name), "expected interned str");
  Runtime* runtime = thread->runtime();
  for (word i = 0; kBuiltinModules[i].name != SymbolId::kSentinelId; i++) {
    if (runtime->symbols()->at(kBuiltinModules[i].name) == name) {
      return i;
    }
  }
  return -1;
}

static RawObject createBuiltinModule(Thread* thread, const Str& name) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  word builtin_index = builtinModuleIndex(thread, name);
  if (builtin_index >= 0) {
    SymbolId id = kBuiltinModules[builtin_index].name;
    Module module(&scope, runtime->createModule(thread, id));
    kBuiltinModules[builtin_index].init(thread, module);
    return *module;
  }

  word extension_index = extensionModuleIndex(name);
  if (extension_index >= 0) {
    PyObject* pymodule = (*_PyImport_Inittab[extension_index].initfunc)();
    if (pymodule == nullptr) {
      if (thread->hasPendingException()) return Error::exception();
      return thread->raiseWithFmt(LayoutId::kSystemError,
                                  "NULL return without exception set");
    };
    Object module_obj(&scope, ApiHandle::fromPyObject(pymodule)->asObject());
    if (!runtime->isInstanceOfModule(*module_obj)) {
      // TODO(T39542987): Enable multi-phase module initialization
      UNIMPLEMENTED("Multi-phase module initialization");
    }
    Module module(&scope, *module_obj);
    runtime->addModule(module);
    return *module;
  }

  return Error::notFound();
}

RawObject ensureBuiltinModule(Thread* thread, const Str& name) {
  DCHECK(Runtime::isInternedStr(thread, name), "expected interned str");
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Object result(&scope, runtime->findModule(name));
  if (!result.isErrorNotFound()) return *result;
  return createBuiltinModule(thread, name);
}

RawObject ensureBuiltinModuleById(Thread* thread, SymbolId id) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Object result(&scope, runtime->findModuleById(id));
  if (!result.isErrorNotFound()) return *result;
  Str name(&scope, runtime->symbols()->at(id));
  return createBuiltinModule(thread, name);
}

void executeFrozenModule(Thread* thread, const FrozenModule* frozen_module,
                         const Module& module) {
  HandleScope scope(thread);
  View<byte> data(reinterpret_cast<const byte*>(frozen_module->marshalled_code),
                  frozen_module->marshalled_code_length);
  Marshal::Reader reader(&scope, thread->runtime(), data);
  reader.setBuiltinFunctions(kBuiltinFunctions, kNumBuiltinFunctions);
  Str filename(&scope, module.name());
  CHECK(!reader.readPycHeader(filename).isErrorException(),
        "Failed to read %s module data", filename.toCStr());
  Code code(&scope, reader.readObject());
  Object result(&scope, executeModule(thread, code, module));
  CHECK(!result.isErrorException(), "Failed to execute %s module",
        filename.toCStr());
  if (DCHECK_IS_ON()) {
    checkBuiltinTypeDeclarations(thread, module);
  }
}

RawObject executeModule(Thread* thread, const Code& code,
                        const Module& module) {
  HandleScope scope(thread);
  DCHECK(code.argcount() == 0, "invalid argcount %ld", code.argcount());
  Object none(&scope, NoneType::object());
  return thread->exec(code, module, none);
}

RawObject executeModuleFromCode(Thread* thread, const Code& code,
                                const Object& name) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Module module(&scope, runtime->newModule(name));
  runtime->addModule(module);
  Object result(&scope, executeModule(thread, code, module));
  if (result.isError()) return *result;
  return *module;
}

bool isBuiltinModule(Thread* thread, const Str& name) {
  return builtinModuleIndex(thread, name) >= 0 ||
         extensionModuleIndex(name) >= 0;
}

void moduleAddBuiltinTypes(Thread* thread, const Module& module,
                           const BuiltinType* types) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object type(&scope, NoneType::object());
  for (word i = 0; types[i].name != SymbolId::kSentinelId; i++) {
    type = runtime->typeAt(types[i].type);
    moduleAtPutById(thread, module, types[i].name, type);
  }
}

}  // namespace py
