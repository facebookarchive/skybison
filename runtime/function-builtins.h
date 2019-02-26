#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

class FunctionBuiltins : public Builtins<FunctionBuiltins, SymbolId::kFunction,
                                         LayoutId::kFunction> {
 public:
  static void postInitialize(Runtime* runtime, const Type& new_type);

  static RawObject dunderGet(Thread* thread, Frame* frame, word nargs);

  static const BuiltinMethod kBuiltinMethods[];
  static const BuiltinAttribute kAttributes[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(FunctionBuiltins);
};

}  // namespace python
