#pragma once

#include "objects.h"
#include "runtime.h"

namespace python {

class BytesBuiltins {
 public:
  static void initialize(Runtime* runtime);

  static RawObject dunderAdd(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderLen(Thread* thread, Frame* frame, word nargs);

 private:
  static const BuiltinMethod kMethods[];

  DISALLOW_IMPLICIT_CONSTRUCTORS(BytesBuiltins);
};

}  // namespace python
