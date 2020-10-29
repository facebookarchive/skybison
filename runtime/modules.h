#pragma once

#include "frame.h"
#include "globals.h"
#include "handles-decl.h"
#include "objects.h"
#include "symbols.h"
#include "view.h"

#define FUNC(module, name) module##_##name##_func
#define METH(type, name) type##_##name##_meth

namespace py {

class Thread;

// Function pointer stored in RawCode::code() for builtin functions.
using BuiltinFunction = RawObject (*)(Thread* thread, Arguments args);

using ModuleInitFunc = void (*)(Thread* thread, const Module& module,
                                View<byte> bytecode);

struct BuiltinType {
  SymbolId name;
  LayoutId type;
};

struct FrozenModule {
  const char* name;
  word marshalled_code_length;
  const byte* marshalled_code;
  ModuleInitFunc init;
  bool is_package;
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

// Returns `true` if there is a frozen module with name `name`.
bool isFrozenModule(const Str& name);

// Returns `true` if there is a frozen package with name `name`.
bool isFrozenPackage(const Str& name);

// Return the FrozenModule with the given name.
// If there is no such frozen module, return nullptr.
const FrozenModule* frozenModuleByName(const Str& name);

}  // namespace py
