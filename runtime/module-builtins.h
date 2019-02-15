#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

// Runs the executable functions found in the PyModuleDef
int execDef(Thread* thread, const Module& module, PyModuleDef* def);

class ModuleBuiltins {
 public:
  static void initialize(Runtime* runtime);

  static RawObject dunderNew(Thread* thread, Frame* frame, word nargs);

 private:
  static const BuiltinAttribute kAttributes[];
  static const BuiltinMethod kBuiltinMethods[];

  DISALLOW_IMPLICIT_CONSTRUCTORS(ModuleBuiltins);
};

}  // namespace python
