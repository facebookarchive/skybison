#pragma once

#include "handles.h"
#include "objects.h"

namespace py {

class Thread;

const int kAttributeBucketNumWords = 2;
const int kAttributeBucketKeyOffset = 0;
const int kAttributeBucketValueOffset = 1;
const RawObject kAttributeDictEmptyKey = NoneType::object();
const RawObject kAttributeDictTombstoneKey = Unbound::object();

RawObject attributeAt(RawAttributeDict attrs, RawObject name);

RawObject attributeAtWithHash(RawAttributeDict attrs, RawObject name,
                              word hash);

RawObject attributeAtPut(Thread* thread, const AttributeDict& attrs,
                         const Object& name, const Object& value);

void attributeDictInit(Thread* thread, const AttributeDict& attrs);

RawObject attributeKeys(Thread* thread, const AttributeDict& attrs);

word attributeLen(Thread* thread, const AttributeDict& attrs);

// Prepare `name` to be used as an attribute name: Raise a TypeError if it
// is not a string; reject some string subclasses. Otherwise return an
// interned string that can be used with attribute accessors.
RawObject attributeName(Thread* thread, const Object& name);

RawObject attributeNameNoException(Thread* thread, const Object& name);

bool attributeFindForRemoval(const AttributeDict& attrs, const Object& name,
                             Object* value_cell_out, word* index_out);

void attributeRemove(const AttributeDict& attrs, word index);

// Look-up underlying value-cell to a name.
RawObject attributeValueCellAt(RawAttributeDict attrs, RawObject name);

// Look-up underlying value-cell to a name.
RawObject attributeValueCellAtWithHash(RawAttributeDict attrs, RawObject name,
                                       word hash);

// Look-up or insert a value-cell for a given name.
RawObject attributeValueCellAtPut(Thread* thread, const AttributeDict& attrs,
                                  const Object& name);

RawObject attributeValues(Thread* thread, const AttributeDict& attrs);

word internedStrHash(RawObject name);

inline RawObject attributeValueCellAtWithHash(RawAttributeDict attrs,
                                              RawObject name, word hash) {
  RawMutableTuple data = MutableTuple::cast(attrs.attributes());
  word mask = (data.length() - 1) >> 1;
  for (word bucket = hash & mask, num_probes = 0;;
       bucket = (bucket + ++num_probes) & mask) {
    word idx = bucket * kAttributeBucketNumWords;
    RawObject key = data.at(idx + kAttributeBucketKeyOffset);
    if (key == name) {
      return data.at(idx + kAttributeBucketValueOffset);
    }
    if (key == kAttributeDictEmptyKey) {
      return Error::notFound();
    }
    // Remaining cases are either a key that does not match or tombstone.
  }
}

inline RawObject attributeAtPut(Thread* thread, const AttributeDict& attrs,
                                const Object& name, const Object& value) {
  RawValueCell value_cell =
      ValueCell::cast(attributeValueCellAtPut(thread, attrs, name));
  value_cell.setValue(*value);
  return value_cell;
}

inline RawObject attributeAtWithHash(RawAttributeDict attrs, RawObject name,
                                     word hash) {
  RawObject result = attributeValueCellAtWithHash(attrs, name, hash);
  if (result.isErrorNotFound()) return result;
  if (ValueCell::cast(result).isPlaceholder()) return Error::notFound();
  return ValueCell::cast(result).value();
}

inline RawObject attributeAt(RawAttributeDict attrs, RawObject name) {
  word hash = internedStrHash(name);
  return attributeAtWithHash(attrs, name, hash);
}

inline word internedStrHash(RawObject name) {
  if (name.isImmediateObjectNotSmallInt()) {
    return SmallStr::cast(name).hash();
  }
  word hash = LargeStr::cast(name).header().hashCode();
  DCHECK(hash != Header::kUninitializedHash,
         "hash has not been computed (string not interned?)");
  return hash;
}

}  // namespace py
