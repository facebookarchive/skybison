#pragma once

#include "globals.h"
#include "runtime.h"

namespace python {

RawObject objectGetAttribute(Thread* thread, const Object& object,
                             const Object& name_str);

class ObjectBuiltins {
 public:
  static void initialize(Runtime* runtime);

  static RawObject dunderGetattribute(Thread*, Frame*, word);
  static RawObject dunderHash(Thread*, Frame*, word);
  static RawObject dunderInit(Thread*, Frame*, word);
  static RawObject dunderNew(Thread*, Frame*, word);
  static RawObject dunderNewKw(Thread*, Frame*, word);
  static RawObject dunderSizeof(Thread*, Frame*, word);

 private:
  static const BuiltinMethod kBuiltinMethods[];

  DISALLOW_IMPLICIT_CONSTRUCTORS(ObjectBuiltins);
};

class NoneBuiltins
    : public Builtins<NoneBuiltins, SymbolId::kNoneType, LayoutId::kNoneType> {
 public:
  static RawObject dunderNew(Thread*, Frame*, word);
  static RawObject dunderRepr(Thread*, Frame*, word);

  static const BuiltinMethod kBuiltinMethods[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(NoneBuiltins);
};

}  // namespace python
