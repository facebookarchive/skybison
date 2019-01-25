#pragma once

#include "runtime.h"

namespace python {

class SliceBuiltins {
 public:
  static void initialize(Runtime* runtime);

  static RawObject dunderNew(Thread* thread, Frame* frame, word nargs);

 private:
  static const BuiltinAttribute kAttributes[];
  static const BuiltinMethod kMethods[];

  DISALLOW_IMPLICIT_CONSTRUCTORS(SliceBuiltins);
};

}  // namespace python
