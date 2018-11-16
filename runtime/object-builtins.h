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
  static Object* dunderNewKw(Thread*, Frame*, word);
  static Object* dunderRepr(Thread*, Frame*, word);

 private:
  static const BuiltinMethod kMethods[];

  DISALLOW_IMPLICIT_CONSTRUCTORS(ObjectBuiltins);
};

class NoneBuiltins {
 public:
  static void initialize(Runtime* runtime);

  static Object* dunderNew(Thread*, Frame*, word);

 private:
  static const BuiltinMethod kMethods[];

  DISALLOW_IMPLICIT_CONSTRUCTORS(NoneBuiltins);
};

}  // namespace python
