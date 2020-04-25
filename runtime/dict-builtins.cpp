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

static word probeBegin(RawTuple indices, word hash, word* indices_mask,
                       uword* perturb) {
  const word nindices = indices.length();
  DCHECK(Utils::isPowerOfTwo(nindices), "%ld is not a power of 2", nindices);
  DCHECK(nindices > 0, "indicesxs size <= 0");
  DCHECK(RawSmallInt::isValid(hash), "hash out of range");
  *perturb = static_cast<uword>(hash);
  *indices_mask = nindices - 1;
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

// Return the item index stored at indices[index].
static word itemIndexAt(RawTuple indices, word index) {
  return SmallInt::cast(indices.at(index)).value();
}

// Set `item_index` at indices[index].
static void itemIndexAtPut(RawTuple indices, word index, word item_index) {
  indices.atPut(index, SmallInt::fromWord(item_index));
}

// Set a tombstone at indices[index] given `data_length` from
// data.length().
static void indicesSetTombstone(RawTuple indices, word index) {
  DCHECK(indices.at(index).isSmallInt(), "unexpected call");
  indices.atPut(index, Unbound::object());
}

// Return true if the indices[index] is filled with an active item.
static bool indicesIsFull(RawTuple indices, word index) {
  return indices.at(index).isSmallInt();
}

// Return true if the indices[index] is never used.
static bool indicesIsEmpty(RawTuple indices, word index) {
  return indices.at(index).isNoneType();
}

// Helper functions for accessing dict items from dict.data().

// Data tuple Layout.
static const word kItemHashOffset = 0;
static const word kItemKeyOffset = 1;
static const word kItemValueOffset = 2;
static const word kItemNumPointers = 3;

static RawObject itemKey(RawTuple data, word index) {
  return data.at(index + kItemKeyOffset);
}

static RawObject itemValue(RawTuple data, word index) {
  return data.at(index + kItemValueOffset);
}

static word itemHash(RawTuple data, word index) {
  return SmallInt::cast(data.at(index + kItemHashOffset)).value();
}

static RawObject itemHashRaw(RawTuple data, word index) {
  return data.at(index + kItemHashOffset);
}

static void itemSet(RawTuple data, word index, word hash, RawObject key,
                    RawObject value) {
  data.atPut(index + kItemHashOffset, SmallInt::fromWordTruncated(hash));
  data.atPut(index + kItemKeyOffset, key);
  data.atPut(index + kItemValueOffset, value);
}

static void itemSetTombstone(RawTuple data, word index) {
  data.atPut(index + kItemHashOffset, Unbound::object());
  data.atPut(index + kItemKeyOffset, NoneType::object());
  data.atPut(index + kItemValueOffset, NoneType::object());
}

static void itemSetValue(RawTuple data, word index, RawObject value) {
  data.atPut(index + kItemValueOffset, value);
}

static bool itemIsEmpty(RawTuple data, word index) {
  return data.at(index + kItemHashOffset).isNoneType();
}

static bool itemIsFull(RawTuple data, word index) {
  return data.at(index + kItemHashOffset).isSmallInt();
}

static bool itemIsTombstone(RawTuple data, word index) {
  return data.at(index + kItemHashOffset).isUnbound();
}

}  // namespace

// Returns one of the three possible values:
// - `key` was found at indices[index] : SmallInt::fromWord(index)
// - `key` was not found, but insertion can be done to indices[index] :
//   SmallInt::fromWord(index - data.length())
// - Exception that was raised from key comparison __eq__ function.
static RawObject dictLookup(Thread* thread, const MutableTuple& data,
                            MutableTuple& indices, const Object& key,
                            word hash) {
  DCHECK(data.length() > 0, "data shouldn't be empty");
  word next_free_index = -1;
  uword perturb;
  word indices_mask;
  RawSmallInt hash_int = SmallInt::fromWord(hash);
  for (word current_index = probeBegin(*indices, hash, &indices_mask, &perturb);
       ; current_index = probeNext(current_index, indices_mask, &perturb)) {
    if (indicesIsFull(*indices, current_index)) {
      word item_index = itemIndexAt(*indices, current_index);
      if (itemHashRaw(*data, item_index) == hash_int) {
        RawObject eq =
            Runtime::objectEquals(thread, itemKey(*data, item_index), *key);
        if (eq == Bool::trueObj()) {
          return SmallInt::fromWord(current_index);
        }
        if (UNLIKELY(eq.isErrorException())) {
          return eq;
        }
      }
      continue;
    }
    if (next_free_index == -1) {
      next_free_index = current_index;
    }
    if (indicesIsEmpty(*indices, current_index)) {
      return SmallInt::fromWord(next_free_index - indices.length());
    }
  }
  UNREACHABLE("Expected to have found an empty index");
}

static word nextItemIndex(RawTuple data, word* index, word end) {
  for (word i = *index; i < end; i += kItemNumPointers) {
    if (!itemIsFull(data, i)) continue;
    *index = i + kItemNumPointers;
    return i;
  }
  return -1;
}

bool dictNextItem(const Dict& dict, word* index, Object* key_out,
                  Object* value_out) {
  RawTuple data = RawTuple::cast(dict.data());
  word next_item_index = nextItemIndex(data, index, dict.firstEmptyItemIndex());
  if (next_item_index < 0) {
    return false;
  }
  *key_out = itemKey(data, next_item_index);
  *value_out = itemValue(data, next_item_index);
  return true;
}

bool dictNextItemHash(const Dict& dict, word* index, Object* key_out,
                      Object* value_out, word* hash_out) {
  RawTuple data = RawTuple::cast(dict.data());
  word next_item_index = nextItemIndex(data, index, dict.firstEmptyItemIndex());
  if (next_item_index < 0) {
    return false;
  }
  *key_out = itemKey(data, next_item_index);
  *value_out = itemValue(data, next_item_index);
  *hash_out = itemHash(data, next_item_index);
  return true;
}

bool dictNextKey(const Dict& dict, word* index, Object* key_out) {
  RawTuple data = RawTuple::cast(dict.data());
  word next_item_index = nextItemIndex(data, index, dict.firstEmptyItemIndex());
  if (next_item_index < 0) {
    return false;
  }
  *key_out = itemKey(data, next_item_index);
  return true;
}

bool dictNextKeyHash(const Dict& dict, word* index, Object* key_out,
                     word* hash_out) {
  RawTuple data = RawTuple::cast(dict.data());
  word next_item_index = nextItemIndex(data, index, dict.firstEmptyItemIndex());
  if (next_item_index < 0) {
    return false;
  }
  *key_out = itemKey(data, next_item_index);
  *hash_out = itemHash(data, next_item_index);
  return true;
}

bool dictNextValue(const Dict& dict, word* index, Object* value_out) {
  RawTuple data = RawTuple::cast(dict.data());
  word next_item_index = nextItemIndex(data, index, dict.firstEmptyItemIndex());
  if (next_item_index < 0) {
    return false;
  }
  *value_out = itemValue(data, next_item_index);
  return true;
}

static word sizeOfDataTuple(word indices_len) { return (indices_len * 2) / 3; }

static const word kDictGrowthFactor = 2;
// Initial size of the dict. According to comments in CPython's
// dictobject.c this accommodates the majority of dictionaries without needing
// a resize (obviously this depends on the load factor used to resize the
// dict).
static const word kInitialDictIndicesLength = 8;

void dictAllocateArrays(Thread* thread, const Dict& dict, word indices_len) {
  indices_len = Utils::maximum(indices_len, kInitialDictIndicesLength);
  DCHECK(Utils::isPowerOfTwo(indices_len), "%ld is not a power of 2",
         indices_len);
  Runtime* runtime = thread->runtime();
  dict.setData(runtime->newMutableTuple(sizeOfDataTuple(indices_len) *
                                        kItemNumPointers));
  dict.setIndices(runtime->newMutableTuple(indices_len));
  dict.setFirstEmptyItemIndex(0);
}

// Return true if `dict` has an available item for insertion.
static bool dictHasUsableItem(const Dict& dict) {
  return dict.firstEmptyItemIndex() < RawTuple::cast(dict.data()).length();
}

// Insert `key`/`value` into dictionary assuming no item with an equivalent
// key and no tombstones exist.
static void dictInsertNoUpdate(const Tuple& data, const Tuple& indices,
                               word item_count, const Object& key, word hash,
                               const Object& value) {
  DCHECK(data.length() > 0, "dict must not be empty");
  uword perturb;
  word indices_mask;
  for (word current_index = probeBegin(*indices, hash, &indices_mask, &perturb);
       ; current_index = probeNext(current_index, indices_mask, &perturb)) {
    if (indicesIsEmpty(*indices, current_index)) {
      word item_index = item_count * kItemNumPointers;
      itemSet(*data, item_index, hash, *key, *value);
      itemIndexAtPut(*indices, current_index, item_index);
      return;
    }
  }
}

static void dictEnsureCapacity(Thread* thread, const Dict& dict) {
  // TODO(T44245141): Move initialization of an empty dict here.
  DCHECK(dict.indicesLength() > 0 && Utils::isPowerOfTwo(dict.indicesLength()),
         "dict capacity must be power of two, greater than zero");
  if (dictHasUsableItem(dict)) {
    return;
  }

  // TODO(T44247845): Handle overflow here.
  word new_indices_len = dict.indicesLength() * kDictGrowthFactor;
  HandleScope scope(thread);
  MutableTuple new_data(
      &scope, thread->runtime()->newMutableTuple(
                  sizeOfDataTuple(new_indices_len) * kItemNumPointers));
  MutableTuple new_indices(&scope,
                           thread->runtime()->newMutableTuple(new_indices_len));
  // Re-insert items
  Object key(&scope, NoneType::object());
  Object value(&scope, NoneType::object());
  word hash;
  word item_count = 0;
  for (word i = 0; dictNextItemHash(dict, &i, &key, &value, &hash);
       item_count++) {
    dictInsertNoUpdate(new_data, new_indices, item_count, key, hash, value);
  }
  DCHECK(item_count == dict.numItems(), "found entries != dictnNumItems()");
  dict.setData(*new_data);
  dict.setIndices(*new_indices);
  dict.setFirstEmptyItemIndex(dict.numItems() * kItemNumPointers);
}

RawObject dictAtPut(Thread* thread, const Dict& dict, const Object& key,
                    word hash, const Object& value) {
  // TODO(T44245141): Move initialization of an empty dict to
  // dictEnsureCapacity.
  if (dict.indicesLength() == 0) {
    dictAllocateArrays(thread, dict, kInitialDictIndicesLength);
  }
  HandleScope scope(thread);
  MutableTuple data(&scope, dict.data());
  MutableTuple indices(&scope, dict.indices());
  RawObject lookup_result = dictLookup(thread, data, indices, key, hash);
  if (UNLIKELY(lookup_result.isErrorException())) {
    return lookup_result;
  }
  word index = SmallInt::cast(lookup_result).value();
  if (index >= 0) {
    word item_index = SmallInt::cast(indices.at(index)).value();
    itemSetValue(*data, item_index, *value);
    return NoneType::object();
  }
  index += indices.length();
  word item_index = dict.firstEmptyItemIndex();
  DCHECK(itemIsEmpty(*data, item_index), "item is expected to be empty");
  itemSet(*data, item_index, hash, *key, *value);
  itemIndexAtPut(*indices, index, item_index);
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
  MutableTuple indices(&scope, dict.indices());
  RawObject lookup_result = dictLookup(thread, data, indices, key, hash);
  if (UNLIKELY(lookup_result.isErrorException())) {
    return lookup_result;
  }
  word index = SmallInt::cast(lookup_result).value();
  if (index >= 0) {
    word item_index = itemIndexAt(*indices, index);
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
  // TODO(T44245141): Move initialization of an empty dict to
  // dictEnsureCapacity.
  if (dict.indicesLength() == 0) {
    dictAllocateArrays(thread, dict, kInitialDictIndicesLength);
  }
  HandleScope scope(thread);
  MutableTuple data(&scope, dict.data());
  MutableTuple indices(&scope, dict.indices());
  word hash = strHash(thread, *name);
  RawObject lookup_result = dictLookup(thread, data, indices, name, hash);
  if (UNLIKELY(lookup_result.isErrorException())) {
    return lookup_result;
  }
  word index = SmallInt::cast(lookup_result).value();
  if (index >= 0) {
    word item_index = itemIndexAt(*indices, index);
    RawValueCell value_cell = ValueCell::cast(itemValue(*data, item_index));
    value_cell.setValue(*value);
    return value_cell;
  }
  index += indices.length();
  word item_index = dict.firstEmptyItemIndex();
  DCHECK(itemIsEmpty(*data, item_index), "item is expected to be empty");
  ValueCell value_cell(&scope, thread->runtime()->newValueCell());
  itemSet(*data, item_index, hash, *name, *value_cell);
  itemIndexAtPut(*indices, index, item_index);
  dict.setNumItems(dict.numItems() + 1);
  dict.setFirstEmptyItemIndex(dict.firstEmptyItemIndex() + kItemNumPointers);
  dictEnsureCapacity(thread, dict);
  DCHECK(dictHasUsableItem(dict), "dict must have an empty item left");
  value_cell.setValue(*value);
  return *value_cell;
}

void dictClear(Thread* thread, const Dict& dict) {
  if (dict.indicesLength() == 0) return;

  HandleScope scope(thread);
  MutableTuple data(&scope, dict.data());
  data.fill(NoneType::object());
  MutableTuple indices(&scope, dict.indices());
  indices.fill(NoneType::object());
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
  MutableTuple indices(&scope, dict.indices());
  RawObject lookup_result = dictLookup(thread, data, indices, key, hash);
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
  MutableTuple indices(&scope, dict.indices());
  RawObject lookup_result = dictLookup(thread, data, indices, key, hash);
  if (UNLIKELY(lookup_result.isErrorException())) {
    return lookup_result;
  }
  word index = SmallInt::cast(lookup_result).value();
  if (index < 0) {
    return Error::notFound();
  }
  word item_index = itemIndexAt(*indices, index);
  Object result(&scope, itemValue(*data, item_index));
  itemSetTombstone(*data, item_index);
  indicesSetTombstone(*indices, index);
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
  Tuple keys(&scope, runtime->newMutableTuple(len));
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

  HandleScope scope;
  Object key(&scope, NoneType::object());
  Object hash_obj(&scope, NoneType::object());
  Object value(&scope, NoneType::object());
  Object included(&scope, NoneType::object());
  Frame* frame = thread->currentFrame();
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
  Object keys(&scope,
              Interpreter::callMethod1(thread, frame, keys_method, mapping));
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
        value = Interpreter::callMethod2(thread, frame, subscr_method, mapping,
                                         key);
        if (value.isError()) return *value;
        dict_result = dictAtPut(thread, dict, key, hash, value);
        if (dict_result.isErrorException()) return *dict_result;
      } else {
        included = dictIncludes(thread, dict, key, hash);
        if (included.isErrorException()) return *included;
        if (included == Bool::falseObj()) {
          value = Interpreter::callMethod2(thread, frame, subscr_method,
                                           mapping, key);
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
        value = Interpreter::callMethod2(thread, frame, subscr_method, mapping,
                                         key);
        if (value.isError()) return *value;
        dict_result = dictAtPut(thread, dict, key, hash, value);
        if (dict_result.isErrorException()) return *dict_result;
      } else {
        included = dictIncludes(thread, dict, key, hash);
        if (included.isErrorException()) return *included;
        if (included == Bool::falseObj()) {
          value = Interpreter::callMethod2(thread, frame, subscr_method,
                                           mapping, key);
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
  Object iter_method(
      &scope, Interpreter::lookupMethod(thread, thread->currentFrame(), keys,
                                        ID(__iter__)));
  if (iter_method.isError()) {
    return thread->raiseWithFmt(LayoutId::kTypeError, "keys() is not iterable");
  }

  Object iterator(&scope,
                  Interpreter::callMethod1(thread, thread->currentFrame(),
                                           iter_method, keys));
  if (iterator.isError()) {
    return thread->raiseWithFmt(LayoutId::kTypeError, "keys() is not iterable");
  }
  Object next_method(&scope,
                     Interpreter::lookupMethod(thread, thread->currentFrame(),
                                               iterator, ID(__next__)));
  if (next_method.isError()) {
    return thread->raiseWithFmt(LayoutId::kTypeError, "keys() is not iterable");
  }
  Object dict_result(&scope, NoneType::object());
  for (;;) {
    key = Interpreter::callMethod1(thread, thread->currentFrame(), next_method,
                                   iterator);
    if (key.isError()) {
      if (thread->clearPendingStopIteration()) break;
      return *key;
    }
    hash_obj = Interpreter::hash(thread, key);
    if (hash_obj.isErrorException()) return *hash_obj;
    word hash = SmallInt::cast(*hash_obj).value();
    if (do_override == Override::kOverride) {
      value =
          Interpreter::callMethod2(thread, frame, subscr_method, mapping, key);
      if (value.isError()) return *value;
      dict_result = dictAtPut(thread, dict, key, hash, value);
      if (dict_result.isErrorException()) return *dict_result;
    } else {
      included = dictIncludes(thread, dict, key, hash);
      if (included.isErrorException()) return *included;
      if (included == Bool::falseObj()) {
        value = Interpreter::callMethod2(thread, frame, subscr_method, mapping,
                                         key);
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

RawObject dictItemIteratorNext(Thread* thread, const DictItemIterator& iter) {
  HandleScope scope(thread);
  Dict dict(&scope, iter.iterable());
  Object key(&scope, NoneType::object());
  Object value(&scope, NoneType::object());
  word i = iter.index();
  if (dictNextItem(dict, &i, &key, &value)) {
    Tuple kv_pair(&scope, thread->runtime()->newTuple(2));
    kv_pair.atPut(0, *key);
    kv_pair.atPut(1, *value);
    iter.setIndex(i);
    iter.setNumFound(iter.numFound() + 1);
    return *kv_pair;
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

const BuiltinAttribute DictBuiltins::kAttributes[] = {
    {ID(_dict__num_items), RawDict::kNumItemsOffset, AttributeFlags::kHidden},
    {ID(_dict__data), RawDict::kDataOffset, AttributeFlags::kHidden},
    {ID(_dict__sparse), RawDict::kIndicesOffset, AttributeFlags::kHidden},
    {ID(_dict__first_empty_item_index), RawDict::kFirstEmptyItemIndexOffset,
     AttributeFlags::kHidden},
    {SymbolId::kSentinelId, -1},
};

RawObject METH(dict, clear)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfDict(*self)) {
    return thread->raiseRequiresType(self, ID(dict));
  }
  Dict dict(&scope, *self);
  dictClear(thread, dict);
  return NoneType::object();
}

RawObject METH(dict, __delitem__)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
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

RawObject METH(dict, __eq__)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
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
  Dict self(&scope, *self_obj);
  Dict other(&scope, *other_obj);
  if (self.numItems() != other.numItems()) {
    return Bool::falseObj();
  }
  Object key(&scope, NoneType::object());
  Object left_value(&scope, NoneType::object());
  word hash;
  Object right_value(&scope, NoneType::object());
  Object cmp_result(&scope, NoneType::object());
  Object cmp_result_bool(&scope, NoneType::object());
  for (word i = 0; dictNextItemHash(self, &i, &key, &left_value, &hash);) {
    right_value = dictAt(thread, other, key, hash);
    if (right_value.isErrorNotFound()) {
      return Bool::falseObj();
    }
    if (left_value == right_value) {
      continue;
    }
    cmp_result = Interpreter::compareOperation(thread, frame, EQ, left_value,
                                               right_value);
    if (cmp_result.isErrorException()) {
      return *cmp_result;
    }
    cmp_result_bool = Interpreter::isTrue(thread, *cmp_result);
    if (cmp_result_bool.isErrorException()) return *cmp_result_bool;
    if (cmp_result_bool == Bool::falseObj()) {
      return Bool::falseObj();
    }
  }
  return Bool::trueObj();
}

RawObject METH(dict, __len__)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfDict(*self)) {
    return thread->raiseRequiresType(self, ID(dict));
  }
  Dict dict(&scope, *self);
  return SmallInt::fromWord(dict.numItems());
}

RawObject METH(dict, __iter__)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
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

RawObject METH(dict, items)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfDict(*self)) {
    return thread->raiseRequiresType(self, ID(dict));
  }
  Dict dict(&scope, *self);
  return runtime->newDictItems(thread, dict);
}

RawObject METH(dict, keys)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfDict(*self)) {
    return thread->raiseRequiresType(self, ID(dict));
  }
  Dict dict(&scope, *self);
  return runtime->newDictKeys(thread, dict);
}

RawObject METH(dict, popitem)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
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
  Tuple data(&scope, dict.data());
  for (word item_index = dict.firstEmptyItemIndex() - kItemNumPointers;
       item_index >= 0; item_index -= kItemNumPointers) {
    if (!itemIsEmpty(*data, item_index) &&
        !itemIsTombstone(*data, item_index)) {
      Tuple result(&scope, thread->runtime()->newTuple(2));
      result.atPut(0, itemKey(*data, item_index));
      result.atPut(1, itemValue(*data, item_index));
      // Find the item for the entry and set a tombstone in it.
      // Note that this would take amortized cost O(1).
      word indices_index = -1;
      Tuple indices(&scope, dict.indices());
      uword perturb;
      word indices_mask;
      word hash = itemHash(*data, item_index);
      for (word current_index =
               probeBegin(*indices, hash, &indices_mask, &perturb);
           ; current_index = probeNext(current_index, indices_mask, &perturb)) {
        if (indicesIsFull(*indices, current_index) &&
            itemIndexAt(*indices, current_index) == item_index) {
          indices_index = current_index;
          break;
        }
      }
      DCHECK(indices_index >= 0, "cannot find index for entry in dict.sparse");
      itemSetTombstone(*data, item_index);
      indicesSetTombstone(*indices, indices_index);
      dict.setNumItems(dict.numItems() - 1);
      return *result;
    }
  }
  UNREACHABLE("dict.numItems() > 0, but couldn't find any active item");
}

RawObject METH(dict, values)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfDict(*self)) {
    return thread->raiseRequiresType(self, ID(dict));
  }
  Dict dict(&scope, *self);
  return runtime->newDictValues(thread, dict);
}

RawObject METH(dict, __new__)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
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
  result.setData(runtime->emptyTuple());
  result.setIndices(runtime->emptyTuple());
  result.setFirstEmptyItemIndex(0);
  return *result;
}

// TODO(T35787656): Instead of re-writing everything for every class, make a
// helper function that takes a member function (type check) and string for the
// Python symbol name

RawObject METH(dict_itemiterator, __iter__)(Thread* thread, Frame* frame,
                                            word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isDictItemIterator()) {
    return thread->raiseRequiresType(self, ID(dict_itemiterator));
  }
  return *self;
}

RawObject METH(dict_itemiterator, __next__)(Thread* thread, Frame* frame,
                                            word nargs) {
  Arguments args(frame, nargs);
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

RawObject METH(dict_itemiterator, __length_hint__)(Thread* thread, Frame* frame,
                                                   word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isDictItemIterator()) {
    return thread->raiseRequiresType(self, ID(dict_itemiterator));
  }
  DictItemIterator iter(&scope, *self);
  Dict dict(&scope, iter.iterable());
  return SmallInt::fromWord(dict.numItems() - iter.numFound());
}

RawObject METH(dict_items, __iter__)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isDictItems()) {
    return thread->raiseRequiresType(self, ID(dict_items));
  }

  Dict dict(&scope, DictItems::cast(*self).dict());
  return thread->runtime()->newDictItemIterator(thread, dict);
}

RawObject METH(dict_items, __len__)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isDictItems()) {
    return thread->raiseRequiresType(self, ID(dict_items));
  }

  Dict dict(&scope, DictItems::cast(*self).dict());
  return thread->runtime()->newInt(dict.numItems());
}

RawObject METH(dict_keyiterator, __iter__)(Thread* thread, Frame* frame,
                                           word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isDictKeyIterator()) {
    return thread->raiseRequiresType(self, ID(dict_keyiterator));
  }
  return *self;
}

RawObject METH(dict_keyiterator, __next__)(Thread* thread, Frame* frame,
                                           word nargs) {
  Arguments args(frame, nargs);
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

RawObject METH(dict_keyiterator, __length_hint__)(Thread* thread, Frame* frame,
                                                  word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isDictKeyIterator()) {
    return thread->raiseRequiresType(self, ID(dict_keyiterator));
  }
  DictKeyIterator iter(&scope, *self);
  Dict dict(&scope, iter.iterable());
  return SmallInt::fromWord(dict.numItems() - iter.numFound());
}

RawObject METH(dict_keys, __iter__)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isDictKeys()) {
    return thread->raiseRequiresType(self, ID(dict_keys));
  }

  Dict dict(&scope, DictKeys::cast(*self).dict());
  return thread->runtime()->newDictKeyIterator(thread, dict);
}

RawObject METH(dict_keys, __len__)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isDictKeys()) {
    return thread->raiseRequiresType(self, ID(dict_keys));
  }

  Dict dict(&scope, DictKeys::cast(*self).dict());
  return thread->runtime()->newInt(dict.numItems());
}

RawObject METH(dict_valueiterator, __iter__)(Thread* thread, Frame* frame,
                                             word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isDictValueIterator()) {
    return thread->raiseRequiresType(self, ID(dict_valueiterator));
  }
  return *self;
}

RawObject METH(dict_valueiterator, __next__)(Thread* thread, Frame* frame,
                                             word nargs) {
  Arguments args(frame, nargs);
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
                                                    Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isDictValueIterator()) {
    return thread->raiseRequiresType(self, ID(dict_valueiterator));
  }
  DictValueIterator iter(&scope, *self);
  Dict dict(&scope, iter.iterable());
  return SmallInt::fromWord(dict.numItems() - iter.numFound());
}

RawObject METH(dict_values, __iter__)(Thread* thread, Frame* frame,
                                      word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isDictValues()) {
    return thread->raiseRequiresType(self, ID(dict_values));
  }

  Dict dict(&scope, DictValues::cast(*self).dict());
  return thread->runtime()->newDictValueIterator(thread, dict);
}

RawObject METH(dict_values, __len__)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isDictValues()) {
    return thread->raiseRequiresType(self, ID(dict_values));
  }

  Dict dict(&scope, DictValues::cast(*self).dict());
  return thread->runtime()->newInt(dict.numItems());
}

}  // namespace py
