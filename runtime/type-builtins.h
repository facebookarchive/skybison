#pragma once

#include "frame.h"
#include "globals.h"
#include "layout.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace py {

constexpr View<BuiltinAttribute> kNoAttributes(nullptr, 0);

// Prepare `name_obj` to be used as an attribute name: Raise a TypeError if it
// is not a string; reject some string subclasses. Otherwise return an
// interned string that can be used with attribute accessors.
RawObject attributeName(Thread* thread, const Object& name_obj);

RawObject attributeNameNoException(Thread* thread, const Object& name_obj);

RawObject addBuiltinType(Thread* thread, SymbolId name, LayoutId layout_id,
                         LayoutId superclass_id, View<BuiltinAttribute> attrs,
                         word size, bool basetype);

RawObject addImmediateBuiltinType(Thread* thread, SymbolId name,
                                  LayoutId layout_id, LayoutId builtin_base,
                                  LayoutId superclass_id, bool basetype);

void builtinTypeEnableTupleOverflow(Thread* thread, const Type& type);

// Returns the most generic base among `bases` that captures inherited
// attributes with a fixed offset (either from __slots__ or builtin types)
// Note that this simulates `best_base` from CPython's typeobject.c.
RawObject computeFixedAttributeBase(Thread* thread, const Tuple& bases);

RawObject findBuiltinTypeWithName(Thread* thread, const Object& name);

RawObject raiseTypeErrorCannotSetImmutable(Thread* thread, const Type& type);

bool typeIsSubclass(RawObject subclass, RawObject superclass);

void typeAddDocstring(Thread* thread, const Type& type);
void typeAddInstanceDict(Thread* thread, const Type& type);

// Assign all key/values from the dict to the type. This interns the keys as
// necessary or may raise an exception for invalid keys (see attributeName()).
RawObject typeAssignFromDict(Thread* thread, const Type& type,
                             const Dict& dict);

RawObject typeAt(const Type& type, const Object& name);

RawObject typeAtSetLocation(RawType type, RawObject name, word hash,
                            Object* location);

RawObject typeAtById(Thread* thread, const Type& type, SymbolId id);

RawObject typeAtPut(Thread* thread, const Type& type, const Object& name,
                    const Object& value);

RawObject typeAtPutById(Thread* thread, const Type& type, SymbolId id,
                        const Object& value);

RawObject typeDeleteAttribute(Thread* thread, const Type& receiver,
                              const Object& name);

RawObject typeRemove(Thread* thread, const Type& type, const Object& name);

RawObject typeRemoveById(Thread* thread, const Type& type, SymbolId id);

RawObject typeKeys(Thread* thread, const Type& type);

word typeLen(Thread* thread, const Type& type);

RawObject typeValues(Thread* thread, const Type& type);

RawObject typeGetAttribute(Thread* thread, const Type& type,
                           const Object& name);

RawObject typeGetAttributeSetLocation(Thread* thread, const Type& type,
                                      const Object& name, Object* location_out);

// Returns true if the type defines a __set__ method.
bool typeIsDataDescriptor(RawType type);

// Returns true if the type defines a __get__ method.
bool typeIsNonDataDescriptor(RawType type);

// If descr's Type has __get__(), call it with the appropriate arguments and
// return the result. Otherwise, return descr.
RawObject resolveDescriptorGet(Thread* thread, const Object& descr,
                               const Object& instance,
                               const Object& instance_type);

RawObject typeInit(Thread* thread, const Type& type, const Str& name,
                   const Dict& dict, const Tuple& mro);

// Looks up `key` in the dict of each entry in type's MRO. Returns
// `Error::notFound()` if the name was not found.
RawObject typeLookupInMro(Thread* thread, RawType type, RawObject name);

RawObject typeLookupInMroSetLocation(Thread* thread, RawType type,
                                     RawObject name, Object* location);

// Looks up `id` in the dict of each entry in type's MRO. Returns
// `Error::notFound()` if the name was not found.
RawObject typeLookupInMroById(Thread* thread, RawType type, SymbolId id);

RawObject typeNew(Thread* thread, const Type& metaclass, const Str& name,
                  const Tuple& bases, const Dict& dict, word flags,
                  bool inherit_slots, bool add_instance_dict);

RawObject typeSetAttr(Thread* thread, const Type& type, const Object& name,
                      const Object& value);

// Sets __class__ of self to new_type.
// Returns None on success.
// Raises an exception and returns Error::exception() otherwise.
RawObject typeSetDunderClass(Thread* thread, const Object& self,
                             const Type& new_type);

// Terminate the process if cache invalidation for updating attr_name in type
// objects is unimplemented.
void terminateIfUnimplementedTypeAttrCacheInvalidation(Thread* thread,
                                                       const Type& type,
                                                       const Object& attr_name);

void initializeTypeTypes(Thread* thread);

inline bool typeIsSubclass(RawObject subclass, RawObject superclass) {
  if (DCHECK_IS_ON()) {
    Runtime* runtime = Thread::current()->runtime();
    DCHECK(runtime->isInstanceOfType(subclass),
           "typeIsSubclass subclass must be a type object");
    DCHECK(runtime->isInstanceOfType(superclass),
           "typeIsSubclass superclass must be a type object");
    DCHECK(Tuple::cast(subclass.rawCast<RawType>().mro()).at(0) == subclass,
           "unexpected mro");
  }
  if (subclass == superclass) return true;
  RawTuple mro = Tuple::cast(subclass.rawCast<RawType>().mro());
  word length = mro.length();
  for (word i = 1; i < length; i++) {
    if (mro.at(i) == superclass) {
      return true;
    }
  }
  return false;
}

}  // namespace py
