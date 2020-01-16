#pragma once

#include "globals.h"
#include "handles.h"
#include "objects.h"
#include "runtime.h"
#include "symbols.h"
#include "thread.h"

namespace py {

struct BuiltinType {
  SymbolId name;
  LayoutId type;
};

struct ModuleInitializer {
  SymbolId name;
  void (*create_module)(Thread*);
};

class ModuleBaseBase {
 public:
  static const BuiltinMethod kBuiltinMethods[];
  static const BuiltinType kBuiltinTypes[];
  static const char kFrozenData[];
};

template <typename T, SymbolId name>
class ModuleBase : public ModuleBaseBase {
 public:
  static void initialize(Thread* thread) {
    HandleScope scope(thread);
    Runtime* runtime = thread->runtime();
    Module module(&scope, runtime->createModule(thread, name));
    for (word i = 0; T::kBuiltinMethods[i].name != SymbolId::kSentinelId; i++) {
      runtime->moduleAddBuiltinFunction(thread, module,
                                        T::kBuiltinMethods[i].name,
                                        T::kBuiltinMethods[i].address);
    }
    for (word i = 0; T::kBuiltinTypes[i].name != SymbolId::kSentinelId; i++) {
      runtime->moduleAddBuiltinType(thread, module, T::kBuiltinTypes[i].name,
                                    T::kBuiltinTypes[i].type);
    }
    CHECK(
        !runtime->executeFrozenModule(thread, T::kFrozenData, module).isError(),
        "Failed to initialize %s module",
        runtime->symbols()->predefinedSymbolAt(name));
  }
};

extern const ModuleInitializer kBuiltinModules[];

}  // namespace py
