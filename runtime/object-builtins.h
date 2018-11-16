#pragma once

#include "globals.h"
#include "runtime.h"

namespace python {

class ObjectBuiltins {
 public:
  static void initialize(Runtime* runtime);

  static Object* dunderHash(Thread*, Frame*, word);
  static Object* dunderInit(Thread*, Frame*, word);
  static Object* dunderNew(Thread*, Frame*, word);
  static Object* dunderRepr(Thread*, Frame*, word);
  static Object* dunderStr(Thread*, Frame*, word);

 private:
  static const BuiltinMethod kMethods[];

  DISALLOW_IMPLICIT_CONSTRUCTORS(ObjectBuiltins);
};

}  // namespace python
