#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

RawObject typeGetAttribute(Thread* thread, const Type& type,
                           const Object& name_str);

// Returns true if the type defines a __set__ method.
bool typeIsDataDescriptor(Thread* thread, const Type& type);

// Returns true if the type defines a __get__ method.
bool typeIsNonDataDescriptor(Thread* thread, const Type& type);

// If descr's Type has __get__(), call it with the appropriate arguments and
// return the result. Otherwise, return descr.
RawObject resolveDescriptorGet(Thread* thread, const Object& descr,
                               const Object& instance,
                               const Object& instance_type);

// Looks up `name` in the dict of each entry in type's MRO. Returns an `Error`
// object if the name wasn't found.
RawObject typeLookupNameInMro(Thread* thread, const Type& type,
                              const Object& name_str);

// Looks up `symbol` in the dict of each entry in type's MRO. Returns an `Error`
// object if the name wasn't found.
RawObject typeLookupSymbolInMro(Thread* thread, const Type& type,
                                SymbolId symbol);

RawObject typeNew(Thread* thread, LayoutId metaclass_id, const Str& name,
                  const Tuple& bases, const Dict& dict);

RawObject typeSetAttr(Thread* thread, const Type& type,
                      const Object& name_interned_str, const Object& value);

// Terminate the process if cache invalidation for updating attr_name in type
// objects is unimplemented.
void terminateIfUnimplementedTypeAttrCacheInvalidation(Thread* thread,
                                                       const Str& attr_name);

class TypeBuiltins
    : public Builtins<TypeBuiltins, SymbolId::kType, LayoutId::kType> {
 public:
  static void postInitialize(Runtime* runtime, const Type& new_type);

  static RawObject dunderCall(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderGetattribute(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderNew(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderSetattr(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderSubclasses(Thread* thread, Frame* frame, word nargs);
  static RawObject mro(Thread* thread, Frame* frame, word nargs);

  static const BuiltinAttribute kAttributes[];
  static const BuiltinMethod kBuiltinMethods[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(TypeBuiltins);
};

}  // namespace python
