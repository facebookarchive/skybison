#include "modules.h"

#include "builtins-module.h"
#include "faulthandler-module.h"
#include "frozen-modules.h"
#include "globals.h"
#include "marshal-module.h"
#include "marshal.h"
#include "module-builtins.h"
#include "runtime.h"
#include "symbols.h"
#include "sys-module.h"
#include "type-builtins.h"
#include "under-builtins-module.h"
#include "under-codecs-module.h"
#include "under-imp-module.h"
#include "under-io-module.h"
#include "under-os-module.h"
#include "under-valgrind-module.h"
#include "under-warnings-module.h"
#include "under-weakref-module.h"

namespace py {

const BuiltinMethod ModuleBaseBase::kBuiltinMethods[] = {
    {SymbolId::kSentinelId, nullptr},
};
const BuiltinType ModuleBaseBase::kBuiltinTypes[] = {
    {SymbolId::kSentinelId, LayoutId::kSentinelId},
};

template <const char* data>
static void initializeFrozenModule(Thread* thread, const Module& module) {
  executeFrozenModule(thread, data, module);
}

const ModuleInitializer kBuiltinModules[] = {
    {SymbolId::kUnderBuiltins, &UnderBuiltinsModule::initialize},
    {SymbolId::kBuiltins, &BuiltinsModule::initialize},
    {SymbolId::kSys, &SysModule::initialize},
    {SymbolId::kUnderCodecs, &UnderCodecsModule::initialize},
    {SymbolId::kUnderImp, &UnderImpModule::initialize},
    {SymbolId::kUnderOs, &UnderOsModule::initialize},
    {SymbolId::kUnderSignal, &initializeFrozenModule<kUnderSignalModuleData>},
    {SymbolId::kUnderWeakref, &UnderWeakrefModule::initialize},
    {SymbolId::kUnderThread, &initializeFrozenModule<kUnderThreadModuleData>},
    {SymbolId::kUnderIo, &UnderIoModule::initialize},
    {SymbolId::kUnderStrMod,
     &initializeFrozenModule<kUnderStrUnderModModuleData>},
    {SymbolId::kUnderValgrind, &UnderValgrindModule::initialize},
    {SymbolId::kFaulthandler, &FaulthandlerModule::initialize},
    {SymbolId::kMarshal, &MarshalModule::initialize},
    {SymbolId::kUnderWarnings, &UnderWarningsModule::initialize},
    {SymbolId::kOperator, &initializeFrozenModule<kOperatorModuleData>},
    {SymbolId::kWarnings, &initializeFrozenModule<kWarningsModuleData>},
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
    if (!typeAtById(thread, type, SymbolId::kDunderDoc).isErrorNotFound()) {
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

void executeFrozenModule(Thread* thread, const char* buffer,
                         const Module& module) {
  HandleScope scope(thread);
  // TODO(matthiasb): 12 is a minimum, we should be using the actual
  // length here!
  word length = 12;
  View<byte> data(reinterpret_cast<const byte*>(buffer), length);
  Marshal::Reader reader(&scope, thread->runtime(), data);
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

void moduleAddBuiltinFunctions(Thread* thread, const Module& module,
                               const BuiltinMethod* functions) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object parameter_names(&scope, runtime->emptyTuple());
  Object name(&scope, NoneType::object());
  Object function(&scope, NoneType::object());
  Object globals(&scope, NoneType::object());
  for (word i = 0; functions[i].name != SymbolId::kSentinelId; i++) {
    name = runtime->symbols()->at(functions[i].name);
    Code code(&scope,
              runtime->newBuiltinCode(/*argcount=*/0, /*posonlyargcount=*/0,
                                      /*kwonlyargcount=*/0,
                                      /*flags=*/0, functions[i].address,
                                      parameter_names, name));
    function = runtime->newFunctionWithCode(thread, name, code, globals);
    moduleAtPut(thread, module, name, function);
  }
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
