#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

class FunctionBuiltins {
 public:
  static void initialize(Runtime* runtime);

  static RawObject dunderGet(Thread* thread, Frame* frame, word nargs);

 private:
  static const BuiltinMethod kMethods[];
  static const BuiltinAttribute kAttributes[];

  DISALLOW_IMPLICIT_CONSTRUCTORS(FunctionBuiltins);
};

}  // namespace python
