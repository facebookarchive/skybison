#pragma once

#include "frame.h"
#include "globals.h"
#include "layout.h"
#include "objects.h"
#include "thread.h"

namespace py {

// Prepare `name_obj` to be used as an attribute name: Raise a TypeError if it
// is not a string; reject some string subclasses. Otherwise return an
// interned string that can be used with attribute accessors.
RawObject attributeName(Thread* thread, const Object& name_obj);

RawObject attributeNameNoException(Thread* thread, const Object& name_obj);

RawObject addBuiltinType(Thread* thread, SymbolId name, LayoutId layout_id,
                         LayoutId superclass_id, View<BuiltinAttribute> attrs);

RawObject addEmptyBuiltinType(Thread* thread, SymbolId name, LayoutId layout_id,
                              LayoutId superclass_id);

RawObject addImmediateBuiltinType(Thread* thread, SymbolId name,
                                  LayoutId layout_id, LayoutId builtin_base,
                                  LayoutId superclass_id);

bool typeIsSubclass(const Type& subclass, const Type& superclass);

void typeAddDocstring(Thread* thread, const Type& type);

// Assign all key/values from the dict to the type. This interns the keys as
// necessary or may raise an exception for invalid keys (see attributeName()).
RawObject typeAssignFromDict(Thread* thread, const Type& type,
                             const Dict& dict);

RawObject typeAt(const Type& type, const Object& name);

RawObject typeAtSetLocation(const Type& type, const Object& name, word hash,
                            Object* location);

RawObject typeAtById(Thread* thread, const Type& type, SymbolId id);

RawObject typeAtPut(Thread* thread, const Type& type, const Object& name,
                    const Object& value);

RawObject typeAtPutById(Thread* thread, const Type& type, SymbolId id,
                        const Object& value);

RawObject typeRemove(Thread* thread, const Type& type, const Object& name);

RawObject typeKeys(Thread* thread, const Type& type);

RawObject typeLen(Thread* thread, const Type& type);

RawObject typeValues(Thread* thread, const Type& type);

RawObject typeGetAttribute(Thread* thread, const Type& type,
                           const Object& name);

RawObject typeGetAttributeSetLocation(Thread* thread, const Type& type,
                                      const Object& name, Object* location_out);

// Defined in ext/Objects/typeslots.cpp
// Return the type's tp_basicsize. Use only with extension types.
uword typeGetBasicSize(const Type& type);

// Returns true if the type defines a __set__ method.
bool typeIsDataDescriptor(Thread* thread, const Type& type);

// Returns true if the type defines a __get__ method.
bool typeIsNonDataDescriptor(Thread* thread, const Type& type);

// If descr's Type has __get__(), call it with the appropriate arguments and
// return the result. Otherwise, return descr.
RawObject resolveDescriptorGet(Thread* thread, const Object& descr,
                               const Object& instance,
                               const Object& instance_type);

RawObject typeInit(Thread* thread, const Type& type, const Str& name,
                   const Dict& dict, const Tuple& mro, bool inherit_slots);

void typeInitAttributes(Thread* thread, const Type& type);

// Inherit slots defined by a C Extension
RawObject typeInheritSlots(Thread* thread, const Type& type);

// Type has a list of type slots attached to it. The type slots are used by the
// C-API emulation layer for C extension types.
bool typeHasSlots(const Type& type);

// Looks up `key` in the dict of each entry in type's MRO. Returns
// `Error::notFound()` if the name was not found.
RawObject typeLookupInMro(Thread* thread, const Type& type, const Object& name);

RawObject typeLookupInMroSetLocation(Thread* thread, const Type& type,
                                     const Object& name, Object* location);

// Looks up `id` in the dict of each entry in type's MRO. Returns
// `Error::notFound()` if the name was not found.
RawObject typeLookupInMroById(Thread* thread, const Type& type, SymbolId id);

// Creates a new type. This function does not perform typeslot inheritance.
RawObject typeNew(Thread* thread, LayoutId metaclass_id, const Str& name,
                  const Tuple& bases, const Dict& dict, Type::Flag flags);

RawObject typeSetAttr(Thread* thread, const Type& type, const Object& name,
                      const Object& value);

// Terminate the process if cache invalidation for updating attr_name in type
// objects is unimplemented.
void terminateIfUnimplementedTypeAttrCacheInvalidation(Thread* thread,
                                                       const Type& type,
                                                       const Object& attr_name);

// Look-up underlying value-cell to a name.
// WARNING: This is a low-level access circumventing cache invalidation logic,
// do not use except for tests and ic.cpp!
RawObject typeValueCellAt(const Type& type, const Object& name);

// Look-up underlying value-cell to a name.
// WARNING: This is a low-level access circumventing cache invalidation logic,
// do not use except for tests and ic.cpp!
RawObject typeValueCellAtWithHash(const Type& type, const Object& name,
                                  word hash);

// Look-up or insert a value-cell for a given name.
// WARNING: This is a low-level access circumventing cache invalidation logic,
// do not use except for tests and ic.cpp!
RawObject typeValueCellAtPut(Thread* thread, const Type& type,
                             const Object& name);

void initializeTypeTypes(Thread* thread);

inline bool typeIsSubclass(const Type& subclass, const Type& superclass) {
  DCHECK(Tuple::cast(subclass.mro()).at(0) == subclass, "unexpected mro");
  if (subclass == superclass) return true;
  RawTuple mro = Tuple::cast(subclass.mro());
  word length = mro.length();
  for (word i = 1; i < length; i++) {
    if (mro.at(i) == superclass) {
      return true;
    }
  }
  return false;
}

}  // namespace py
