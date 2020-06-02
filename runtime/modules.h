#pragma once

#include "globals.h"
#include "handles.h"
#include "objects.h"
#include "runtime.h"
#include "symbols.h"
#include "thread.h"

namespace py {

#define FUNC(module, name) module##_##name##_func

struct BuiltinType {
  SymbolId name;
  LayoutId type;
};

typedef void (*ModuleInitFunc)(Thread* thread, const Module& module,
                               View<byte> bytecode);

struct FrozenModule {
  const char* name;
  word marshalled_code_length;
  const byte* marshalled_code;
  ModuleInitFunc init;
};

RawObject ensureBuiltinModule(Thread* thread, const Str& name);

RawObject ensureBuiltinModuleById(Thread* thread, SymbolId id);

// Execute a frozen module by marshalling it into a code object and then
// executing it. Aborts if module execution is unsuccessful.
void executeFrozenModule(Thread* thread, const Module& module,
                         View<byte> bytecode);

// Execute the code object that represents the code for the top-level module
// (eg the result of compiling some_file.py). Return the result.
NODISCARD RawObject executeModule(Thread* thread, const Code& code,
                                  const Module& module);

// Exposed for use by the tests. We may be able to remove this later.
NODISCARD RawObject executeModuleFromCode(Thread* thread, const Code& code,
                                          const Object& name);

// Initialize built-in extension module `name` if it exists, otherwise
// return `nullptr`.
RawObject moduleInitBuiltinExtension(Thread* thread, const Str& name);

// Returns `true` if there is a built-in extension module with name `name`.
bool isBuiltinExtensionModule(const Str& name);

// Returns `true` if there is a frozen module with name `name`.
bool isFrozenModule(const Str& name);

void moduleAddBuiltinTypes(Thread* thread, const Module& module,
                           View<BuiltinType> types);

int moduleAddToState(Thread* thread, Module* module);

}  // namespace py
