#pragma once

#include "frame.h"
#include "globals.h"
#include "modules.h"
#include "objects.h"
#include "runtime.h"

namespace py {

class ArrayModule {
 public:
  static void initialize(Thread* thread, const Module& module);

 private:
  static const BuiltinType kBuiltinTypes[];
};

class ArrayBuiltins
    : public Builtins<ArrayBuiltins, SymbolId::kArray, LayoutId::kArray> {
 public:
  static const BuiltinAttribute kAttributes[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(ArrayBuiltins);
};

}  // namespace py
