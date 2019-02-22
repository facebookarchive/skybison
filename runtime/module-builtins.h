#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

// Runs the executable functions found in the PyModuleDef
int execDef(Thread* thread, const Module& module, PyModuleDef* def);

class ModuleBuiltins
    : public Builtins<ModuleBuiltins, SymbolId::kModule, LayoutId::kModule> {
 public:
  static RawObject dunderNew(Thread* thread, Frame* frame, word nargs);

  static const View<BuiltinAttribute> kAttributes;
  static const View<BuiltinMethod> kBuiltinMethods;

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(ModuleBuiltins);
};

}  // namespace python
