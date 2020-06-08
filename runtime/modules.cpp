#include "modules.h"

#include "builtins.h"
#include "frozen-modules.h"
#include "globals.h"
#include "marshal.h"
#include "module-builtins.h"
#include "runtime.h"
#include "symbols.h"
#include "type-builtins.h"

namespace py {

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
    runtime->addModule(thread, module);
    ModuleInitFunc init = frozen_module->init;
    if (init == nullptr) {
      init = executeFrozenModule;
    }
    View<byte> bytecode(frozen_module->marshalled_code,
                        frozen_module->marshalled_code_length);
    init(thread, module, bytecode);
    return *module;
  }

  return moduleInitBuiltinExtension(thread, name);
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
  runtime->addModule(thread, module);
  Object result(&scope, executeModule(thread, code, module));
  if (result.isError()) return *result;
  return *module;
}

bool isFrozenModule(const Str& name) { return builtinModuleIndex(name) >= 0; }

void moduleAddBuiltinTypes(Thread* thread, const Module& module,
                           View<BuiltinType> types) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object type(&scope, NoneType::object());
  for (word i = 0, length = types.length(); i < length; i++) {
    type = runtime->typeAt(types.get(i).type);
    moduleAtPutById(thread, module, types.get(i).name, type);
  }
}

}  // namespace py
