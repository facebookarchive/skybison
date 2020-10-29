#include "modules.h"

#include "builtins.h"
#include "capi.h"
#include "frozen-modules.h"
#include "globals.h"
#include "marshal.h"
#include "module-builtins.h"
#include "object-builtins.h"
#include "runtime.h"
#include "symbols.h"
#include "type-builtins.h"

namespace py {

static word builtinModuleIndex(const Str& name) {
  for (word i = 0; i < kNumFrozenModules; i++) {
    if (name.equalsCStr(kFrozenModules[i].name)) {
      return i;
    }
  }
  return -1;
}

static RawObject createBuiltinModule(Thread* thread, const Str& name) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  word builtin_index = builtinModuleIndex(name);
  if (builtin_index >= 0) {
    const FrozenModule* frozen_module = &kFrozenModules[builtin_index];
    Module module(&scope, runtime->newModule(name));
    Object modules(&scope, runtime->modules());
    Object result(&scope, objectSetItem(thread, modules, name, module));
    if (result.isErrorException()) return *result;
    ModuleInitFunc init = frozen_module->init;
    if (init == nullptr) {
      init = executeFrozenModule;
    }
    View<byte> bytecode(frozen_module->marshalled_code,
                        frozen_module->marshalled_code_length);
    init(thread, module, bytecode);
    return *module;
  }

  Object module(&scope, moduleInitBuiltinExtension(thread, name));
  if (module.isErrorException()) return *module;
  Object modules(&scope, runtime->modules());
  Object result(&scope, objectSetItem(thread, modules, name, module));
  if (result.isErrorException()) return *result;
  return *module;
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

void executeFrozenModule(Thread* thread, const Module& module,
                         View<byte> bytecode) {
  HandleScope scope(thread);
  Marshal::Reader reader(&scope, thread, bytecode);
  reader.setBuiltinFunctions(kBuiltinFunctions, kNumBuiltinFunctions);
  Str filename(&scope, module.name());
  CHECK(!reader.readPycHeader(filename).isErrorException(),
        "Failed to read %s module data", filename.toCStr());
  Code code(&scope, reader.readObject());
  Object result(&scope, executeModule(thread, code, module));
  CHECK(!result.isErrorException(), "Failed to execute %s module",
        filename.toCStr());
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
  Object modules(&scope, runtime->modules());
  Object result(&scope, objectSetItem(thread, modules, name, module));
  if (result.isErrorException()) return *result;
  result = executeModule(thread, code, module);
  if (result.isError()) return *result;
  return *module;
}

bool isFrozenModule(const Str& name) { return builtinModuleIndex(name) >= 0; }

bool isFrozenPackage(const Str& name) {
  word index = builtinModuleIndex(name);
  if (index < 0) {
    return false;
  }
  const FrozenModule* frozen_module = &kFrozenModules[index];
  return frozen_module->is_package;
}

const FrozenModule* frozenModuleByName(const Str& name) {
  word index = builtinModuleIndex(name);
  if (index < 0) {
    return nullptr;
  }
  return &kFrozenModules[index];
}

}  // namespace py
