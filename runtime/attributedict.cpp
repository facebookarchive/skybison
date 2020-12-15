#include "attributedict.h"

#include "runtime.h"
#include "thread.h"
#include "type-builtins.h"

namespace py {

const word kInitialCapacity = 16;

static NEVER_INLINE void attributeGrow(Thread* thread,
                                       const AttributeDict& attrs) {
  HandleScope scope(thread);
  Tuple old_data(&scope, attrs.attributes());

  // Count the number of filled buckets that are not tombstones.
  word old_capacity = old_data.length();
  word num_items = 0;
  for (word idx = 0; idx < old_capacity; idx += kAttributeBucketNumWords) {
    RawObject key = old_data.at(idx + kAttributeBucketKeyOffset);
    if (key != kAttributeDictEmptyKey && key != kAttributeDictTombstoneKey) {
      num_items++;
    }
  }

  // Grow if more than half of the buckets are filled, otherwise maintain size
  // and just clean out the tombstones.
  word old_num_buckets = old_capacity >> 1;
  word new_capacity = old_capacity;
  if (num_items > (old_num_buckets >> 1)) {
    // Grow if more than half of the buckets are filled, otherwise just clean
    // out the tombstones.
    new_capacity *= 2;
  }

  // Allocate new tuple and re-hash.
  MutableTuple new_data(&scope,
                        thread->runtime()->newMutableTuple(new_capacity));
  word num_buckets = new_capacity >> 1;
  DCHECK(Utils::isPowerOfTwo(num_buckets), "must be power of two");
  word new_remaining = (num_buckets * 2) / 3;
  word mask = num_buckets - 1;
  Object key(&scope, NoneType::object());
  for (word old_idx = 0; old_idx < old_capacity;
       old_idx += kAttributeBucketNumWords) {
    key = old_data.at(old_idx + kAttributeBucketKeyOffset);
    if (key == kAttributeDictEmptyKey || key == kAttributeDictTombstoneKey) {
      continue;
    }
    DCHECK(key.isStr(), "key must be None, _Unbound or str");
    word hash = internedStrHash(*key);
    word bucket = hash & mask;
    word num_probes = 0;
    while (new_data.at(bucket * kAttributeBucketNumWords +
                       kAttributeBucketKeyOffset) != kAttributeDictEmptyKey) {
      num_probes++;
      bucket = (bucket + num_probes) & mask;
    }
    new_data.atPut(
        bucket * kAttributeBucketNumWords + kAttributeBucketKeyOffset, *key);
    new_data.atPut(
        bucket * kAttributeBucketNumWords + kAttributeBucketValueOffset,
        old_data.at(old_idx + kAttributeBucketValueOffset));
    new_remaining--;
  }
  DCHECK(new_remaining > 0, "must have remaining buckets");
  attrs.setAttributes(*new_data);
  attrs.setAttributesRemaining(new_remaining);
}

void attributeDictInit(Thread* thread, const AttributeDict& attrs) {
  attrs.setAttributes(thread->runtime()->newMutableTuple(kInitialCapacity));
  word num_buckets = kInitialCapacity >> 1;
  attrs.setAttributesRemaining((num_buckets * 2) / 3);
}

RawObject attributeKeys(Thread* thread, const AttributeDict& attrs) {
  HandleScope scope(thread);
  MutableTuple data(&scope, attrs.attributes());
  Runtime* runtime = thread->runtime();
  List keys(&scope, runtime->newList());
  Object key(&scope, NoneType::object());
  Object cell(&scope, NoneType::object());
  for (word i = 0, length = data.length(); i < length;
       i += kAttributeBucketNumWords) {
    key = data.at(i + kAttributeBucketKeyOffset);
    if (key == kAttributeDictEmptyKey || key == kAttributeDictTombstoneKey) {
      continue;
    }
    DCHECK(key.isStr(), "key must be a str");
    cell = data.at(i + kAttributeBucketValueOffset);
    if (ValueCell::cast(*cell).isPlaceholder()) {
      continue;
    }
    runtime->listAdd(thread, keys, key);
  }
  return *keys;
}

word attributeLen(Thread* thread, const AttributeDict& attrs) {
  HandleScope scope(thread);
  MutableTuple data(&scope, attrs.attributes());
  Object key(&scope, NoneType::object());
  Object cell(&scope, NoneType::object());
  word count = 0;
  for (word i = 0, length = data.length(); i < length;
       i += kAttributeBucketNumWords) {
    key = data.at(i + kAttributeBucketKeyOffset);
    if (key == kAttributeDictEmptyKey || key == kAttributeDictTombstoneKey) {
      continue;
    }
    DCHECK(key.isStr(), "key must be a str");
    cell = data.at(i + kAttributeBucketValueOffset);
    if (ValueCell::cast(*cell).isPlaceholder()) {
      continue;
    }
    count++;
  }
  return count;
}

RawObject attributeName(Thread* thread, const Object& name_obj) {
  if (name_obj.isSmallStr()) {
    return *name_obj;
  }
  if (name_obj.isLargeStr()) {
    return Runtime::internLargeStr(thread, name_obj);
  }

  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfStr(*name_obj)) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "attribute name must be string, not '%T'",
                                &name_obj);
  }
  HandleScope scope(thread);
  Type type(&scope, runtime->typeOf(*name_obj));
  if (typeLookupInMroById(thread, *type, ID(__eq__)) !=
          runtime->strDunderEq() ||
      typeLookupInMroById(thread, *type, ID(__hash__)) !=
          runtime->strDunderHash()) {
    UNIMPLEMENTED(
        "str subclasses with __eq__ or __hash__ not supported as attribute "
        "name");
  }
  Str name_str(&scope, strUnderlying(*name_obj));
  return Runtime::internStr(thread, name_str);
}

RawObject attributeNameNoException(Thread* thread, const Object& name_obj) {
  if (name_obj.isSmallStr()) {
    return *name_obj;
  }
  if (name_obj.isLargeStr()) {
    return Runtime::internLargeStr(thread, name_obj);
  }

  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfStr(*name_obj)) {
    return Error::error();
  }
  HandleScope scope(thread);
  Type type(&scope, runtime->typeOf(*name_obj));
  if (typeLookupInMroById(thread, *type, ID(__eq__)) !=
          runtime->strDunderEq() ||
      typeLookupInMroById(thread, *type, ID(__hash__)) !=
          runtime->strDunderHash()) {
    UNIMPLEMENTED(
        "str subclasses with __eq__ or __hash__ not supported as attribute "
        "name");
  }
  Str name_str(&scope, strUnderlying(*name_obj));
  return Runtime::internStr(thread, name_str);
}

bool attributeFindForRemoval(const AttributeDict& attrs, const Object& name,
                             Object* value_cell_out, word* index_out) {
  RawMutableTuple data = MutableTuple::cast(attrs.attributes());
  word hash = internedStrHash(*name);
  word mask = (data.length() - 1) >> 1;
  for (word bucket = hash & mask, num_probes = 0;;
       bucket = (bucket + ++num_probes) & mask) {
    word index = bucket * kAttributeBucketNumWords;
    RawObject key = data.at(index + kAttributeBucketKeyOffset);
    if (key == *name) {
      *value_cell_out = data.at(index + kAttributeBucketValueOffset);
      *index_out = index;
      return true;
    }
    if (key == kAttributeDictEmptyKey) {
      return false;
    }
    // Remaining cases are either a key that does not match or tombstone.
  }
}

void attributeRemove(const AttributeDict& attrs, word index) {
  RawMutableTuple data = MutableTuple::cast(attrs.attributes());
  data.atPut(index + kAttributeBucketKeyOffset, kAttributeDictTombstoneKey);
  data.atPut(index + kAttributeBucketValueOffset, NoneType::object());
}

RawObject inline USED attributeValueCellAtPut(Thread* thread,
                                              const AttributeDict& attrs,
                                              const Object& name) {
  HandleScope scope(thread);
  MutableTuple data_obj(&scope, attrs.attributes());
  RawMutableTuple data = *data_obj;
  word hash = internedStrHash(*name);
  word mask = (data.length() - 1) >> 1;
  for (word bucket = hash & mask, num_probes = 0, last_tombstone = -1;;
       bucket = (bucket + ++num_probes) & mask) {
    word idx = bucket * kAttributeBucketNumWords;
    RawObject key = data.at(idx + kAttributeBucketKeyOffset);
    if (key == *name) {
      return RawValueCell::cast(data.at(idx + kAttributeBucketValueOffset));
    }
    if (key == kAttributeDictEmptyKey) {
      DCHECK(Runtime::isInternedStr(thread, name), "expected interned str");
      RawValueCell cell = ValueCell::cast(thread->runtime()->newValueCell());
      cell.makePlaceholder();
      // newValueCell() may have triggered a GC; restore raw references.
      data = *data_obj;
      if (last_tombstone >= 0) {
        // Overwrite an existing tombstone entry.
        word last_tombstone_idx = last_tombstone * kAttributeBucketNumWords;
        data.atPut(last_tombstone_idx + kAttributeBucketKeyOffset, *name);
        data.atPut(last_tombstone_idx + kAttributeBucketValueOffset, cell);
      } else {
        // Use new bucket.
        data.atPut(idx + kAttributeBucketKeyOffset, *name);
        data.atPut(idx + kAttributeBucketValueOffset, cell);
        word remaining = attrs.attributesRemaining() - 1;
        attrs.setAttributesRemaining(remaining);
        if (remaining == 0) {
          ValueCell cell_obj(&scope, cell);
          attributeGrow(thread, attrs);
          return *cell_obj;
        }
      }
      return cell;
    }
    if (key == kAttributeDictTombstoneKey) {
      last_tombstone = bucket;
    }
  }
}

RawObject attributeValueCellAt(RawAttributeDict attrs, RawObject name) {
  word hash = internedStrHash(name);
  return attributeValueCellAtWithHash(attrs, name, hash);
}

RawObject attributeValues(Thread* thread, const AttributeDict& attrs) {
  HandleScope scope(thread);
  MutableTuple data(&scope, attrs.attributes());
  Runtime* runtime = thread->runtime();
  List values(&scope, runtime->newList());
  Object key(&scope, NoneType::object());
  Object value(&scope, NoneType::object());
  for (word i = 0, length = data.length(); i < length;
       i += kAttributeBucketNumWords) {
    key = data.at(i + kAttributeBucketKeyOffset);
    if (key == kAttributeDictEmptyKey || key == kAttributeDictTombstoneKey) {
      continue;
    }
    DCHECK(key.isStr(), "key must be a str");
    value = data.at(i + kAttributeBucketValueOffset);
    if (ValueCell::cast(*value).isPlaceholder()) {
      continue;
    }
    value = ValueCell::cast(*value).value();
    runtime->listAdd(thread, values, value);
  }
  return *values;
}

}  // namespace py
