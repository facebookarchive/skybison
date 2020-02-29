#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace py {

class FunctionBuiltins
    : public Builtins<FunctionBuiltins, ID(function), LayoutId::kFunction> {
 public:
  static void postInitialize(Runtime* runtime, const Type& new_type);

  static const BuiltinAttribute kAttributes[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(FunctionBuiltins);
};

class BoundMethodBuiltins
    : public Builtins<BoundMethodBuiltins, ID(method), LayoutId::kBoundMethod> {
 public:
  static const BuiltinAttribute kAttributes[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(BoundMethodBuiltins);
};

}  // namespace py
