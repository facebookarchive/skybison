#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

RawObject typeNew(Thread* thread, LayoutId metaclass_id, const Str& name,
                  const Tuple& bases, const Dict& dict);
// Returns the "user-visible" type of an object. This hides the smallint,
// smallstr, largeint, largestr types and pretends the object is of type
// str/int instead.
RawObject userVisibleTypeOf(Thread* thread, const Object& obj);

class TypeBuiltins
    : public Builtins<TypeBuiltins, SymbolId::kType, LayoutId::kType> {
 public:
  static void postInitialize(Runtime* runtime, const Type& new_type);

  static RawObject dunderCall(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderNew(Thread* thread, Frame* frame, word nargs);

  static const BuiltinAttribute kAttributes[];
  static const BuiltinMethod kBuiltinMethods[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(TypeBuiltins);
};

}  // namespace python
