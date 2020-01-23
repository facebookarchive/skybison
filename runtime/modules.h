#pragma once

#include "globals.h"
#include "handles.h"
#include "objects.h"
#include "runtime.h"
#include "symbols.h"
#include "thread.h"

namespace py {

struct BuiltinFunction {
  SymbolId name;
  NativeMethodType address;
};

struct BuiltinType {
  SymbolId name;
  LayoutId type;
};

struct ModuleInitializer {
  SymbolId name;
  void (*init)(Thread*, const Module&);
};

// Execute a frozen module by marshalling it into a code object and then
// executing it. Aborts if module execution is unsuccessful.
void executeFrozenModule(Thread* thread, const char* buffer,
                         const Module& module);

// Execute the code object that represents the code for the top-level module
// (eg the result of compiling some_file.py). Return the result.
NODISCARD RawObject executeModule(Thread* thread, const Code& code,
                                  const Module& module);

// Exposed for use by the tests. We may be able to remove this later.
NODISCARD RawObject executeModuleFromCode(Thread* thread, const Code& code,
                                          const Object& name);

void moduleAddBuiltinFunctions(Thread* thread, const Module& module,
                               const BuiltinFunction* functions);
void moduleAddBuiltinTypes(Thread* thread, const Module& module,
                           const BuiltinType* types);

extern const ModuleInitializer kBuiltinModules[];

}  // namespace py
