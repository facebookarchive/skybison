#include "dict-builtins.h"

#include "frame.h"
#include "globals.h"
#include "int-builtins.h"
#include "interpreter.h"
#include "objects.h"
#include "runtime.h"
#include "str-builtins.h"
#include "thread.h"
#include "type-builtins.h"

namespace py {

static bool dictLookup(Thread* thread, const Tuple& data, const Object& key,
                       word hash, word* index) {
  if (data.length() == 0) {
    *index = -1;
    return false;
  }
  word next_free_index = -1;
  uword perturb;
  word bucket_mask;
  RawSmallInt hash_int = SmallInt::fromWord(hash);
  for (word current = Dict::Bucket::bucket(*data, hash, &bucket_mask, &perturb);
       ; current = Dict::Bucket::nextBucket(current, bucket_mask, &perturb)) {
    word current_index = current * Dict::Bucket::kNumPointers;
    if (Dict::Bucket::hashRaw(*data, current_index) == hash_int) {
      RawObject eq = Runtime::objectEquals(
          thread, Dict::Bucket::key(*data, current_index), *key);
      if (eq == Bool::trueObj()) {
        *index = current_index;
        return true;
      }
      if (eq.isErrorException()) {
        UNIMPLEMENTED("exception in key comparison");
      }
      continue;
    }
    if (Dict::Bucket::isEmpty(*data, current_index)) {
      if (next_free_index == -1) {
        next_free_index = current_index;
      }
      *index = next_free_index;
      return false;
    }
    if (Dict::Bucket::isTombstone(*data, current_index)) {
      if (next_free_index == -1) {
        next_free_index = current_index;
      }
    }
  }
}

void dictAtPut(Thread* thread, const Dict& dict, const Object& key, word hash,
               const Object& value) {
  // TODO(T44245141): Move initialization of an empty dict to
  // dictEnsureCapacity.
  if (dict.capacity() == 0) {
    dict.setData(thread->runtime()->newMutableTuple(
        Runtime::kInitialDictCapacity * Dict::Bucket::kNumPointers));
    dict.resetNumUsableItems();
  }
  HandleScope scope(thread);
  Tuple data(&scope, dict.data());
  word index = -1;
  bool found = dictLookup(thread, data, key, hash, &index);
  DCHECK(index != -1, "invalid index %ld", index);
  if (found) {
    Dict::Bucket::setValue(*data, index, *value);
    return;
  }
  bool empty_slot = Dict::Bucket::isEmpty(*data, index);
  Dict::Bucket::set(*data, index, hash, *key, *value);
  dict.setNumItems(dict.numItems() + 1);
  if (empty_slot) {
    dict.decrementNumUsableItems();
    dictEnsureCapacity(thread, dict);
  }
  DCHECK(dict.hasUsableItems(), "dict must have an empty bucket left");
}

void dictAtPutByStr(Thread* thread, const Dict& dict, const Object& name,
                    const Object& value) {
  word hash = strHash(thread, *name);
  dictAtPut(thread, dict, name, hash, value);
}

void dictAtPutById(Thread* thread, const Dict& dict, SymbolId id,
                   const Object& value) {
  HandleScope scope(thread);
  Object name(&scope, thread->runtime()->symbols()->at(id));
  dictAtPutByStr(thread, dict, name, value);
}

RawObject dictAt(Thread* thread, const Dict& dict, const Object& key,
                 word hash) {
  HandleScope scope(thread);
  Tuple data(&scope, dict.data());
  word index = -1;
  bool found = dictLookup(thread, data, key, hash, &index);
  if (found) {
    return Dict::Bucket::value(*data, index);
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
  if (dict.capacity() == 0) {
    dict.setData(thread->runtime()->newMutableTuple(
        Runtime::kInitialDictCapacity * Dict::Bucket::kNumPointers));
    dict.resetNumUsableItems();
  }
  HandleScope scope(thread);
  Tuple data(&scope, dict.data());
  word index = -1;
  word hash = strHash(thread, *name);
  bool found = dictLookup(thread, data, name, hash, &index);
  DCHECK(index != -1, "invalid index %ld", index);
  if (found) {
    RawValueCell value_cell =
        ValueCell::cast(Dict::Bucket::value(*data, index));
    value_cell.setValue(*value);
    return value_cell;
  }
  ValueCell value_cell(&scope, thread->runtime()->newValueCell());
  bool empty_slot = Dict::Bucket::isEmpty(*data, index);
  Dict::Bucket::set(*data, index, hash, *name, *value_cell);
  dict.setNumItems(dict.numItems() + 1);
  if (empty_slot) {
    DCHECK(dict.numUsableItems() > 0, "dict.numIsableItems() must be positive");
    dict.decrementNumUsableItems();
    dictEnsureCapacity(thread, dict);
  }
  DCHECK(dict.hasUsableItems(), "dict must have an empty bucket left");
  value_cell.setValue(*value);
  return *value_cell;
}

void dictClear(Thread* thread, const Dict& dict) {
  if (dict.capacity() == 0) return;

  HandleScope scope(thread);
  dict.setNumItems(0);
  MutableTuple data(&scope, dict.data());
  data.fill(NoneType::object());
  dict.resetNumUsableItems();
}

bool dictIncludesByStr(Thread* thread, const Dict& dict, const Object& name) {
  word hash = strHash(thread, *name);
  return dictIncludes(thread, dict, name, hash);
}

bool dictIncludes(Thread* thread, const Dict& dict, const Object& key,
                  word hash) {
  HandleScope scope(thread);
  Tuple data(&scope, dict.data());
  word ignore;
  return dictLookup(thread, data, key, hash, &ignore);
}

RawObject dictRemoveByStr(Thread* thread, const Dict& dict,
                          const Object& name) {
  word hash = strHash(thread, *name);
  return dictRemove(thread, dict, name, hash);
}

RawObject dictRemove(Thread* thread, const Dict& dict, const Object& key,
                     word hash) {
  HandleScope scope(thread);
  Tuple data(&scope, dict.data());
  word index = -1;
  Object result(&scope, Error::notFound());
  bool found = dictLookup(thread, data, key, hash, &index);
  if (found) {
    result = Dict::Bucket::value(*data, index);
    Dict::Bucket::setTombstone(*data, index);
    dict.setNumItems(dict.numItems() - 1);
  }
  return *result;
}

// Insert `key`/`value` into dictionary assuming no bucket with an equivalent
// key and no tombstones exist.
static void dictInsertNoUpdate(const Tuple& data, const Object& key, word hash,
                               const Object& value) {
  DCHECK(data.length() > 0, "dict must not be empty");
  uword perturb;
  word bucket_mask;
  for (word current = Dict::Bucket::bucket(*data, hash, &bucket_mask, &perturb);
       ; current = Dict::Bucket::nextBucket(current, bucket_mask, &perturb)) {
    word current_index = current * Dict::Bucket::kNumPointers;
    if (Dict::Bucket::isEmpty(*data, current_index)) {
      Dict::Bucket::set(*data, current_index, hash, *key, *value);
      return;
    }
  }
}

void dictEnsureCapacity(Thread* thread, const Dict& dict) {
  // TODO(T44245141): Move initialization of an empty dict here.
  DCHECK(dict.capacity() > 0 && Utils::isPowerOfTwo(dict.capacity()),
         "dict capacity must be power of two, greater than zero");
  if (dict.hasUsableItems()) {
    return;
  }
  // TODO(T44247845): Handle overflow here.
  word new_capacity = dict.capacity() * Runtime::kDictGrowthFactor;
  HandleScope scope(thread);
  Tuple data(&scope, dict.data());
  MutableTuple new_data(&scope, thread->runtime()->newMutableTuple(
                                    new_capacity * Dict::Bucket::kNumPointers));
  // Re-insert items
  Object key(&scope, NoneType::object());
  Object value(&scope, NoneType::object());
  for (word i = Dict::Bucket::kFirst; Dict::Bucket::nextItem(*data, &i);) {
    key = Dict::Bucket::key(*data, i);
    word hash = Dict::Bucket::hash(*data, i);
    value = Dict::Bucket::value(*data, i);
    dictInsertNoUpdate(new_data, key, hash, value);
  }
  dict.setData(*new_data);
  dict.resetNumUsableItems();
}

RawObject dictKeys(Thread* thread, const Dict& dict) {
  word len = dict.numItems();
  Runtime* runtime = thread->runtime();
  if (len == 0) {
    return runtime->newList();
  }
  HandleScope scope(thread);
  Tuple data(&scope, dict.data());
  Tuple keys(&scope, runtime->newMutableTuple(len));
  word num_keys = 0;
  for (word i = Dict::Bucket::kFirst; Dict::Bucket::nextItem(*data, &i);) {
    DCHECK(num_keys < keys.length(), "%ld ! < %ld", num_keys, keys.length());
    keys.atPut(num_keys, Dict::Bucket::key(*data, i));
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
  Dict other(&scope, *mapping);
  Tuple other_data(&scope, other.data());
  for (word i = Dict::Bucket::kFirst;
       Dict::Bucket::nextItem(*other_data, &i);) {
    key = Dict::Bucket::key(*other_data, i);
    value = Dict::Bucket::value(*other_data, i);
    word hash = Dict::Bucket::hash(*other_data, i);
    if (do_override == Override::kOverride ||
        !dictIncludes(thread, dict, key, hash)) {
      dictAtPut(thread, dict, key, hash, value);
    } else if (do_override == Override::kError) {
      return thread->raise(LayoutId::kKeyError, *key);
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
    for (word i = 0; i < keys_list.numItems(); ++i) {
      key = keys_list.at(i);
      hash_obj = Interpreter::hash(thread, key);
      if (hash_obj.isErrorException()) return *hash_obj;
      word hash = SmallInt::cast(*hash_obj).value();
      if (do_override == Override::kOverride ||
          !dictIncludes(thread, dict, key, hash)) {
        value = Interpreter::callMethod2(thread, frame, subscr_method, mapping,
                                         key);
        if (value.isError()) return *value;
        dictAtPut(thread, dict, key, hash, value);
      } else if (do_override == Override::kError) {
        return thread->raise(LayoutId::kKeyError, *key);
      }
    }
    return NoneType::object();
  }

  if (keys.isTuple()) {
    Tuple keys_tuple(&scope, *keys);
    for (word i = 0; i < keys_tuple.length(); ++i) {
      key = keys_tuple.at(i);
      hash_obj = Interpreter::hash(thread, key);
      if (hash_obj.isErrorException()) return *hash_obj;
      word hash = SmallInt::cast(*hash_obj).value();
      if (do_override == Override::kOverride ||
          !dictIncludes(thread, dict, key, hash)) {
        value = Interpreter::callMethod2(thread, frame, subscr_method, mapping,
                                         key);
        if (value.isError()) return *value;
        dictAtPut(thread, dict, key, hash, value);
      } else if (do_override == Override::kError) {
        return thread->raise(LayoutId::kKeyError, *key);
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
    if (do_override == Override::kOverride ||
        !dictIncludes(thread, dict, key, hash)) {
      value =
          Interpreter::callMethod2(thread, frame, subscr_method, mapping, key);
      if (value.isError()) return *value;
      dictAtPut(thread, dict, key, hash, value);
    } else if (do_override == Override::kError) {
      return thread->raise(LayoutId::kKeyError, *key);
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
  Tuple buckets(&scope, dict.data());

  word i = iter.index();
  if (Dict::Bucket::nextItem(*buckets, &i)) {
    // At this point, we have found a valid index in the buckets.
    Object key(&scope, Dict::Bucket::key(*buckets, i));
    Object value(&scope, Dict::Bucket::value(*buckets, i));
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
  Tuple buckets(&scope, dict.data());

  word i = iter.index();
  if (Dict::Bucket::nextItem(*buckets, &i)) {
    // At this point, we have found a valid index in the buckets.
    iter.setIndex(i);
    iter.setNumFound(iter.numFound() + 1);
    return Dict::Bucket::key(*buckets, i);
  }

  // We hit the end.
  iter.setIndex(i);
  return Error::noMoreItems();
}

RawObject dictValueIteratorNext(Thread* thread, const DictValueIterator& iter) {
  HandleScope scope(thread);
  Dict dict(&scope, iter.iterable());
  Tuple buckets(&scope, dict.data());

  word i = iter.index();
  if (Dict::Bucket::nextItem(*buckets, &i)) {
    // At this point, we have found a valid index in the buckets.
    iter.setIndex(i);
    iter.setNumFound(iter.numFound() + 1);
    return Dict::Bucket::value(*buckets, i);
  }

  // We hit the end.
  iter.setIndex(i);
  return Error::noMoreItems();
}

const BuiltinAttribute DictBuiltins::kAttributes[] = {
    {ID(_dict__num_items), RawDict::kNumItemsOffset, AttributeFlags::kHidden},
    {ID(_dict__data), RawDict::kDataOffset, AttributeFlags::kHidden},
    {ID(_dict__num_usable_items), RawDict::kNumUsableItemsOffset,
     AttributeFlags::kHidden},
    {SymbolId::kSentinelId, -1},
};

// clang-format off
const BuiltinMethod DictBuiltins::kBuiltinMethods[] = {
    {ID(clear), METH(dict, clear)},
    {ID(__delitem__), METH(dict, __delitem__)},
    {ID(__eq__), METH(dict, __eq__)},
    {ID(__iter__), METH(dict, __iter__)},
    {ID(__len__), METH(dict, __len__)},
    {ID(__new__), METH(dict, __new__)},
    {ID(items), METH(dict, items)},
    {ID(keys), METH(dict, keys)},
    {ID(values), METH(dict, values)},
    {SymbolId::kSentinelId, nullptr},
};
// clang-format on

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
  Tuple self_data(&scope, self.data());
  Object key(&scope, NoneType::object());
  Object left_value(&scope, NoneType::object());
  Object right_value(&scope, NoneType::object());
  Object cmp_result(&scope, NoneType::object());
  Object cmp_result_bool(&scope, NoneType::object());
  for (word i = Dict::Bucket::kFirst; Dict::Bucket::nextItem(*self_data, &i);) {
    key = Dict::Bucket::key(*self_data, i);
    word hash = Dict::Bucket::hash(*self_data, i);
    right_value = dictAt(thread, other, key, hash);
    if (right_value.isErrorNotFound()) {
      return Bool::falseObj();
    }

    left_value = Dict::Bucket::value(*self_data, i);
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
  result.resetNumUsableItems();
  return *result;
}

// TODO(T35787656): Instead of re-writing everything for every class, make a
// helper function that takes a member function (type check) and string for the
// Python symbol name
const BuiltinMethod DictItemIteratorBuiltins::kBuiltinMethods[] = {
    {ID(__iter__), METH(dict_itemiterator, __iter__)},
    {ID(__length_hint__), METH(dict_itemiterator, __length_hint__)},
    {ID(__next__), METH(dict_itemiterator, __next__)},
    {SymbolId::kSentinelId, nullptr},
};

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

const BuiltinMethod DictItemsBuiltins::kBuiltinMethods[] = {
    {ID(__iter__), METH(dict_items, __iter__)},
    {SymbolId::kSentinelId, nullptr},
};

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

const BuiltinMethod DictKeyIteratorBuiltins::kBuiltinMethods[] = {
    {ID(__iter__), METH(dict_keyiterator, __iter__)},
    {ID(__length_hint__), METH(dict_keyiterator, __length_hint__)},
    {ID(__next__), METH(dict_keyiterator, __next__)},
    {SymbolId::kSentinelId, nullptr},
};

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

const BuiltinMethod DictKeysBuiltins::kBuiltinMethods[] = {
    {ID(__iter__), METH(dict_keys, __iter__)},
    {SymbolId::kSentinelId, nullptr},
};

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

const BuiltinMethod DictValueIteratorBuiltins::kBuiltinMethods[] = {
    {ID(__iter__), METH(dict_valueiterator, __iter__)},
    {ID(__length_hint__), METH(dict_valueiterator, __length_hint__)},
    {ID(__next__), METH(dict_valueiterator, __next__)},
    {SymbolId::kSentinelId, nullptr},
};

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

const BuiltinMethod DictValuesBuiltins::kBuiltinMethods[] = {
    {ID(__iter__), METH(dict_values, __iter__)},
    {SymbolId::kSentinelId, nullptr},
};

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

}  // namespace py
