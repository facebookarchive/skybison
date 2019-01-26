#pragma once

#include "runtime.h"

namespace python {

RawObject complexGetImag(Thread* thread, Frame* frame, word nargs);
RawObject complexGetReal(Thread* thread, Frame* frame, word nargs);

class ComplexBuiltins final {
 public:
  static void initialize(Runtime* runtime);

  static RawObject dunderNew(Thread*, Frame*, word);

 private:
  static const BuiltinMethod kMethods[];

  DISALLOW_IMPLICIT_CONSTRUCTORS(ComplexBuiltins);
};

}  // namespace python
