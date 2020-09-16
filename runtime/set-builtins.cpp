#include "set-builtins.h"

#include "builtins.h"
#include "dict-builtins.h"
#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"
#include "type-builtins.h"

namespace py {
// Data tuple layout
static const word kHashOffset = 0;
static const word kValueOffset = kHashOffset + 1;
static const word kNumPointers = kValueOffset + 1;

static word getIndex(RawTuple data, word hash) {
  DCHECK(SmallInt::isValid(hash), "hash out of range");
  word nitems = data.length() / kNumPointers;
  DCHECK(Utils::isPowerOfTwo(nitems), "%ld not a power of 2", nitems);
  return (hash & (nitems - 1)) * kNumPointers;
}

static bool isEmpty(RawTuple data, word index) {
  return data.at(index + kHashOffset).isNoneType();
}

static bool isFull(RawTuple data, word index) {
  return data.at(index + kHashOffset).isSmallInt();
}

static bool isTombstone(RawTuple data, word index) {
  return data.at(index + kHashOffset).isUnbound();
}

static word itemHash(RawTuple data, word index) {
  return SmallInt::cast(data.at(index + kHashOffset)).value();
}

static RawObject itemValue(RawTuple data, word index) {
  return data.at(index + kValueOffset);
}

static void itemAtPut(RawTuple data, word index, word hash, RawObject value) {
  data.atPut(index + kHashOffset, SmallInt::fromWordTruncated(hash));
  data.atPut(index + kValueOffset, value);
}

static void itemAtPutTombstone(RawTuple data, word index) {
  data.atPut(index + kHashOffset, Unbound::object());
  data.atPut(index + kValueOffset, NoneType::object());
}

static word nextItemIndex(RawTuple data, word* index) {
  for (word i = *index; i < data.length(); i += kNumPointers) {
    if (!isFull(data, i)) continue;
    *index = i + kNumPointers;
    return i;
  }
  return -1;
}

bool setNextItem(const SetBase& set, word* index, RawObject* value_out) {
  RawTuple data = RawTuple::cast(set.data());
  word next_item_index = nextItemIndex(data, index);
  if (next_item_index < 0) {
    return false;
  }
  *value_out = itemValue(data, next_item_index);
  return true;
}

bool setNextItemHash(const SetBase& set, word* index, RawObject* value_out,
                     word* hash_out) {
  DCHECK(hash_out != nullptr, "hash_out must not be null");
  RawTuple data = RawTuple::cast(set.data());
  word next_item_index = nextItemIndex(data, index);
  if (next_item_index < 0) {
    return false;
  }
  *value_out = itemValue(data, next_item_index);
  *hash_out = itemHash(data, next_item_index);

  return true;
}

static word entryMatches(Thread* thread, RawTuple data, word entry,
                         word hash_value, const Object& key) {
  DCHECK(isFull(data, entry), "entry cannot be a tombstone");
  if (itemHash(data, entry) == hash_value) {
    RawObject start_key = itemValue(data, entry);
    RawObject eq = Runtime::objectEquals(thread, start_key, *key);
    if (eq == Bool::trueObj()) {
      return entry;
    }
    if (eq.isErrorException()) {
      UNIMPLEMENTED("exception in value comparison");
    }
    return -1;
  }
  return -1;
}

static const word kNumLinearProbes = 9;
static const word kPerturbShift = 5;

static word setLookup(Thread* thread, const Tuple& data, const Object& key,
                      word hash_value) {
  word length = data.length();
  if (length == 0) {
    return -1;
  }
  uword perturb = static_cast<uword>(hash_value);
  word mask = length - 1;
  word i = hash_value & mask;
  word entry = getIndex(*data, i);

  if (isEmpty(*data, entry)) {
    return -1;
  }

  for (;;) {
    if (isFull(*data, entry)) {
      word match = entryMatches(thread, *data, entry, hash_value, key);
      if (match != -1) {
        return match;
      }
    }
    if (entry + kNumLinearProbes * kNumPointers <= mask) {
      for (int j = 1; j <= kNumLinearProbes; j++) {
        entry += kNumPointers;
        if (isEmpty(*data, entry)) {
          return -1;
        }
        if (!isTombstone(*data, entry)) {
          word match = entryMatches(thread, *data, entry, hash_value, key);
          if (match != -1) {
            return match;
          }
        }
      }
    }
    perturb >>= kPerturbShift;
    i = (i * 5 + 1 + perturb) & mask;
    entry = getIndex(*data, i);
    if (isEmpty(*data, entry)) {
      return -1;
    }
  }
}

static word setLookupForInsertion(Thread* thread, const Tuple& data,
                                  const Object& key, word hash_value) {
  word length = data.length();
  if (length == 0) {
    return -1;
  }
  uword perturb = static_cast<uword>(hash_value);
  word mask = length - 1;
  word i = hash_value & mask;
  word entry = getIndex(*data, i);

  if (isEmpty(*data, entry)) {
    return entry;
  }

  for (word freeslot = -1;;) {
    if (isFull(*data, entry)) {
      word match = entryMatches(thread, *data, entry, hash_value, key);
      if (match != -1) {
        return match;
      }
    } else if (isTombstone(*data, entry)) {
      freeslot = entry;
    }
    if (entry + kNumLinearProbes * kNumPointers <= mask) {
      for (int j = 1; j <= kNumLinearProbes; j++) {
        entry += kNumPointers;
        if (isEmpty(*data, entry)) {
          return entry;
        }
        if (!isTombstone(*data, entry)) {
          word match = entryMatches(thread, *data, entry, hash_value, key);
          if (match != -1) {
            return match;
          }
        } else if (isTombstone(*data, entry)) {
          freeslot = entry;
        }
      }
    }
    perturb >>= kPerturbShift;
    i = (i * 5 + 1 + perturb) & mask;
    entry = getIndex(*data, i);
    if (isEmpty(*data, entry)) {
      return (freeslot == -1) ? entry : freeslot;
    }
  }
}

static RawTuple setGrow(Thread* thread, const SetBase& set) {
  HandleScope scope(thread);
  Tuple data(&scope, set.data());
  word new_length = data.length() * Runtime::kSetGrowthFactor;
  if (new_length == 0) {
    new_length = Runtime::kInitialSetCapacity * kNumPointers;
  }
  MutableTuple new_data(&scope, thread->runtime()->newMutableTuple(new_length));
  // Re-insert items
  Object value(&scope, NoneType::object());
  for (word i = 0, hash; setNextItemHash(set, &i, &value, &hash);) {
    word index = setLookupForInsertion(thread, new_data, value, hash);
    DCHECK(index != -1, "unexpected index %ld", index);
    itemAtPut(*new_data, index, hash, *value);
  }
  set.setNumFilled(set.numItems());
  return *new_data;
}

RawObject setAdd(Thread* thread, const SetBase& set, const Object& value,
                 word hash) {
  HandleScope scope(thread);
  Tuple data(&scope, set.data());
  word index = setLookup(thread, data, value, hash);
  if (index != -1) {
    return itemValue(*data, index);
  }

  Tuple new_data(&scope, *data);
  if (data.length() == 0 || 10 * set.numFilled() >= 3 * data.length()) {
    new_data = setGrow(thread, set);
  }
  index = setLookupForInsertion(thread, new_data, value, hash);
  DCHECK(index != -1, "unexpected index %ld", index);
  set.setData(*new_data);
  itemAtPut(*new_data, index, hash, *value);
  set.setNumItems(set.numItems() + 1);
  set.setNumFilled(set.numFilled() + 1);
  return *value;
}

bool setIncludes(Thread* thread, const SetBase& set, const Object& key,
                 word hash) {
  HandleScope scope(thread);
  Tuple data(&scope, set.data());
  return setLookup(thread, data, key, hash) != -1;
}

RawObject setIntersection(Thread* thread, const SetBase& set,
                          const Object& iterable) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  SetBase dst(&scope, runtime->isInstanceOfSet(*set) ? runtime->newSet()
                                                     : runtime->newFrozenSet());
  Object value(&scope, NoneType::object());
  // Special case for sets
  if (runtime->isInstanceOfSetBase(*iterable)) {
    SetBase self(&scope, *set);
    SetBase other(&scope, *iterable);
    if (set.numItems() == 0 || other.numItems() == 0) {
      return *dst;
    }
    // Iterate over the smaller set
    if (set.numItems() > other.numItems()) {
      self = *iterable;
      other = *set;
    }
    Tuple other_data(&scope, other.data());
    for (word i = 0, hash; setNextItemHash(self, &i, &value, &hash);) {
      if (setLookup(thread, other_data, value, hash) != -1) {
        setAdd(thread, dst, value, hash);
      }
    }
    return *dst;
  }
  // Generic case
  Object iter_method(&scope,
                     Interpreter::lookupMethod(thread, iterable, ID(__iter__)));
  if (iter_method.isError()) {
    return thread->raiseWithFmt(LayoutId::kTypeError, "object is not iterable");
  }
  Object iterator(&scope,
                  Interpreter::callMethod1(thread, iter_method, iterable));
  if (iterator.isError()) {
    return thread->raiseWithFmt(LayoutId::kTypeError, "object is not iterable");
  }
  Object next_method(&scope,
                     Interpreter::lookupMethod(thread, iterator, ID(__next__)));
  if (next_method.isError()) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "iter() returned a non-iterator");
  }
  if (set.numItems() == 0) {
    return *dst;
  }
  Tuple data(&scope, set.data());
  Object hash_obj(&scope, NoneType::object());
  for (;;) {
    value = Interpreter::callMethod1(thread, next_method, iterator);
    if (value.isError()) {
      if (thread->clearPendingStopIteration()) break;
      return *value;
    }
    hash_obj = Interpreter::hash(thread, value);
    if (hash_obj.isErrorException()) return *hash_obj;
    word hash = SmallInt::cast(*hash_obj).value();
    if (setLookup(thread, data, value, hash) != -1) {
      setAdd(thread, dst, value, hash);
    }
  }
  return *dst;
}

bool setRemove(Thread* thread, const Set& set, const Object& key, word hash) {
  HandleScope scope(thread);
  Tuple data(&scope, set.data());
  word index = setLookup(thread, data, key, hash);
  if (index != -1) {
    itemAtPutTombstone(*data, index);
    set.setNumItems(set.numItems() - 1);
    return true;
  }
  return false;
}

RawObject setUpdate(Thread* thread, const SetBase& dst,
                    const Object& iterable) {
  HandleScope scope(thread);
  Object elt(&scope, NoneType::object());
  Object hash_obj(&scope, NoneType::object());
  // Special case for lists
  if (iterable.isList()) {
    List src(&scope, *iterable);
    for (word i = 0; i < src.numItems(); i++) {
      elt = src.at(i);
      hash_obj = Interpreter::hash(thread, elt);
      if (hash_obj.isErrorException()) return *hash_obj;
      word hash = SmallInt::cast(*hash_obj).value();
      setAdd(thread, dst, elt, hash);
    }
    return *dst;
  }
  // Special case for lists iterators
  if (iterable.isListIterator()) {
    ListIterator list_iter(&scope, *iterable);
    List src(&scope, list_iter.iterable());
    for (word i = 0; i < src.numItems(); i++) {
      elt = src.at(i);
      hash_obj = Interpreter::hash(thread, elt);
      if (hash_obj.isErrorException()) return *hash_obj;
      word hash = SmallInt::cast(*hash_obj).value();
      setAdd(thread, dst, elt, hash);
    }
  }
  // Special case for tuples
  if (iterable.isTuple()) {
    Tuple tuple(&scope, *iterable);
    if (tuple.length() > 0) {
      for (word i = 0; i < tuple.length(); i++) {
        elt = tuple.at(i);
        hash_obj = Interpreter::hash(thread, elt);
        if (hash_obj.isErrorException()) return *hash_obj;
        word hash = SmallInt::cast(*hash_obj).value();
        setAdd(thread, dst, elt, hash);
      }
    }
    return *dst;
  }
  // Special case for built-in set types
  if (thread->runtime()->isInstanceOfSetBase(*iterable)) {
    SetBase src(&scope, *iterable);
    if (src.numItems() > 0) {
      for (word i = 0, hash; setNextItemHash(src, &i, &elt, &hash);) {
        // take hash from data to avoid recomputing it.
        setAdd(thread, dst, elt, hash);
      }
    }
    return *dst;
  }
  // Special case for dicts
  if (iterable.isDict()) {
    Dict dict(&scope, *iterable);
    for (word i = 0, hash; dictNextKeyHash(dict, &i, &elt, &hash);) {
      setAdd(thread, dst, elt, hash);
    }
    return *dst;
  }
  // Generic case
  Object iter_method(&scope,
                     Interpreter::lookupMethod(thread, iterable, ID(__iter__)));
  if (iter_method.isError()) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "'%T' object is not iterable", &iterable);
  }
  Object iterator(&scope,
                  Interpreter::callMethod1(thread, iter_method, iterable));
  if (iterator.isError()) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "'%T' object is not iterable", &iterable);
  }
  Object next_method(&scope,
                     Interpreter::lookupMethod(thread, iterator, ID(__next__)));
  if (next_method.isError()) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "iter() returned a non-iterator");
  }
  for (;;) {
    elt = Interpreter::callMethod1(thread, next_method, iterator);
    if (elt.isError()) {
      if (thread->clearPendingStopIteration()) break;
      return *elt;
    }
    hash_obj = Interpreter::hash(thread, elt);
    if (hash_obj.isErrorException()) return *hash_obj;
    word hash = SmallInt::cast(*hash_obj).value();
    setAdd(thread, dst, elt, hash);
  }
  return *dst;
}

RawSmallInt frozensetHash(Thread* thread, const Object& frozenset) {
  HandleScope scope(thread);
  FrozenSet set(&scope, *frozenset);
  uword result = 0;
  Object value(&scope, NoneType::object());
  for (word i = 0, value_hash; setNextItemHash(set, &i, &value, &value_hash);) {
    uword h = static_cast<uword>(value_hash);
    result ^= ((h ^ uword{89869747}) ^ (h << 16)) * uword{3644798167};
  }
  result ^= static_cast<uword>(set.numItems() + 1) * uword{1927868237};

  result ^= (result >> 11) ^ (result >> 25);
  result = result * uword{69069} + uword{907133923};

  // cpython replaces `-1` results with -2, because -1 is used as an
  // "uninitialized hash" marker in some situations. We do not use the same
  // marker, but do the same to match behavior.
  if (result == static_cast<uword>(word{-1})) {
    result--;
  }
  return SmallInt::fromWordTruncated(result);
}

static RawObject dunderLenImpl(Thread* thread, Frame* frame, word nargs,
                               SymbolId id) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfSetBase(*self)) {
    return thread->raiseRequiresType(self, id);
  }
  SetBase set(&scope, *self);
  return SmallInt::fromWord(set.numItems());
}

static RawObject dunderContainsImpl(Thread* thread, Frame* frame, word nargs,
                                    SymbolId id) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfSetBase(*self)) {
    return thread->raiseRequiresType(self, id);
  }
  SetBase set(&scope, *self);
  Object key(&scope, args.get(1));
  Object hash_obj(&scope, Interpreter::hash(thread, key));
  if (hash_obj.isErrorException()) return *hash_obj;
  word hash = SmallInt::cast(*hash_obj).value();
  return Bool::fromBool(setIncludes(thread, set, key, hash));
}

static RawObject dunderIterImpl(Thread* thread, Frame* frame, word nargs,
                                SymbolId id) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfSetBase(*self)) {
    return thread->raiseRequiresType(self, id);
  }
  return thread->runtime()->newSetIterator(self);
}

static RawObject isdisjointImpl(Thread* thread, Frame* frame, word nargs,
                                SymbolId id) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  Object other(&scope, args.get(1));
  Object value(&scope, NoneType::object());
  if (!thread->runtime()->isInstanceOfSetBase(*self)) {
    return thread->raiseRequiresType(self, id);
  }
  SetBase a(&scope, *self);
  if (a.numItems() == 0) {
    return Bool::trueObj();
  }
  if (thread->runtime()->isInstanceOfSetBase(*other)) {
    SetBase b(&scope, *other);
    if (b.numItems() == 0) {
      return Bool::trueObj();
    }
    // Iterate over the smaller set
    if (a.numItems() > b.numItems()) {
      a = *other;
      b = *self;
    }
    for (word i = 0, hash; setNextItemHash(a, &i, &value, &hash);) {
      if (setIncludes(thread, b, value, hash)) {
        return Bool::falseObj();
      }
    }
    return Bool::trueObj();
  }
  // Generic iterator case
  Object iter_method(&scope,
                     Interpreter::lookupMethod(thread, other, ID(__iter__)));
  if (iter_method.isError()) {
    return thread->raiseWithFmt(LayoutId::kTypeError, "object is not iterable");
  }
  Object iterator(&scope, Interpreter::callMethod1(thread, iter_method, other));
  if (iterator.isError()) {
    return thread->raiseWithFmt(LayoutId::kTypeError, "object is not iterable");
  }
  Object next_method(&scope,
                     Interpreter::lookupMethod(thread, iterator, ID(__next__)));
  if (next_method.isError()) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "iter() returned a non-iterator");
  }
  Object hash_obj(&scope, NoneType::object());
  for (;;) {
    value = Interpreter::callMethod1(thread, next_method, iterator);
    if (value.isError()) {
      if (thread->clearPendingStopIteration()) break;
      return *value;
    }
    hash_obj = Interpreter::hash(thread, value);
    if (hash_obj.isErrorException()) return *hash_obj;
    word hash = SmallInt::cast(*hash_obj).value();
    if (setIncludes(thread, a, value, hash)) {
      return Bool::falseObj();
    }
  }
  return Bool::trueObj();
}

static RawObject intersectionImpl(Thread* thread, Frame* frame, word nargs,
                                  SymbolId id) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfSetBase(*self)) {
    return thread->raiseRequiresType(self, id);
  }
  SetBase set(&scope, *self);
  // TODO(T46058798): convert others to starargs
  if (nargs == 1) {
    // Return a copy of the set
    return setCopy(thread, set);
  }
  // nargs is at least 2
  Object other(&scope, args.get(1));
  Object result(&scope, setIntersection(thread, set, other));
  if (result.isError() || nargs == 2) {
    return *result;
  }

  SetBase base(&scope, *result);
  for (word i = 2; i < nargs; i++) {
    other = args.get(i);
    result = setIntersection(thread, base, other);
    if (result.isError()) {
      return *result;
    }
    base = *result;
    // Early exit
    if (base.numItems() == 0) {
      break;
    }
  }
  return *result;
}

static RawObject dunderEqImpl(Thread* thread, Frame* frame, word nargs,
                              SymbolId id) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  Object other(&scope, args.get(1));
  if (!runtime->isInstanceOfSetBase(*self)) {
    return thread->raiseRequiresType(self, id);
  }
  if (!runtime->isInstanceOfSetBase(*other)) {
    return NotImplementedType::object();
  }
  SetBase set(&scope, *self);
  SetBase other_set(&scope, *other);
  return Bool::fromBool(setEquals(thread, set, other_set));
}

static RawObject dunderNeImpl(Thread* thread, Frame* frame, word nargs,
                              SymbolId id) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  Object other(&scope, args.get(1));
  if (!runtime->isInstanceOfSetBase(*self)) {
    return thread->raiseRequiresType(self, id);
  }
  if (!runtime->isInstanceOfSetBase(*other)) {
    return NotImplementedType::object();
  }
  SetBase set(&scope, *self);
  SetBase other_set(&scope, *other);
  return Bool::fromBool(!setEquals(thread, set, other_set));
}

static RawObject dunderLeImpl(Thread* thread, Frame* frame, word nargs,
                              SymbolId id) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  Object other(&scope, args.get(1));
  if (!runtime->isInstanceOfSetBase(*self)) {
    return thread->raiseRequiresType(self, id);
  }
  if (!runtime->isInstanceOfSetBase(*other)) {
    return NotImplementedType::object();
  }
  SetBase set(&scope, *self);
  SetBase other_set(&scope, *other);
  return Bool::fromBool(setIsSubset(thread, set, other_set));
}

static RawObject dunderLtImpl(Thread* thread, Frame* frame, word nargs,
                              SymbolId id) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  Object other(&scope, args.get(1));
  if (!runtime->isInstanceOfSetBase(*self)) {
    return thread->raiseRequiresType(self, id);
  }
  if (!runtime->isInstanceOfSetBase(*other)) {
    return NotImplementedType::object();
  }
  SetBase set(&scope, *self);
  SetBase other_set(&scope, *other);
  return Bool::fromBool(setIsProperSubset(thread, set, other_set));
}

static RawObject dunderGeImpl(Thread* thread, Frame* frame, word nargs,
                              SymbolId id) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  Object other(&scope, args.get(1));
  if (!runtime->isInstanceOfSetBase(*self)) {
    return thread->raiseRequiresType(self, id);
  }
  if (!runtime->isInstanceOfSetBase(*other)) {
    return NotImplementedType::object();
  }
  SetBase set(&scope, *self);
  SetBase other_set(&scope, *other);
  return Bool::fromBool(setIsSubset(thread, other_set, set));
}

static RawObject dunderGtImpl(Thread* thread, Frame* frame, word nargs,
                              SymbolId id) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  Object other(&scope, args.get(1));
  if (!runtime->isInstanceOfSetBase(*self)) {
    return thread->raiseRequiresType(self, id);
  }
  if (!runtime->isInstanceOfSetBase(*other)) {
    return NotImplementedType::object();
  }
  SetBase set(&scope, *self);
  SetBase other_set(&scope, *other);
  return Bool::fromBool(setIsProperSubset(thread, other_set, set));
}

static const BuiltinAttribute kFrozenSetAttributes[] = {
    {ID(_frozenset__num_items), RawFrozenSet::kNumItemsOffset,
     AttributeFlags::kHidden},
    {ID(_frozenset__data), RawFrozenSet::kDataOffset, AttributeFlags::kHidden},
};

RawObject METH(frozenset, __and__)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  Object other(&scope, args.get(1));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfFrozenSet(*self)) {
    return thread->raiseRequiresType(self, ID(frozenset));
  }
  if (!runtime->isInstanceOfSetBase(*other)) {
    return NotImplementedType::object();
  }
  FrozenSet set(&scope, *self);
  SetBase other_set(&scope, *other);
  return setIntersection(thread, set, other_set);
}

RawObject METH(frozenset, __contains__)(Thread* thread, Frame* frame,
                                        word nargs) {
  return dunderContainsImpl(thread, frame, nargs, ID(frozenset));
}

RawObject METH(frozenset, __eq__)(Thread* thread, Frame* frame, word nargs) {
  return dunderEqImpl(thread, frame, nargs, ID(frozenset));
}

RawObject METH(frozenset, __ge__)(Thread* thread, Frame* frame, word nargs) {
  return dunderGeImpl(thread, frame, nargs, ID(frozenset));
}

RawObject METH(frozenset, __gt__)(Thread* thread, Frame* frame, word nargs) {
  return dunderGtImpl(thread, frame, nargs, ID(frozenset));
}

RawObject METH(frozenset, __hash__)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfFrozenSet(*self)) {
    return thread->raiseRequiresType(self, ID(frozenset));
  }
  FrozenSet set(&scope, *self);
  return frozensetHash(thread, set);
}

RawObject METH(frozenset, __iter__)(Thread* thread, Frame* frame, word nargs) {
  return dunderIterImpl(thread, frame, nargs, ID(frozenset));
}

RawObject METH(frozenset, __le__)(Thread* thread, Frame* frame, word nargs) {
  return dunderLeImpl(thread, frame, nargs, ID(frozenset));
}

RawObject METH(frozenset, __len__)(Thread* thread, Frame* frame, word nargs) {
  return dunderLenImpl(thread, frame, nargs, ID(frozenset));
}

RawObject METH(frozenset, __lt__)(Thread* thread, Frame* frame, word nargs) {
  return dunderLtImpl(thread, frame, nargs, ID(frozenset));
}

RawObject METH(frozenset, __ne__)(Thread* thread, Frame* frame, word nargs) {
  return dunderNeImpl(thread, frame, nargs, ID(frozenset));
}

RawObject METH(frozenset, __new__)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object type_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfType(*type_obj)) {
    return thread->raiseRequiresType(type_obj, ID(type));
  }
  Type type(&scope, *type_obj);
  if (type.builtinBase() != LayoutId::kFrozenSet) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "not a subtype of frozenset");
  }
  if (args.get(1).isUnbound()) {
    // Iterable not provided
    if (type.isBuiltin() && type.builtinBase() == LayoutId::kFrozenSet) {
      // Called with exact frozenset type, should return singleton
      return runtime->emptyFrozenSet();
    }
    // Not called with exact frozenset type, should return new distinct
    // frozenset
    Layout layout(&scope, type.instanceLayout());
    FrozenSet result(&scope, runtime->newInstance(layout));
    result.setNumItems(0);
    result.setNumFilled(0);
    result.setData(runtime->emptyTuple());
    return *result;
  }
  // Called with iterable, so iterate
  Object iterable(&scope, args.get(1));
  // frozenset(f) where f is a frozenset is idempotent
  if (iterable.isFrozenSet()) {
    return *iterable;
  }
  Object dunder_iter(&scope,
                     Interpreter::lookupMethod(thread, iterable, ID(__iter__)));
  if (dunder_iter.isError()) {
    return thread->raiseWithFmt(
        LayoutId::kTypeError,
        "frozenset.__new__ must be called with an iterable");
  }
  if (type.isBuiltin() && type.builtinBase() == LayoutId::kFrozenSet) {
    // Called with exact frozenset type
    FrozenSet result(&scope, runtime->newFrozenSet());
    RawObject maybe_error = setUpdate(thread, result, iterable);
    if (maybe_error.isError()) return maybe_error;
    result = maybe_error;
    if (result.numItems() == 0) {
      return runtime->emptyFrozenSet();
    }
    return *result;
  }
  // Not called with exact frozenset type, should return new frozenset subclass
  Layout layout(&scope, type.instanceLayout());
  FrozenSet result(&scope, runtime->newInstance(layout));
  result.setNumItems(0);
  result.setNumFilled(0);
  result.setData(runtime->emptyTuple());
  return setUpdate(thread, result, iterable);
}

RawObject METH(frozenset, __or__)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  Object other(&scope, args.get(1));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfFrozenSet(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(frozenset));
  }
  if (!runtime->isInstanceOfSetBase(*other)) {
    return NotImplementedType::object();
  }
  FrozenSet self(&scope, *self_obj);
  if (*self == *other) {
    // Make a shallow copy; we can alias the underlying immutable data
    // structure
    FrozenSet result(&scope, runtime->newFrozenSet());
    result.setData(self.data());
    result.setNumItems(self.numItems());
    result.setNumFilled(self.numFilled());
    return *result;
  }
  FrozenSet result(&scope, runtime->newFrozenSet());
  Object maybe_error(&scope, setUpdate(thread, result, self));
  if (maybe_error.isError()) return *maybe_error;
  result = *maybe_error;
  return setUpdate(thread, result, other);
}

RawObject METH(frozenset, copy)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfFrozenSet(*self)) {
    return thread->raiseRequiresType(self, ID(frozenset));
  }
  FrozenSet set(&scope, *self);
  if (set.isFrozenSet()) {
    return *set;
  }
  return setCopy(thread, set);
}

RawObject METH(frozenset, intersection)(Thread* thread, Frame* frame,
                                        word nargs) {
  return intersectionImpl(thread, frame, nargs, ID(frozenset));
}

RawObject METH(frozenset, isdisjoint)(Thread* thread, Frame* frame,
                                      word nargs) {
  return isdisjointImpl(thread, frame, nargs, ID(frozenset));
}

RawObject setCopy(Thread* thread, const SetBase& set) {
  word num_items = set.numItems();
  Runtime* runtime = thread->runtime();
  if (num_items == 0) {
    return runtime->isInstanceOfSet(*set) ? runtime->newSet()
                                          : runtime->emptyFrozenSet();
  }

  HandleScope scope(thread);
  SetBase new_set(&scope, runtime->isInstanceOfSet(*set)
                              ? runtime->newSet()
                              : runtime->newFrozenSet());
  Tuple data(&scope, set.data());
  MutableTuple new_data(&scope, runtime->newMutableTuple(data.length()));
  Object value(&scope, NoneType::object());
  for (word i = 0, hash; setNextItemHash(set, &i, &value, &hash);) {
    word dst_index = i - kNumPointers;
    itemAtPut(*new_data, dst_index, hash, *value);
  }
  new_set.setData(*new_data);
  new_set.setNumItems(set.numItems());
  new_set.setNumFilled(set.numItems());
  return *new_set;
}

bool setIsSubset(Thread* thread, const SetBase& set, const SetBase& other) {
  HandleScope scope(thread);
  Object value(&scope, NoneType::object());
  for (word i = 0, hash; setNextItemHash(set, &i, &value, &hash);) {
    if (!setIncludes(thread, other, value, hash)) {
      return false;
    }
  }
  return true;
}

bool setIsProperSubset(Thread* thread, const SetBase& set,
                       const SetBase& other) {
  if (set.numItems() == other.numItems()) {
    return false;
  }
  return setIsSubset(thread, set, other);
}

bool setEquals(Thread* thread, const SetBase& set, const SetBase& other) {
  if (set.numItems() != other.numItems()) {
    return false;
  }
  if (*set == *other) {
    return true;
  }
  return setIsSubset(thread, set, other);
}

RawObject setPop(Thread* thread, const Set& set) {
  HandleScope scope(thread);
  Tuple data(&scope, set.data());
  word num_items = set.numItems();
  if (num_items != 0) {
    Object value(&scope, NoneType::object());
    for (word i = 0; setNextItem(set, &i, &value);) {
      itemAtPutTombstone(*data, i - kNumPointers);
      set.setNumItems(num_items - 1);
      return *value;
    }
  }
  // num_items == 0 or all items were found empty
  return thread->raiseWithFmt(LayoutId::kKeyError, "pop from an empty set");
}

RawObject setIteratorNext(Thread* thread, const SetIterator& iter) {
  word idx = iter.index();
  HandleScope scope(thread);
  SetBase underlying(&scope, iter.iterable());
  Object value(&scope, NoneType::object());
  // Find the next non empty item
  if (!setNextItem(underlying, &idx, &value)) {
    return Error::noMoreItems();
  }
  iter.setConsumedCount(iter.consumedCount() + 1);
  iter.setIndex(idx);
  return *value;
}

static const BuiltinAttribute kSetAttributes[] = {
    {ID(_set__data), RawSet::kDataOffset, AttributeFlags::kHidden},
    {ID(_set__num_items), RawSet::kNumItemsOffset, AttributeFlags::kHidden},
    {ID(_set__num_filled), RawSet::kNumFilledOffset, AttributeFlags::kHidden},
};

RawObject METH(set, __and__)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  Object other(&scope, args.get(1));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfSet(*self)) {
    return thread->raiseRequiresType(self, ID(set));
  }
  if (!runtime->isInstanceOfSetBase(*other)) {
    return NotImplementedType::object();
  }
  Set set(&scope, *self);
  SetBase other_set(&scope, *other);
  return setIntersection(thread, set, other_set);
}

RawObject METH(set, __contains__)(Thread* thread, Frame* frame, word nargs) {
  return dunderContainsImpl(thread, frame, nargs, ID(set));
}

RawObject METH(set, __eq__)(Thread* thread, Frame* frame, word nargs) {
  return dunderEqImpl(thread, frame, nargs, ID(set));
}

RawObject METH(set, __ge__)(Thread* thread, Frame* frame, word nargs) {
  return dunderGeImpl(thread, frame, nargs, ID(set));
}

RawObject METH(set, __gt__)(Thread* thread, Frame* frame, word nargs) {
  return dunderGtImpl(thread, frame, nargs, ID(set));
}

RawObject METH(set, __iand__)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  Object other(&scope, args.get(1));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfSet(*self)) {
    return thread->raiseRequiresType(self, ID(set));
  }
  if (!runtime->isInstanceOfSet(*other)) {
    return NotImplementedType::object();
  }
  Set set(&scope, *self);
  Object intersection(&scope, setIntersection(thread, set, other));
  if (intersection.isError()) {
    return *intersection;
  }
  RawSet intersection_set = Set::cast(*intersection);
  set.setData(intersection_set.data());
  set.setNumItems(intersection_set.numItems());
  set.setNumFilled(intersection_set.numFilled());
  return *set;
}

RawObject METH(set, __init__)(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  if (!runtime->isInstanceOfSet(*self)) {
    return thread->raiseRequiresType(self, ID(set));
  }
  Set set(&scope, *self);
  Object iterable(&scope, args.get(1));
  Object result(&scope, setUpdate(thread, set, iterable));
  if (result.isError()) {
    return *result;
  }
  return NoneType::object();
}

RawObject METH(set, __iter__)(Thread* thread, Frame* frame, word nargs) {
  return dunderIterImpl(thread, frame, nargs, ID(set));
}

RawObject METH(set, __le__)(Thread* thread, Frame* frame, word nargs) {
  return dunderLeImpl(thread, frame, nargs, ID(set));
}

RawObject METH(set, __len__)(Thread* thread, Frame* frame, word nargs) {
  return dunderLenImpl(thread, frame, nargs, ID(set));
}

RawObject METH(set, __lt__)(Thread* thread, Frame* frame, word nargs) {
  return dunderLtImpl(thread, frame, nargs, ID(set));
}

RawObject METH(set, __ne__)(Thread* thread, Frame* frame, word nargs) {
  return dunderNeImpl(thread, frame, nargs, ID(set));
}

RawObject METH(set, __new__)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object type_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfType(*type_obj)) {
    return thread->raiseRequiresType(type_obj, ID(type));
  }
  Type type(&scope, *type_obj);
  if (type.builtinBase() != LayoutId::kSet) {
    return thread->raiseWithFmt(LayoutId::kTypeError, "not a subtype of set");
  }
  Layout layout(&scope, type.instanceLayout());
  Set result(&scope, runtime->newInstance(layout));
  result.setNumItems(0);
  result.setNumFilled(0);
  result.setData(runtime->emptyTuple());
  return *result;
}

RawObject METH(set, add)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfSet(*self)) {
    return thread->raiseRequiresType(self, ID(set));
  }
  Set set(&scope, *self);
  Object value(&scope, args.get(1));
  Object hash_obj(&scope, Interpreter::hash(thread, value));
  if (hash_obj.isErrorException()) return *hash_obj;
  word hash = SmallInt::cast(*hash_obj).value();

  Object result(&scope, setAdd(thread, set, value, hash));
  if (result.isError()) return *result;
  return NoneType::object();
}

RawObject METH(set, clear)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfSet(*self)) {
    return thread->raiseRequiresType(self, ID(set));
  }
  Set set(&scope, *self);
  if (set.numItems() == 0) {
    return NoneType::object();
  }
  set.setNumItems(0);
  set.setNumFilled(0);
  MutableTuple data(&scope, set.data());
  data.fill(NoneType::object());
  return NoneType::object();
}

RawObject METH(set, copy)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfSet(*self)) {
    return thread->raiseRequiresType(self, ID(set));
  }
  Set set(&scope, *self);
  return setCopy(thread, set);
}

RawObject METH(set, discard)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfSet(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(set));
  }
  Set self(&scope, *self_obj);
  Object key(&scope, args.get(1));
  Object hash_obj(&scope, Interpreter::hash(thread, key));
  if (hash_obj.isErrorException()) return *hash_obj;
  word hash = SmallInt::cast(*hash_obj).value();
  setRemove(thread, self, key, hash);
  return NoneType::object();
}

RawObject METH(set, intersection)(Thread* thread, Frame* frame, word nargs) {
  return intersectionImpl(thread, frame, nargs, ID(set));
}

RawObject METH(set, isdisjoint)(Thread* thread, Frame* frame, word nargs) {
  return isdisjointImpl(thread, frame, nargs, ID(set));
}

RawObject METH(set, pop)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfSet(*self)) {
    return thread->raiseRequiresType(self, ID(set));
  }
  Set set(&scope, args.get(0));
  return setPop(thread, set);
}

RawObject METH(set, remove)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfSet(*self)) {
    return thread->raiseRequiresType(self, ID(set));
  }
  Set set(&scope, *self);
  Object key(&scope, args.get(1));
  Object hash_obj(&scope, Interpreter::hash(thread, key));
  if (hash_obj.isErrorException()) return *hash_obj;
  word hash = SmallInt::cast(*hash_obj).value();
  if (!setRemove(thread, set, key, hash)) {
    return thread->raise(LayoutId::kKeyError, *key);
  }
  return NoneType::object();
}

RawObject METH(set, update)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Runtime* runtime = thread->runtime();
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfSet(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(set));
  }
  Set self(&scope, *self_obj);
  Object starargs_obj(&scope, args.get(1));
  if (!runtime->isInstanceOfTuple(*starargs_obj)) {
    return thread->raiseRequiresType(starargs_obj, ID(tuple));
  }
  Tuple starargs(&scope, tupleUnderlying(*starargs_obj));
  Object result(&scope, NoneType::object());
  for (word i = 0; i < starargs.length(); i++) {
    Object other(&scope, starargs.at(i));
    result = setUpdate(thread, self, other);
    if (result.isError()) {
      return *result;
    }
  }
  return NoneType::object();
}

static const BuiltinAttribute kSetIteratorAttributes[] = {
    {ID(_set_iterator__iterable), RawSetIterator::kIterableOffset,
     AttributeFlags::kHidden},
    {ID(_set_iterator__index), RawSetIterator::kIndexOffset,
     AttributeFlags::kHidden},
};

RawObject METH(set_iterator, __iter__)(Thread* thread, Frame* frame,
                                       word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isSetIterator()) {
    return thread->raiseRequiresType(self, ID(set_iterator));
  }
  return *self;
}

RawObject METH(set_iterator, __next__)(Thread* thread, Frame* frame,
                                       word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  if (!self_obj.isSetIterator()) {
    return thread->raiseRequiresType(self_obj, ID(set_iterator));
  }
  SetIterator self(&scope, *self_obj);
  Object value(&scope, setIteratorNext(thread, self));
  if (value.isError()) {
    return thread->raise(LayoutId::kStopIteration, NoneType::object());
  }
  return *value;
}

RawObject METH(set_iterator, __length_hint__)(Thread* thread, Frame* frame,
                                              word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isSetIterator()) {
    return thread->raiseRequiresType(self, ID(set_iterator));
  }
  SetIterator set_iterator(&scope, *self);
  Set set(&scope, set_iterator.iterable());
  return SmallInt::fromWord(set.numItems() - set_iterator.consumedCount());
}

void initializeSetTypes(Thread* thread) {
  addBuiltinType(thread, ID(set), LayoutId::kSet,
                 /*superclass_id=*/LayoutId::kObject, kSetAttributes);

  addBuiltinType(thread, ID(frozenset), LayoutId::kFrozenSet,
                 /*superclass_id=*/LayoutId::kObject, kFrozenSetAttributes);

  addBuiltinType(thread, ID(set_iterator), LayoutId::kSetIterator,
                 /*superclass_id=*/LayoutId::kObject, kSetIteratorAttributes);
}

}  // namespace py
