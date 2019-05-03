#pragma once

#include "globals.h"
#include "runtime.h"

namespace python {

RawObject objectGetAttribute(Thread* thread, const Object& object,
                             const Object& name_str);

RawObject objectRaiseAttributeError(Thread* thread, const Object& object,
                                    const Object& name_str);

RawObject objectSetAttr(Thread* thread, const Object& object,
                        const Object& name_interned_str, const Object& value);

class ObjectBuiltins {
 public:
  static void initialize(Runtime* runtime);
  static void postInitialize(Runtime* runtime, const Type& new_type);

  static RawObject dunderGetattribute(Thread*, Frame*, word);
  static RawObject dunderHash(Thread*, Frame*, word);
  static RawObject dunderInit(Thread*, Frame*, word);
  static RawObject dunderNew(Thread*, Frame*, word);
  static RawObject dunderNewKw(Thread*, Frame*, word);
  static RawObject dunderSetattr(Thread*, Frame*, word);
  static RawObject dunderSizeof(Thread*, Frame*, word);

 private:
  static const BuiltinMethod kBuiltinMethods[];

  DISALLOW_IMPLICIT_CONSTRUCTORS(ObjectBuiltins);
};

class NoneBuiltins
    : public Builtins<NoneBuiltins, SymbolId::kNoneType, LayoutId::kNoneType> {
 public:
  static void postInitialize(Runtime*, const Type& new_type);

  static RawObject dunderNew(Thread*, Frame*, word);
  static RawObject dunderRepr(Thread*, Frame*, word);

  static const BuiltinMethod kBuiltinMethods[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(NoneBuiltins);
};

}  // namespace python
