#pragma once

#include "globals.h"
#include "runtime.h"

namespace py {

RawObject instanceDelAttr(Thread* thread, const Instance& instance,
                          const Str& name_interned);

RawObject instanceGetAttribute(Thread* thread, const Instance& instance,
                               const Str& name_interned);

RawObject instanceSetAttr(Thread* thread, const Instance& instance,
                          const Str& name_interned, const Object& value);

RawObject objectGetAttribute(Thread* thread, const Object& object,
                             const Object& name_str, word hash);

// Same as `objectGetAttribute()` but stores a value into `location_out` that is
// used by the inline cache and interpreted by
// `Interpreter::loadAttrWithLocation()`.
RawObject objectGetAttributeSetLocation(Thread* thread, const Object& object,
                                        const Object& name_str, word hash,
                                        Object* location_out);

// Raise an AttributeError that `object` has no attribute named `name_str`.
RawObject objectRaiseAttributeError(Thread* thread, const Object& object,
                                    const Object& name_str);

RawObject objectSetAttr(Thread* thread, const Object& object,
                        const Object& name_str, word hash, const Object& value);

RawObject objectSetAttrSetLocation(Thread* thread, const Object& object,
                                   const Object& name_str, word hash,
                                   const Object& value, Object* location_out);

RawObject objectNew(Thread* thread, const Type& type);

RawObject objectDelItem(Thread* thread, const Object& object,
                        const Object& key);

RawObject objectGetItem(Thread* thread, const Object& object,
                        const Object& key);

RawObject objectSetItem(Thread* thread, const Object& object, const Object& key,
                        const Object& value);

class ObjectBuiltins {
 public:
  static void initialize(Runtime* runtime);
  static void postInitialize(Runtime* runtime, const Type& new_type);

  static RawObject dunderGetattribute(Thread*, Frame*, word);
  static RawObject dunderHash(Thread*, Frame*, word);
  static RawObject dunderInit(Thread*, Frame*, word);
  static RawObject dunderNew(Thread*, Frame*, word);
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

}  // namespace py
