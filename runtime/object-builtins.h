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
  static const NativeMethod kNativeMethods[];
  static const BuiltinMethod kBuiltinMethods[];

  DISALLOW_IMPLICIT_CONSTRUCTORS(ObjectBuiltins);
};

class NoneBuiltins
    : public Builtins<NoneBuiltins, SymbolId::kNoneType, LayoutId::kNoneType> {
 public:
  static RawObject dunderNew(Thread*, Frame*, word);
  static RawObject dunderRepr(Thread*, Frame*, word);

  static const NativeMethod kNativeMethods[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(NoneBuiltins);
};

}  // namespace python
