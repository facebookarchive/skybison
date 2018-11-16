#pragma once

#include "globals.h"
#include "runtime.h"

namespace python {

class ObjectBuiltins {
 public:
  static void initialize(Runtime* runtime);

  static RawObject dunderHash(Thread*, Frame*, word);
  static RawObject dunderInit(Thread*, Frame*, word);
  static RawObject dunderNew(Thread*, Frame*, word);
  static RawObject dunderNewKw(Thread*, Frame*, word);
  static RawObject dunderRepr(Thread*, Frame*, word);

 private:
  static const BuiltinMethod kMethods[];

  DISALLOW_IMPLICIT_CONSTRUCTORS(ObjectBuiltins);
};

class NoneBuiltins {
 public:
  static void initialize(Runtime* runtime);

  static RawObject dunderNew(Thread*, Frame*, word);

 private:
  static const BuiltinMethod kMethods[];

  DISALLOW_IMPLICIT_CONSTRUCTORS(NoneBuiltins);
};

}  // namespace python
