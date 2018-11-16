#pragma once

#include "runtime.h"

namespace python {

class ComplexBuiltins final {
 public:
  static void initialize(Runtime* runtime);

  static Object* dunderNew(Thread*, Frame*, word);

 private:
  static const BuiltinMethod kMethods[];

  DISALLOW_IMPLICIT_CONSTRUCTORS(ComplexBuiltins);
};

}  // namespace python
