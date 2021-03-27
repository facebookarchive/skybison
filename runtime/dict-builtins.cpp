#include "dict-builtins.h"

#include "builtins.h"
#include "frame.h"
#include "globals.h"
#include "int-builtins.h"
#include "interpreter.h"
#include "objects.h"
#include "runtime.h"
#include "str-builtins.h"
#include "thread.h"
#include "type-builtins.h"
#include "utils.h"

namespace py {

namespace {

// Helper functions for accessing sparse array from dict.indices().
static const uint32_t kTombstoneValue = 0xFFFFFFFE;
static const uint32_t kEmptyValue = 0xFFFFFFFF;
static const word kNumBytesExponent = 2;

static word probeBegin(word num_indices, word hash, word* indices_mask,
                       uword* perturb) {
  DCHECK(Utils::isPowerOfTwo(num_indices), "%ld is not a power of 2",
         num_indices);
  DCHECK(num_indices > 0, "indicesxs size <= 0");
  DCHECK(RawSmallInt::isValid(hash), "hash out of range");
  *perturb = static_cast<uword>(hash);
  *indices_mask = num_indices - 1;
  return *indices_mask & hash;
}

static word probeNext(word current, word indices_mask, uword* perturb) {
  // Given that current stands for the index into dict.indices, this advances
  // current to (5 * current + 1 + perturb). Note that it's guaranteed that
  // keeping calling this function returns a permutation of all indices when
  // the number of the indices is power of two. See
  // https://en.wikipedia.org/wiki/Linear_congruential_generator#c_%E2%89%A0_0.
  *perturb >>= 5;
  return (current * 5 + 1 + *perturb) & indices_mask;
}

// Returns the bytes offset into the indices array given an index
static word indexOffset(word index) { return index << kNumBytesExponent; }

// Return the item index stored at indices[bytes_offset].
static uint32_t itemIndexAt(RawMutableBytes indices, word bytes_offset) {
  DCHECK(bytes_offset % 4 == 0, "bytes_offset must be a multiple of 4");
  return indices.uint32At(bytes_offset);
}

// Set `item_index` at indices[bytes_offset].
static void itemIndexAtPut(RawMutableBytes indices, word bytes_offset,
                           uint32_t item_index) {
  indices.uint32AtPut(bytes_offset, item_index);
}

// Set a tombstone at indices[bytes_offset] given `data_length` from
// data.length().
static void indicesSetTombstone(RawMutableBytes indices, word bytes_offset) {
  itemIndexAtPut(indices, bytes_offset, kTombstoneValue);
}

// Return true if the indices[bytes_offset] is filled with an active item.
static bool indicesIsFull(RawMutableBytes indices, word bytes_offset,
                          uint32_t* item_index) {
  *item_index = itemIndexAt(indices, bytes_offset);
  return *item_index < kTombstoneValue;
}

// Return true if the indices[bytes_offset] is never used.
static bool indicesIsEmpty(RawMutableBytes indices, word bytes_offset) {
  return itemIndexAt(indices, bytes_offset) == kEmptyValue;
}

// Helper functions for accessing dict items from dict.data().

// Data tuple Layout.
static const word kItemHashOffset = 0;
static const word kItemKeyOffset = 1;
static const word kItemValueOffset = 2;
static const word kItemNumPointers = 3;

static RawObject itemKey(RawMutableTuple data, word index) {
  return data.at(index + kItemKeyOffset);
}

static RawObject itemValue(RawMutableTuple data, word index) {
  return data.at(index + kItemValueOffset);
}

static word itemHash(RawMutableTuple data, word index) {
  return SmallInt::cast(data.at(index + kItemHashOffset)).value();
}

static RawObject itemHashRaw(RawMutableTuple data, word index) {
  return data.at(index + kItemHashOffset);
}

static void itemSet(RawMutableTuple data, word index, word hash, RawObject key,
                    RawObject value) {
  data.atPut(index + kItemHashOffset, SmallInt::fromWordTruncated(hash));
  data.atPut(index + kItemKeyOffset, key);
  data.atPut(index + kItemValueOffset, value);
}

static void itemSetTombstone(RawMutableTuple data, word index) {
  data.atPut(index + kItemHashOffset, Unbound::object());
  data.atPut(index + kItemKeyOffset, NoneType::object());
  data.atPut(index + kItemValueOffset, NoneType::object());
}

static void itemSetValue(RawMutableTuple data, word index, RawObject value) {
  data.atPut(index + kItemValueOffset, value);
}

static bool itemIsEmpty(RawMutableTuple data, word index) {
  return data.at(index + kItemHashOffset).isNoneType();
}

static bool itemIsFull(RawMutableTuple data, word index) {
  return data.at(index + kItemHashOffset).isSmallInt();
}

static bool itemIsTombstone(RawMutableTuple data, word index) {
  return data.at(index + kItemHashOffset).isUnbound();
}

}  // namespace

// Returns one of the three possible values:
// - `key` was found at indices[bytes_offset] : SmallInt::fromWord(bytes_offset)
// - `key` was not found : SmallInt::fromWord(-1)
// - Exception that was raised from key comparison __eq__ function.
static RawObject dictLookup(Thread* thread, const MutableTuple& data,
                            MutableBytes& indices, word num_indices,
                            const Object& key, word hash) {
  DCHECK(data.length() > 0, "data shouldn't be empty");
  uword perturb;
  word indices_mask;
  RawSmallInt hash_int = SmallInt::fromWord(hash);
  for (word current_index =
           probeBegin(num_indices, hash, &indices_mask, &perturb);
       ; current_index = probeNext(current_index, indices_mask, &perturb)) {
    word bytes_offset = indexOffset(current_index);
    uint32_t item_index;
    if (indicesIsFull(*indices, bytes_offset, &item_index)) {
      if (itemHashRaw(*data, item_index) == hash_int) {
        RawObject eq =
            Runtime::objectEquals(thread, itemKey(*data, item_index), *key);
        if (eq == Bool::trueObj()) {
          return SmallInt::fromWord(bytes_offset);
        }
        if (UNLIKELY(eq.isErrorException())) {
          return eq;
        }
      }
      continue;
    }
    if (indicesIsEmpty(*indices, bytes_offset)) {
      return SmallInt::fromWord(-1);
    }
  }
  UNREACHABLE("Expected to have found an empty index");
}

// Returns one of the three possible values:
// - `key` was found at indices[bytes_offset] : SmallInt::fromWord(bytes_offset)
// - `key` was not found, but insertion can be done to indices[bytes_offset] :
//   SmallInt::fromWord(bytes_offset - data.length())
// - Exception that was raised from key comparison __eq__ function.
static RawObject dictLookupForInsertion(Thread* thread,
                                        const MutableTuple& data,
                                        MutableBytes& indices, word num_indices,
                                        const Object& key, word hash) {
  DCHECK(data.length() > 0, "data shouldn't be empty");
  word next_free_index = -1;
  uword perturb;
  word indices_mask;
  RawSmallInt hash_int = SmallInt::fromWord(hash);
  for (word current_index =
           probeBegin(num_indices, hash, &indices_mask, &perturb);
       ; current_index = probeNext(current_index, indices_mask, &perturb)) {
    word bytes_offset = indexOffset(current_index);
    uint32_t item_index;
    if (indicesIsFull(*indices, bytes_offset, &item_index)) {
      if (itemHashRaw(*data, item_index) == hash_int) {
        RawObject eq =
            Runtime::objectEquals(thread, itemKey(*data, item_index), *key);
        if (eq == Bool::trueObj()) {
          return SmallInt::fromWord(bytes_offset);
        }
        if (UNLIKELY(eq.isErrorException())) {
          return eq;
        }
      }
      continue;
    }
    if (next_free_index == -1) {
      next_free_index = bytes_offset;
    }
    if (indicesIsEmpty(*indices, bytes_offset)) {
      return SmallInt::fromWord(next_free_index - indexOffset(num_indices));
    }
  }
  UNREACHABLE("Expected to have found an empty index");
}

static word nextItemIndex(RawObject data, word* index, word end) {
  for (word i = *index; i < end; i += kItemNumPointers) {
    if (!itemIsFull(MutableTuple::cast(data), i)) continue;
    *index = i + kItemNumPointers;
    return i;
  }
  return -1;
}

bool dictNextItem(const Dict& dict, word* index, Object* key_out,
                  Object* value_out) {
  RawObject data = dict.data();
  word next_item_index = nextItemIndex(data, index, dict.firstEmptyItemIndex());
  if (next_item_index < 0) {
    return false;
  }
  *key_out = itemKey(MutableTuple::cast(data), next_item_index);
  *value_out = itemValue(MutableTuple::cast(data), next_item_index);
  return true;
}

bool dictNextItemHash(const Dict& dict, word* index, Object* key_out,
                      Object* value_out, word* hash_out) {
  RawObject data = dict.data();
  word next_item_index = nextItemIndex(data, index, dict.firstEmptyItemIndex());
  if (next_item_index < 0) {
    return false;
  }
  *key_out = itemKey(MutableTuple::cast(data), next_item_index);
  *value_out = itemValue(MutableTuple::cast(data), next_item_index);
  *hash_out = itemHash(MutableTuple::cast(data), next_item_index);
  return true;
}

bool dictNextKey(const Dict& dict, word* index, Object* key_out) {
  RawObject data = dict.data();
  word next_item_index = nextItemIndex(data, index, dict.firstEmptyItemIndex());
  if (next_item_index < 0) {
    return false;
  }
  *key_out = itemKey(MutableTuple::cast(data), next_item_index);
  return true;
}

bool dictNextKeyHash(const Dict& dict, word* index, Object* key_out,
                     word* hash_out) {
  RawObject data = dict.data();
  word next_item_index = nextItemIndex(data, index, dict.firstEmptyItemIndex());
  if (next_item_index < 0) {
    return false;
  }
  *key_out = itemKey(MutableTuple::cast(data), next_item_index);
  *hash_out = itemHash(MutableTuple::cast(data), next_item_index);
  return true;
}

bool dictNextValue(const Dict& dict, word* index, Object* value_out) {
  RawObject data = dict.data();
  word next_item_index = nextItemIndex(data, index, dict.firstEmptyItemIndex());
  if (next_item_index < 0) {
    return false;
  }
  *value_out = itemValue(MutableTuple::cast(data), next_item_index);
  return true;
}

static word sizeOfDataTuple(word num_indices) { return (num_indices * 2) / 3; }

static const word kDictGrowthFactor = 2;
// Initial size of the dict. According to comments in CPython's
// dictobject.c this accommodates the majority of dictionaries without needing
// a resize (obviously this depends on the load factor used to resize the
// dict).
static const word kInitialDictIndicesLength = 8;

void dictAllocateArrays(Thread* thread, const Dict& dict, word num_indices) {
  num_indices = Utils::maximum(num_indices, kInitialDictIndicesLength);
  DCHECK(Utils::isPowerOfTwo(num_indices), "%ld is not a power of 2",
         num_indices);
  Runtime* runtime = thread->runtime();
  dict.setData(runtime->newMutableTuple(sizeOfDataTuple(num_indices) *
                                        kItemNumPointers));
  MutableTuple::cast(dict.data()).fill(NoneType::object());
  dict.setIndices(
      runtime->mutableBytesWith(indexOffset(num_indices), kMaxByte));
  dict.setFirstEmptyItemIndex(0);
}

// Return true if `dict` has an available item for insertion.
static bool dictHasUsableItem(const Dict& dict) {
  return dict.firstEmptyItemIndex() <
         RawMutableTuple::cast(dict.data()).length();
}

// Insert `key`/`value` into dictionary assuming no item with an equivalent
// key and no tombstones exist.
static void dictInsertNoUpdate(const MutableTuple& data,
                               const MutableBytes& indices, word num_indices,
                               word item_count, const Object& key, word hash,
                               const Object& value) {
  DCHECK(data.length() > 0, "dict must not be empty");
  uword perturb;
  word indices_mask;
  for (word current_index =
           probeBegin(num_indices, hash, &indices_mask, &perturb);
       ; current_index = probeNext(current_index, indices_mask, &perturb)) {
    word bytes_offset = indexOffset(current_index);
    if (indicesIsEmpty(*indices, bytes_offset)) {
      uint32_t item_index = item_count * kItemNumPointers;
      itemSet(*data, item_index, hash, *key, *value);
      itemIndexAtPut(*indices, bytes_offset, item_index);
      return;
    }
  }
}

static void dictEnsureCapacity(Thread* thread, const Dict& dict) {
  DCHECK(dict.numIndices() && Utils::isPowerOfTwo(dict.numIndices()),
         "dict capacity must be power of two, greater than zero");
  if (dictHasUsableItem(dict)) {
    return;
  }

  // TODO(T44247845): Handle overflow here.
  word new_num_indices = dict.numIndices() * kDictGrowthFactor;
  DCHECK(new_num_indices < kTombstoneValue,
         "new_num_indices is expected to be less than kTombstoneValue");
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  MutableTuple new_data(
      &scope, runtime->newMutableTuple(sizeOfDataTuple(new_num_indices) *
                                       kItemNumPointers));
  new_data.fill(NoneType::object());
  MutableBytes new_indices(&scope, runtime->mutableBytesWith(
                                       indexOffset(new_num_indices), kMaxByte));

  // Re-insert items
  Object key(&scope, NoneType::object());
  Object value(&scope, NoneType::object());
  word hash;
  word item_count = 0;
  for (word i = 0; dictNextItemHash(dict, &i, &key, &value, &hash);
       item_count++) {
    dictInsertNoUpdate(new_data, new_indices, new_num_indices, item_count, key,
                       hash, value);
  }
  DCHECK(item_count == dict.numItems(), "found entries != dictNumItems()");
  dict.setData(*new_data);
  dict.setIndices(*new_indices);
  dict.setFirstEmptyItemIndex(dict.numItems() * kItemNumPointers);
}

RawObject dictAtPut(Thread* thread, const Dict& dict, const Object& key,
                    word hash, const Object& value) {
  if (dict.indices() == SmallInt::fromWord(0)) {
    dictAllocateArrays(thread, dict, kInitialDictIndicesLength);
  }
  HandleScope scope(thread);
  MutableTuple data(&scope, dict.data());
  MutableBytes indices(&scope, dict.indices());
  word num_indices = dict.numIndices();
  RawObject lookup_result =
      dictLookupForInsertion(thread, data, indices, num_indices, key, hash);
  if (UNLIKELY(lookup_result.isErrorException())) {
    return lookup_result;
  }
  word bytes_offset = SmallInt::cast(lookup_result).value();
  if (bytes_offset >= 0) {
    uint32_t item_index = itemIndexAt(*indices, bytes_offset);
    itemSetValue(*data, item_index, *value);
    return NoneType::object();
  }

  bytes_offset += indexOffset(num_indices);
  word item_index = dict.firstEmptyItemIndex();
  DCHECK(itemIsEmpty(*data, item_index), "item is expected to be empty");
  itemSet(*data, item_index, hash, *key, *value);
  itemIndexAtPut(*indices, bytes_offset, item_index);
  dict.setNumItems(dict.numItems() + 1);
  dict.setFirstEmptyItemIndex(dict.firstEmptyItemIndex() + kItemNumPointers);
  dictEnsureCapacity(thread, dict);
  DCHECK(dictHasUsableItem(dict), "dict must have an empty item left");
  return NoneType::object();
}

void dictAtPutByStr(Thread* thread, const Dict& dict, const Object& name,
                    const Object& value) {
  word hash = strHash(thread, *name);
  UNUSED RawObject result = dictAtPut(thread, dict, name, hash, value);
  DCHECK(!result.isError(), "result must not be an error");
}

void dictAtPutById(Thread* thread, const Dict& dict, SymbolId id,
                   const Object& value) {
  HandleScope scope(thread);
  Object name(&scope, thread->runtime()->symbols()->at(id));
  dictAtPutByStr(thread, dict, name, value);
}

RawObject dictAt(Thread* thread, const Dict& dict, const Object& key,
                 word hash) {
  if (dict.numItems() == 0) {
    return Error::notFound();
  }
  HandleScope scope(thread);
  MutableTuple data(&scope, dict.data());
  MutableBytes indices(&scope, dict.indices());
  RawObject lookup_result =
      dictLookup(thread, data, indices, dict.numIndices(), key, hash);
  if (UNLIKELY(lookup_result.isErrorException())) {
    return lookup_result;
  }
  word bytes_offset = SmallInt::cast(lookup_result).value();
  if (bytes_offset >= 0) {
    uint32_t item_index = itemIndexAt(*indices, bytes_offset);
    return itemValue(*data, item_index);
  }
  return Error::notFound();
}

RawObject dictAtByStr(Thread* thread, const Dict& dict, const Object& name) {
  word hash = strHash(thread, *name);
  return dictAt(thread, dict, name, hash);
}

RawObject dictAtById(Thread* thread, const Dict& dict, SymbolId id) {
  HandleScope scope(thread);
  Object name(&scope, thread->runtime()->symbols()->at(id));
  return dictAtByStr(thread, dict, name);
}

RawObject dictAtPutInValueCellByStr(Thread* thread, const Dict& dict,
                                    const Object& name, const Object& value) {
  if (dict.indices() == SmallInt::fromWord(0)) {
    dictAllocateArrays(thread, dict, kInitialDictIndicesLength);
  }
  HandleScope scope(thread);
  MutableTuple data(&scope, dict.data());
  MutableBytes indices(&scope, dict.indices());
  word num_indices = dict.numIndices();
  word hash = strHash(thread, *name);
  RawObject lookup_result =
      dictLookupForInsertion(thread, data, indices, num_indices, name, hash);
  if (UNLIKELY(lookup_result.isErrorException())) {
    return lookup_result;
  }
  word bytes_offset = SmallInt::cast(lookup_result).value();
  if (bytes_offset >= 0) {
    uint32_t item_index = itemIndexAt(*indices, bytes_offset);
    RawValueCell value_cell = ValueCell::cast(itemValue(*data, item_index));
    value_cell.setValue(*value);
    return value_cell;
  }
  bytes_offset += indexOffset(num_indices);
  word item_index = dict.firstEmptyItemIndex();
  DCHECK(itemIsEmpty(*data, item_index), "item is expected to be empty");
  ValueCell value_cell(&scope, thread->runtime()->newValueCell());
  itemSet(*data, item_index, hash, *name, *value_cell);
  itemIndexAtPut(*indices, bytes_offset, item_index);
  dict.setNumItems(dict.numItems() + 1);
  dict.setFirstEmptyItemIndex(dict.firstEmptyItemIndex() + kItemNumPointers);
  dictEnsureCapacity(thread, dict);
  DCHECK(dictHasUsableItem(dict), "dict must have an empty item left");
  value_cell.setValue(*value);
  return *value_cell;
}

void dictClear(Thread* thread, const Dict& dict) {
  if (dict.indices() == SmallInt::fromWord(0)) return;

  HandleScope scope(thread);
  MutableTuple data(&scope, dict.data());
  data.fill(NoneType::object());
  MutableBytes indices(&scope, dict.indices());
  indices.replaceFromWithByte(0, kMaxByte, indices.length());
  dict.setNumItems(0);
  dict.setFirstEmptyItemIndex(0);
}

RawObject dictIncludes(Thread* thread, const Dict& dict, const Object& key,
                       word hash) {
  if (dict.numItems() == 0) {
    return Bool::falseObj();
  }
  HandleScope scope(thread);
  MutableTuple data(&scope, dict.data());
  MutableBytes indices(&scope, dict.indices());
  word num_indices = dict.numIndices();
  RawObject lookup_result =
      dictLookup(thread, data, indices, num_indices, key, hash);
  if (UNLIKELY(lookup_result.isErrorException())) {
    return lookup_result;
  }
  return Bool::fromBool(SmallInt::cast(lookup_result).value() >= 0);
}

RawObject dictRemoveByStr(Thread* thread, const Dict& dict,
                          const Object& name) {
  word hash = strHash(thread, *name);
  return dictRemove(thread, dict, name, hash);
}

RawObject dictRemove(Thread* thread, const Dict& dict, const Object& key,
                     word hash) {
  if (dict.numItems() == 0) {
    return Error::notFound();
  }
  HandleScope scope(thread);
  MutableTuple data(&scope, dict.data());
  MutableBytes indices(&scope, dict.indices());
  word num_indices = dict.numIndices();
  RawObject lookup_result =
      dictLookup(thread, data, indices, num_indices, key, hash);
  if (UNLIKELY(lookup_result.isErrorException())) {
    return lookup_result;
  }
  word bytes_offset = SmallInt::cast(lookup_result).value();
  if (bytes_offset < 0) {
    return Error::notFound();
  }
  uint32_t item_index = itemIndexAt(*indices, bytes_offset);
  Object result(&scope, itemValue(*data, item_index));
  itemSetTombstone(*data, item_index);
  indicesSetTombstone(*indices, bytes_offset);
  dict.setNumItems(dict.numItems() - 1);
  return *result;
}

RawObject dictKeys(Thread* thread, const Dict& dict) {
  word len = dict.numItems();
  Runtime* runtime = thread->runtime();
  if (len == 0) {
    return runtime->newList();
  }
  HandleScope scope(thread);
  MutableTuple keys(&scope, runtime->newMutableTuple(len));
  Object key(&scope, NoneType::object());
  word num_keys = 0;
  for (word i = 0; dictNextKey(dict, &i, &key);) {
    DCHECK(num_keys < keys.length(), "%ld ! < %ld", num_keys, keys.length());
    keys.atPut(num_keys, *key);
    num_keys++;
  }
  List result(&scope, runtime->newList());
  result.setItems(*keys);
  result.setNumItems(len);
  return *result;
}

RawObject dictCopy(Thread* thread, const Dict& dict) {
  HandleScope scope(thread);
  Dict copy(&scope, thread->runtime()->newDict());
  Object result(&scope, dictMergeError(thread, copy, dict));
  if (result.isError()) {
    return *result;
  }
  return *copy;
}

namespace {
enum class Override {
  kIgnore,
  kOverride,
  kError,
};

RawObject dictMergeDict(Thread* thread, const Dict& dict, const Object& mapping,
                        Override do_override) {
  HandleScope scope(thread);
  if (*mapping == *dict) return NoneType::object();

  Object key(&scope, NoneType::object());
  Object value(&scope, NoneType::object());
  word hash;
  Dict other(&scope, *mapping);
  Object included(&scope, NoneType::object());
  Object dict_result(&scope, NoneType::object());
  for (word i = 0; dictNextItemHash(other, &i, &key, &value, &hash);) {
    if (do_override == Override::kOverride) {
      dict_result = dictAtPut(thread, dict, key, hash, value);
      if (dict_result.isErrorException()) return *dict_result;
    } else {
      included = dictIncludes(thread, dict, key, hash);
      if (included.isErrorException()) return *included;
      if (included == Bool::falseObj()) {
        dict_result = dictAtPut(thread, dict, key, hash, value);
        if (dict_result.isErrorException()) return *dict_result;
      } else if (do_override == Override::kError) {
        return thread->raise(LayoutId::kKeyError, *key);
      }
    }
  }
  return NoneType::object();
}

RawObject dictMergeImpl(Thread* thread, const Dict& dict, const Object& mapping,
                        Override do_override) {
  Runtime* runtime = thread->runtime();
  if (runtime->isInstanceOfDict(*mapping)) {
    return dictMergeDict(thread, dict, mapping, do_override);
  }

  HandleScope scope(thread);
  Object key(&scope, NoneType::object());
  Object hash_obj(&scope, NoneType::object());
  Object value(&scope, NoneType::object());
  Object included(&scope, NoneType::object());
  Object keys_method(&scope,
                     runtime->attributeAtById(thread, mapping, ID(keys)));
  if (keys_method.isError()) {
    return *keys_method;
  }

  // Generic mapping, use keys() and __getitem__()
  Object subscr_method(
      &scope, runtime->attributeAtById(thread, mapping, ID(__getitem__)));

  if (subscr_method.isError()) {
    return *subscr_method;
  }
  Object keys(&scope, Interpreter::callMethod1(thread, keys_method, mapping));
  if (keys.isError()) return *keys;

  if (keys.isList()) {
    List keys_list(&scope, *keys);
    Object dict_result(&scope, NoneType::object());
    for (word i = 0; i < keys_list.numItems(); ++i) {
      key = keys_list.at(i);
      hash_obj = Interpreter::hash(thread, key);
      if (hash_obj.isErrorException()) return *hash_obj;
      word hash = SmallInt::cast(*hash_obj).value();
      if (do_override == Override::kOverride) {
        value = Interpreter::callMethod2(thread, subscr_method, mapping, key);
        if (value.isError()) return *value;
        dict_result = dictAtPut(thread, dict, key, hash, value);
        if (dict_result.isErrorException()) return *dict_result;
      } else {
        included = dictIncludes(thread, dict, key, hash);
        if (included.isErrorException()) return *included;
        if (included == Bool::falseObj()) {
          value = Interpreter::callMethod2(thread, subscr_method, mapping, key);
          if (value.isError()) return *value;
          dict_result = dictAtPut(thread, dict, key, hash, value);
          if (dict_result.isErrorException()) return *dict_result;
        } else if (do_override == Override::kError) {
          return thread->raise(LayoutId::kKeyError, *key);
        }
      }
    }
    return NoneType::object();
  }

  if (keys.isTuple()) {
    Tuple keys_tuple(&scope, *keys);
    Object dict_result(&scope, NoneType::object());
    for (word i = 0; i < keys_tuple.length(); ++i) {
      key = keys_tuple.at(i);
      hash_obj = Interpreter::hash(thread, key);
      if (hash_obj.isErrorException()) return *hash_obj;
      word hash = SmallInt::cast(*hash_obj).value();
      if (do_override == Override::kOverride) {
        value = Interpreter::callMethod2(thread, subscr_method, mapping, key);
        if (value.isError()) return *value;
        dict_result = dictAtPut(thread, dict, key, hash, value);
        if (dict_result.isErrorException()) return *dict_result;
      } else {
        included = dictIncludes(thread, dict, key, hash);
        if (included.isErrorException()) return *included;
        if (included == Bool::falseObj()) {
          value = Interpreter::callMethod2(thread, subscr_method, mapping, key);
          if (value.isError()) return *value;
          dict_result = dictAtPut(thread, dict, key, hash, value);
          if (dict_result.isErrorException()) return *dict_result;
        } else if (do_override == Override::kError) {
          return thread->raise(LayoutId::kKeyError, *key);
        }
      }
    }
    return NoneType::object();
  }

  // keys is probably an iterator
  Object iter_method(&scope,
                     Interpreter::lookupMethod(thread, keys, ID(__iter__)));
  if (iter_method.isError()) {
    return thread->raiseWithFmt(LayoutId::kTypeError, "keys() is not iterable");
  }

  Object iterator(&scope, Interpreter::callMethod1(thread, iter_method, keys));
  if (iterator.isError()) {
    return thread->raiseWithFmt(LayoutId::kTypeError, "keys() is not iterable");
  }
  Object next_method(&scope,
                     Interpreter::lookupMethod(thread, iterator, ID(__next__)));
  if (next_method.isError()) {
    return thread->raiseWithFmt(LayoutId::kTypeError, "keys() is not iterable");
  }
  Object dict_result(&scope, NoneType::object());
  for (;;) {
    key = Interpreter::callMethod1(thread, next_method, iterator);
    if (key.isError()) {
      if (thread->clearPendingStopIteration()) break;
      return *key;
    }
    hash_obj = Interpreter::hash(thread, key);
    if (hash_obj.isErrorException()) return *hash_obj;
    word hash = SmallInt::cast(*hash_obj).value();
    if (do_override == Override::kOverride) {
      value = Interpreter::callMethod2(thread, subscr_method, mapping, key);
      if (value.isError()) return *value;
      dict_result = dictAtPut(thread, dict, key, hash, value);
      if (dict_result.isErrorException()) return *dict_result;
    } else {
      included = dictIncludes(thread, dict, key, hash);
      if (included.isErrorException()) return *included;
      if (included == Bool::falseObj()) {
        value = Interpreter::callMethod2(thread, subscr_method, mapping, key);
        if (value.isError()) return *value;
        dict_result = dictAtPut(thread, dict, key, hash, value);
        if (dict_result.isErrorException()) return *dict_result;
      } else if (do_override == Override::kError) {
        return thread->raise(LayoutId::kKeyError, *key);
      }
    }
  }
  return NoneType::object();
}
}  // namespace

RawObject dictMergeOverride(Thread* thread, const Dict& dict,
                            const Object& mapping) {
  return dictMergeImpl(thread, dict, mapping, Override::kOverride);
}

RawObject dictMergeError(Thread* thread, const Dict& dict,
                         const Object& mapping) {
  return dictMergeImpl(thread, dict, mapping, Override::kError);
}

RawObject dictMergeIgnore(Thread* thread, const Dict& dict,
                          const Object& mapping) {
  return dictMergeImpl(thread, dict, mapping, Override::kIgnore);
}

RawObject dictEq(Thread* thread, const Dict& left, const Dict& right) {
  if (left.numItems() != right.numItems()) {
    return Bool::falseObj();
  }
  HandleScope scope(thread);
  Object key(&scope, NoneType::object());
  Object left_value(&scope, NoneType::object());
  word hash;
  Object right_value(&scope, NoneType::object());
  Object result(&scope, NoneType::object());
  for (word i = 0; dictNextItemHash(left, &i, &key, &left_value, &hash);) {
    right_value = dictAt(thread, right, key, hash);
    if (right_value.isErrorNotFound()) {
      return Bool::falseObj();
    }
    if (left_value == right_value) {
      continue;
    }
    result = Interpreter::compareOperation(thread, EQ, left_value, right_value);
    if (result.isErrorException()) {
      // equality comparison raised
      return *result;
    }
    result = Interpreter::isTrue(thread, *result);
    if (result != Bool::trueObj()) {
      // bool conversion raised or returned false
      return *result;
    }
  }
  return Bool::trueObj();
}

RawObject dictItemIteratorNext(Thread* thread, const DictItemIterator& iter) {
  HandleScope scope(thread);
  Dict dict(&scope, iter.iterable());
  Object key(&scope, NoneType::object());
  Object value(&scope, NoneType::object());
  word i = iter.index();
  if (dictNextItem(dict, &i, &key, &value)) {
    iter.setIndex(i);
    iter.setNumFound(iter.numFound() + 1);
    return thread->runtime()->newTupleWith2(key, value);
  }

  // We hit the end.
  iter.setIndex(i);
  return Error::noMoreItems();
}

RawObject dictKeyIteratorNext(Thread* thread, const DictKeyIterator& iter) {
  HandleScope scope(thread);
  Dict dict(&scope, iter.iterable());
  Object key(&scope, NoneType::object());
  word i = iter.index();
  if (dictNextKey(dict, &i, &key)) {
    iter.setIndex(i);
    iter.setNumFound(iter.numFound() + 1);
    return *key;
  }

  // We hit the end.
  iter.setIndex(i);
  return Error::noMoreItems();
}

RawObject dictValueIteratorNext(Thread* thread, const DictValueIterator& iter) {
  HandleScope scope(thread);
  Dict dict(&scope, iter.iterable());
  Object value(&scope, NoneType::object());
  word i = iter.index();
  if (dictNextValue(dict, &i, &value)) {
    iter.setIndex(i);
    iter.setNumFound(iter.numFound() + 1);
    return *value;
  }

  // We hit the end.
  iter.setIndex(i);
  return Error::noMoreItems();
}

static const BuiltinAttribute kDictAttributes[] = {
    {ID(_dict__num_items), RawDict::kNumItemsOffset, AttributeFlags::kHidden},
    {ID(_dict__data), RawDict::kDataOffset, AttributeFlags::kHidden},
    {ID(_dict__sparse), RawDict::kIndicesOffset, AttributeFlags::kHidden},
    {ID(_dict__first_empty_item_index), RawDict::kFirstEmptyItemIndexOffset,
     AttributeFlags::kHidden},
};

static const BuiltinAttribute kDictItemIteratorAttributes[] = {
    {ID(_dict_item_iterator__iterable), RawDictItemIterator::kIterableOffset,
     AttributeFlags::kHidden},
    {ID(_dict_item_iterator__index), RawDictItemIterator::kIndexOffset,
     AttributeFlags::kHidden},
    {ID(_dict_item_iterator__num_found), RawDictItemIterator::kNumFoundOffset,
     AttributeFlags::kHidden},
};

static const BuiltinAttribute kDictItemsAttributes[] = {
    {ID(_dict_items__dict), RawDictItems::kDictOffset, AttributeFlags::kHidden},
};

static const BuiltinAttribute kDictKeyIteratorAttributes[] = {
    {ID(_dict_key_iterator__iterable), RawDictKeyIterator::kIterableOffset,
     AttributeFlags::kHidden},
    {ID(_dict_key_iterator__index), RawDictKeyIterator::kIndexOffset,
     AttributeFlags::kHidden},
    {ID(_dict_key_iterator__num_found), RawDictKeyIterator::kNumFoundOffset,
     AttributeFlags::kHidden},
};

static const BuiltinAttribute kDictKeysAttributes[] = {
    {ID(_dict_keys__dict), RawDictKeys::kDictOffset, AttributeFlags::kHidden},
};

static const BuiltinAttribute kDictValueIteratorAttributes[] = {
    {ID(_dict_value_iterator__iterable), RawDictValueIterator::kIterableOffset,
     AttributeFlags::kHidden},
    {ID(_dict_value_iterator__index), RawDictValueIterator::kIndexOffset,
     AttributeFlags::kHidden},
    {ID(_dict_value_iterator__num_found), RawDictValueIterator::kNumFoundOffset,
     AttributeFlags::kHidden},
};

static const BuiltinAttribute kDictValuesAttributes[] = {
    {ID(_dict_values__dict), RawDictValues::kDictOffset,
     AttributeFlags::kHidden},
};

void initializeDictTypes(Thread* thread) {
  addBuiltinType(thread, ID(dict), LayoutId::kDict,
                 /*superclass_id=*/LayoutId::kObject, kDictAttributes,
                 Dict::kSize, /*basetype=*/true);

  addBuiltinType(thread, ID(dict_itemiterator), LayoutId::kDictItemIterator,
                 /*superclass_id=*/LayoutId::kObject,
                 kDictItemIteratorAttributes, DictItemIterator::kSize,
                 /*basetype=*/false);

  addBuiltinType(thread, ID(dict_items), LayoutId::kDictItems,
                 /*superclass_id=*/LayoutId::kObject, kDictItemsAttributes,
                 DictItems::kSize, /*basetype=*/false);

  addBuiltinType(thread, ID(dict_keyiterator), LayoutId::kDictKeyIterator,
                 /*superclass_id=*/LayoutId::kObject,
                 kDictKeyIteratorAttributes, DictKeyIterator::kSize,
                 /*basetype=*/false);

  addBuiltinType(thread, ID(dict_keys), LayoutId::kDictKeys,
                 /*superclass_id=*/LayoutId::kObject, kDictKeysAttributes,
                 DictKeys::kSize, /*basetype=*/false);

  addBuiltinType(thread, ID(dict_valueiterator), LayoutId::kDictValueIterator,
                 /*superclass_id=*/LayoutId::kObject,
                 kDictValueIteratorAttributes, DictValueIterator::kSize,
                 /*basetype=*/false);

  addBuiltinType(thread, ID(dict_values), LayoutId::kDictValues,
                 /*superclass_id=*/LayoutId::kObject, kDictValuesAttributes,
                 DictValues::kSize, /*basetype=*/false);
}

RawObject METH(dict, clear)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfDict(*self)) {
    return thread->raiseRequiresType(self, ID(dict));
  }
  Dict dict(&scope, *self);
  dictClear(thread, dict);
  return NoneType::object();
}

RawObject METH(dict, __delitem__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Object key(&scope, args.get(1));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfDict(*self)) {
    return thread->raiseRequiresType(self, ID(dict));
  }
  Dict dict(&scope, *self);
  Object hash_obj(&scope, Interpreter::hash(thread, key));
  if (hash_obj.isErrorException()) return *hash_obj;
  word hash = SmallInt::cast(*hash_obj).value();
  // Remove the key. If it doesn't exist, throw a KeyError.
  if (dictRemove(thread, dict, key, hash).isError()) {
    return thread->raise(LayoutId::kKeyError, *key);
  }
  return NoneType::object();
}

RawObject METH(dict, __eq__)(Thread* thread, Arguments args) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfDict(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(dict));
  }
  Object other_obj(&scope, args.get(1));
  if (!runtime->isInstanceOfDict(*other_obj)) {
    return NotImplementedType::object();
  }
  Dict a(&scope, *self_obj);
  Dict b(&scope, *other_obj);
  return dictEq(thread, a, b);
}

RawObject METH(dict, __len__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfDict(*self)) {
    return thread->raiseRequiresType(self, ID(dict));
  }
  Dict dict(&scope, *self);
  return SmallInt::fromWord(dict.numItems());
}

RawObject METH(dict, __iter__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfDict(*self)) {
    return thread->raiseRequiresType(self, ID(dict));
  }
  Dict dict(&scope, *self);
  // .iter() on a dict returns a keys iterator
  return runtime->newDictKeyIterator(thread, dict);
}

RawObject METH(dict, items)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfDict(*self)) {
    return thread->raiseRequiresType(self, ID(dict));
  }
  Dict dict(&scope, *self);
  return runtime->newDictItems(thread, dict);
}

RawObject METH(dict, keys)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfDict(*self)) {
    return thread->raiseRequiresType(self, ID(dict));
  }
  Dict dict(&scope, *self);
  return runtime->newDictKeys(thread, dict);
}

RawObject METH(dict, popitem)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfDict(*self)) {
    return thread->raiseRequiresType(self, ID(dict));
  }
  Dict dict(&scope, *self);
  if (dict.numItems() == 0) {
    return thread->raiseWithFmt(LayoutId::kKeyError,
                                "popitem(): dictionary is empty");
  }
  MutableTuple data(&scope, dict.data());
  for (word item_index = dict.firstEmptyItemIndex() - kItemNumPointers;
       item_index >= 0; item_index -= kItemNumPointers) {
    if (!itemIsEmpty(*data, item_index) &&
        !itemIsTombstone(*data, item_index)) {
      Object key(&scope, itemKey(*data, item_index));
      Object value(&scope, itemValue(*data, item_index));
      Tuple result(&scope, runtime->newTupleWith2(key, value));
      // Find the item for the entry and set a tombstone in it.
      // Note that this would take amortized cost O(1).
      word indices_index = -1;
      MutableBytes indices(&scope, dict.indices());
      uword perturb;
      word indices_mask;
      word hash = itemHash(*data, item_index);
      word num_indices = dict.numIndices();
      for (word current_index =
               probeBegin(num_indices, hash, &indices_mask, &perturb);
           ; current_index = probeNext(current_index, indices_mask, &perturb)) {
        word bytes_offset = indexOffset(current_index);
        uint32_t comp;
        if (indicesIsFull(*indices, bytes_offset, &comp) &&
            comp == item_index) {
          indices_index = current_index;
          break;
        }
      }
      DCHECK(indices_index >= 0, "cannot find index for entry in dict.sparse");
      itemSetTombstone(*data, item_index);
      indicesSetTombstone(*indices, indexOffset(indices_index));
      dict.setNumItems(dict.numItems() - 1);
      return *result;
    }
  }
  UNREACHABLE("dict.numItems() > 0, but couldn't find any active item");
}

RawObject METH(dict, values)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfDict(*self)) {
    return thread->raiseRequiresType(self, ID(dict));
  }
  Dict dict(&scope, *self);
  return runtime->newDictValues(thread, dict);
}

RawObject METH(dict, __new__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object type_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfType(*type_obj)) {
    return thread->raiseWithFmt(LayoutId::kTypeError, "not a type object");
  }
  Type type(&scope, *type_obj);
  if (type.builtinBase() != LayoutId::kDict) {
    return thread->raiseWithFmt(LayoutId::kTypeError, "not a subtype of dict");
  }
  Layout layout(&scope, type.instanceLayout());
  Dict result(&scope, runtime->newInstance(layout));
  result.setNumItems(0);
  result.setData(SmallInt::fromWord(0));
  result.setIndices(SmallInt::fromWord(0));
  result.setFirstEmptyItemIndex(0);
  return *result;
}

RawObject METH(dict, __contains__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Object key(&scope, args.get(1));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfDict(*self)) {
    return thread->raiseRequiresType(self, ID(dict));
  }
  Object hash_obj(&scope, Interpreter::hash(thread, key));
  if (hash_obj.isErrorException()) {
    return *hash_obj;
  }
  Dict dict(&scope, *self);
  word hash = SmallInt::cast(*hash_obj).value();
  return dictIncludes(thread, dict, key, hash);
}

RawObject METH(dict, pop)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Object key(&scope, args.get(1));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfDict(*self)) {
    return thread->raiseRequiresType(self, ID(dict));
  }
  Dict dict(&scope, *self);
  Object hash_obj(&scope, Interpreter::hash(thread, key));
  if (hash_obj.isErrorException()) {
    return *hash_obj;
  }
  word hash = SmallInt::cast(*hash_obj).value();
  Object result(&scope, dictRemove(thread, dict, key, hash));
  if (result.isErrorNotFound()) {
    Object default_obj(&scope, args.get(2));
    return default_obj.isUnbound() ? thread->raise(LayoutId::kKeyError, *key)
                                   : *default_obj;
  }
  return *result;
}

// TODO(T35787656): Instead of re-writing everything for every class, make a
// helper function that takes a member function (type check) and string for the
// Python symbol name

RawObject METH(dict_itemiterator, __iter__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isDictItemIterator()) {
    return thread->raiseRequiresType(self, ID(dict_itemiterator));
  }
  return *self;
}

RawObject METH(dict_itemiterator, __next__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isDictItemIterator()) {
    return thread->raiseRequiresType(self, ID(dict_itemiterator));
  }
  DictItemIterator iter(&scope, *self);
  Object value(&scope, dictItemIteratorNext(thread, iter));
  if (value.isError()) {
    return thread->raise(LayoutId::kStopIteration, NoneType::object());
  }
  return *value;
}

RawObject METH(dict_itemiterator, __length_hint__)(Thread* thread,
                                                   Arguments args) {
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isDictItemIterator()) {
    return thread->raiseRequiresType(self, ID(dict_itemiterator));
  }
  DictItemIterator iter(&scope, *self);
  Dict dict(&scope, iter.iterable());
  return SmallInt::fromWord(dict.numItems() - iter.numFound());
}

RawObject METH(dict_items, __iter__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isDictItems()) {
    return thread->raiseRequiresType(self, ID(dict_items));
  }

  Dict dict(&scope, DictItems::cast(*self).dict());
  return thread->runtime()->newDictItemIterator(thread, dict);
}

RawObject METH(dict_items, __len__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isDictItems()) {
    return thread->raiseRequiresType(self, ID(dict_items));
  }

  Dict dict(&scope, DictItems::cast(*self).dict());
  return thread->runtime()->newInt(dict.numItems());
}

RawObject METH(dict_keyiterator, __iter__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isDictKeyIterator()) {
    return thread->raiseRequiresType(self, ID(dict_keyiterator));
  }
  return *self;
}

RawObject METH(dict_keyiterator, __next__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isDictKeyIterator()) {
    return thread->raiseRequiresType(self, ID(dict_keyiterator));
  }
  DictKeyIterator iter(&scope, *self);
  Object value(&scope, dictKeyIteratorNext(thread, iter));
  if (value.isError()) {
    return thread->raise(LayoutId::kStopIteration, NoneType::object());
  }
  return *value;
}

RawObject METH(dict_keyiterator, __length_hint__)(Thread* thread,
                                                  Arguments args) {
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isDictKeyIterator()) {
    return thread->raiseRequiresType(self, ID(dict_keyiterator));
  }
  DictKeyIterator iter(&scope, *self);
  Dict dict(&scope, iter.iterable());
  return SmallInt::fromWord(dict.numItems() - iter.numFound());
}

RawObject METH(dict_keys, __iter__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isDictKeys()) {
    return thread->raiseRequiresType(self, ID(dict_keys));
  }

  Dict dict(&scope, DictKeys::cast(*self).dict());
  return thread->runtime()->newDictKeyIterator(thread, dict);
}

RawObject METH(dict_keys, __len__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isDictKeys()) {
    return thread->raiseRequiresType(self, ID(dict_keys));
  }

  Dict dict(&scope, DictKeys::cast(*self).dict());
  return thread->runtime()->newInt(dict.numItems());
}

RawObject METH(dict_valueiterator, __iter__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isDictValueIterator()) {
    return thread->raiseRequiresType(self, ID(dict_valueiterator));
  }
  return *self;
}

RawObject METH(dict_valueiterator, __next__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isDictValueIterator()) {
    return thread->raiseRequiresType(self, ID(dict_valueiterator));
  }
  DictValueIterator iter(&scope, *self);
  Object value(&scope, dictValueIteratorNext(thread, iter));
  if (value.isError()) {
    return thread->raise(LayoutId::kStopIteration, NoneType::object());
  }
  return *value;
}

RawObject METH(dict_valueiterator, __length_hint__)(Thread* thread,
                                                    Arguments args) {
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isDictValueIterator()) {
    return thread->raiseRequiresType(self, ID(dict_valueiterator));
  }
  DictValueIterator iter(&scope, *self);
  Dict dict(&scope, iter.iterable());
  return SmallInt::fromWord(dict.numItems() - iter.numFound());
}

RawObject METH(dict_values, __iter__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isDictValues()) {
    return thread->raiseRequiresType(self, ID(dict_values));
  }

  Dict dict(&scope, DictValues::cast(*self).dict());
  return thread->runtime()->newDictValueIterator(thread, dict);
}

RawObject METH(dict_values, __len__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isDictValues()) {
    return thread->raiseRequiresType(self, ID(dict_values));
  }

  Dict dict(&scope, DictValues::cast(*self).dict());
  return thread->runtime()->newInt(dict.numItems());
}

}  // namespace py
