#include "set-builtins.h"

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace py {

enum class SetLookupType { Lookup, Insertion };

template <SetLookupType type>
static word setLookupImpl(Thread* thread, const Tuple& data, const Object& key,
                          word hash) {
  word start = SetBase::Bucket::getIndex(*data, hash);
  word current = start;
  word next_free_index = -1;

  // TODO(mpage) - Quadratic probing?
  word length = data.length();
  if (length == 0) {
    return -1;
  }

  do {
    if (SetBase::Bucket::hasValue(*data, current)) {
      RawObject eq = Runtime::objectEquals(
          thread, SetBase::Bucket::value(*data, current), *key);
      if (eq == Bool::trueObj()) {
        return current;
      }
      if (eq.isErrorException()) {
        UNIMPLEMENTED("exception in value comparison");
      }
    }
    if (next_free_index == -1 && SetBase::Bucket::isTombstone(*data, current)) {
      if (type == SetLookupType::Insertion) {
        return current;
      }
      next_free_index = current;
    } else if (SetBase::Bucket::isEmpty(*data, current)) {
      if (next_free_index == -1) {
        next_free_index = current;
      }
      break;
    }
    current = (current + SetBase::Bucket::kNumPointers) & (length - 1);
  } while (current != start);

  if (type == SetLookupType::Insertion) {
    return next_free_index;
  }
  return -1;
}

static word setLookup(Thread* thread, const Tuple& data, const Object& key,
                      word hash) {
  return setLookupImpl<SetLookupType::Lookup>(thread, data, key, hash);
}

static word setLookupForInsertion(Thread* thread, const Tuple& data,
                                  const Object& key, word hash) {
  return setLookupImpl<SetLookupType::Insertion>(thread, data, key, hash);
}

static RawTuple setGrow(Thread* thread, const Tuple& data) {
  HandleScope scope(thread);
  word new_length = data.length() * Runtime::kSetGrowthFactor;
  if (new_length == 0) {
    new_length = Runtime::kInitialSetCapacity * SetBase::Bucket::kNumPointers;
  }
  MutableTuple new_data(&scope, thread->runtime()->newMutableTuple(new_length));
  // Re-insert items
  Object value(&scope, NoneType::object());
  for (word i = SetBase::Bucket::kFirst;
       SetBase::Bucket::nextItem(*data, &i);) {
    value = SetBase::Bucket::value(*data, i);
    word hash = SetBase::Bucket::hash(*data, i);
    word index = setLookupForInsertion(thread, new_data, value, hash);
    DCHECK(index != -1, "unexpected index %ld", index);
    SetBase::Bucket::set(*new_data, index, hash, *value);
  }
  return *new_data;
}

RawObject setAdd(Thread* thread, const SetBase& set, const Object& value,
                 word hash) {
  HandleScope scope(thread);
  Tuple data(&scope, set.data());
  word index = setLookup(thread, data, value, hash);
  if (index != -1) {
    return SetBase::Bucket::value(*data, index);
  }
  Tuple new_data(&scope, *data);
  if (data.length() == 0 || set.numItems() >= data.length() / 2) {
    new_data = setGrow(thread, data);
  }
  index = setLookupForInsertion(thread, new_data, value, hash);
  DCHECK(index != -1, "unexpected index %ld", index);
  set.setData(*new_data);
  SetBase::Bucket::set(*new_data, index, hash, *value);
  set.setNumItems(set.numItems() + 1);
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
    Tuple data(&scope, self.data());
    Tuple other_data(&scope, other.data());
    for (word i = SetBase::Bucket::kFirst;
         SetBase::Bucket::nextItem(*data, &i);) {
      value = SetBase::Bucket::value(*data, i);
      word hash = SetBase::Bucket::hash(*data, i);
      if (setLookup(thread, other_data, value, hash) != -1) {
        setAdd(thread, dst, value, hash);
      }
    }
    return *dst;
  }
  // Generic case
  Object iter_method(
      &scope, Interpreter::lookupMethod(thread, thread->currentFrame(),
                                        iterable, SymbolId::kDunderIter));
  if (iter_method.isError()) {
    return thread->raiseWithFmt(LayoutId::kTypeError, "object is not iterable");
  }
  Object iterator(&scope,
                  Interpreter::callMethod1(thread, thread->currentFrame(),
                                           iter_method, iterable));
  if (iterator.isError()) {
    return thread->raiseWithFmt(LayoutId::kTypeError, "object is not iterable");
  }
  Object next_method(
      &scope, Interpreter::lookupMethod(thread, thread->currentFrame(),
                                        iterator, SymbolId::kDunderNext));
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
    value = Interpreter::callMethod1(thread, thread->currentFrame(),
                                     next_method, iterator);
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
    SetBase::Bucket::setTombstone(*data, index);
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
    Tuple data(&scope, src.data());
    if (src.numItems() > 0) {
      for (word i = SetBase::Bucket::kFirst;
           SetBase::Bucket::nextItem(*data, &i);) {
        elt = SetBase::Bucket::value(*data, i);
        // take hash from data to avoid recomputing it.
        word hash = SetBase::Bucket::hash(*data, i);
        setAdd(thread, dst, elt, hash);
      }
    }
    return *dst;
  }
  // Special case for dicts
  if (iterable.isDict()) {
    Dict dict(&scope, *iterable);
    Tuple data(&scope, dict.data());
    for (word i = Dict::Bucket::kFirst; Dict::Bucket::nextItem(*data, &i);) {
      elt = Dict::Bucket::key(*data, i);
      word hash = Dict::Bucket::hash(*data, i);
      setAdd(thread, dst, elt, hash);
    }
    return *dst;
  }
  // Generic case
  Object iter_method(
      &scope, Interpreter::lookupMethod(thread, thread->currentFrame(),
                                        iterable, SymbolId::kDunderIter));
  if (iter_method.isError()) {
    return thread->raiseWithFmt(LayoutId::kTypeError, "object is not iterable");
  }
  Object iterator(&scope,
                  Interpreter::callMethod1(thread, thread->currentFrame(),
                                           iter_method, iterable));
  if (iterator.isError()) {
    return thread->raiseWithFmt(LayoutId::kTypeError, "object is not iterable");
  }
  Object next_method(
      &scope, Interpreter::lookupMethod(thread, thread->currentFrame(),
                                        iterator, SymbolId::kDunderNext));
  if (next_method.isError()) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "iter() returned a non-iterator");
  }
  for (;;) {
    elt = Interpreter::callMethod1(thread, thread->currentFrame(), next_method,
                                   iterator);
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
  Tuple data(&scope, set.data());

  uword result = 0;
  for (word i = SetBase::Bucket::kFirst;
       SetBase::Bucket::nextItem(*data, &i);) {
    word value_hash = SetBase::Bucket::hash(*data, i);
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

RawObject SetBaseBuiltins::dunderLen(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfSetBase(*self)) {
    return thread->raiseWithFmt(
        LayoutId::kTypeError,
        "'__len__' requires a 'set' or 'frozenset' object");
  }
  SetBase set(&scope, *self);
  return SmallInt::fromWord(set.numItems());
}

RawObject SetBaseBuiltins::dunderContains(Thread* thread, Frame* frame,
                                          word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfSetBase(*self)) {
    return thread->raiseWithFmt(
        LayoutId::kTypeError,
        "__contains__() requires a 'set' or 'frozenset' object");
  }
  SetBase set(&scope, *self);
  Object key(&scope, args.get(1));
  Object hash_obj(&scope, Interpreter::hash(thread, key));
  if (hash_obj.isErrorException()) return *hash_obj;
  word hash = SmallInt::cast(*hash_obj).value();
  return Bool::fromBool(setIncludes(thread, set, key, hash));
}

RawObject SetBaseBuiltins::dunderIter(Thread* thread, Frame* frame,
                                      word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfSetBase(*self)) {
    return thread->raiseWithFmt(
        LayoutId::kTypeError,
        "__iter__() must be called with a 'set' or 'frozenset' instance as the "
        "first argument");
  }
  return thread->runtime()->newSetIterator(self);
}

RawObject SetBaseBuiltins::isDisjoint(Thread* thread, Frame* frame,
                                      word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  Object other(&scope, args.get(1));
  Object value(&scope, NoneType::object());
  if (!thread->runtime()->isInstanceOfSetBase(*self)) {
    return thread->raiseWithFmt(
        LayoutId::kTypeError,
        "isdisjoint() requires a 'set' or 'frozenset' object");
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
    Tuple data(&scope, a.data());
    for (word i = SetBase::Bucket::kFirst;
         SetBase::Bucket::nextItem(*data, &i);) {
      value = SetBase::Bucket::value(*data, i);
      word hash = SetBase::Bucket::hash(*data, i);
      if (setIncludes(thread, b, value, hash)) {
        return Bool::falseObj();
      }
    }
    return Bool::trueObj();
  }
  // Generic iterator case
  Object iter_method(
      &scope, Interpreter::lookupMethod(thread, thread->currentFrame(), other,
                                        SymbolId::kDunderIter));
  if (iter_method.isError()) {
    return thread->raiseWithFmt(LayoutId::kTypeError, "object is not iterable");
  }
  Object iterator(&scope,
                  Interpreter::callMethod1(thread, thread->currentFrame(),
                                           iter_method, other));
  if (iterator.isError()) {
    return thread->raiseWithFmt(LayoutId::kTypeError, "object is not iterable");
  }
  Object next_method(
      &scope, Interpreter::lookupMethod(thread, thread->currentFrame(),
                                        iterator, SymbolId::kDunderNext));
  if (next_method.isError()) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "iter() returned a non-iterator");
  }
  Object hash_obj(&scope, NoneType::object());
  for (;;) {
    value = Interpreter::callMethod1(thread, thread->currentFrame(),
                                     next_method, iterator);
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

RawObject SetBaseBuiltins::dunderAnd(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  Object other(&scope, args.get(1));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfSetBase(*self)) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "__and__() requires a 'set' object");
  }
  if (!runtime->isInstanceOfSetBase(*other)) {
    return NotImplementedType::object();
  }
  SetBase set(&scope, *self);
  SetBase other_set(&scope, *other);
  return setIntersection(thread, set, other_set);
}

RawObject SetBaseBuiltins::intersection(Thread* thread, Frame* frame,
                                        word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfSetBase(*self)) {
    return thread->raiseWithFmt(
        LayoutId::kTypeError,
        "intersection() requires a 'set' or 'frozenset' object");
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

RawObject SetBaseBuiltins::dunderEq(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  Object other(&scope, args.get(1));
  if (!runtime->isInstanceOfSetBase(*self)) {
    return thread->raiseWithFmt(
        LayoutId::kTypeError,
        "__eq__() requires a 'set' or 'frozenset' object");
  }
  if (!runtime->isInstanceOfSetBase(*other)) {
    return NotImplementedType::object();
  }
  SetBase set(&scope, *self);
  SetBase other_set(&scope, *other);
  return Bool::fromBool(setEquals(thread, set, other_set));
}

RawObject SetBaseBuiltins::dunderNe(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  Object other(&scope, args.get(1));
  if (!runtime->isInstanceOfSetBase(*self)) {
    return thread->raiseWithFmt(
        LayoutId::kTypeError,
        "__ne__() requires a 'set' or 'frozenset' object");
  }
  if (!runtime->isInstanceOfSetBase(*other)) {
    return NotImplementedType::object();
  }
  SetBase set(&scope, *self);
  SetBase other_set(&scope, *other);
  return Bool::fromBool(!setEquals(thread, set, other_set));
}

RawObject SetBaseBuiltins::dunderLe(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  Object other(&scope, args.get(1));
  if (!runtime->isInstanceOfSetBase(*self)) {
    return thread->raiseWithFmt(
        LayoutId::kTypeError,
        "__le__() requires a 'set' or 'frozenset' object");
  }
  if (!runtime->isInstanceOfSetBase(*other)) {
    return NotImplementedType::object();
  }
  SetBase set(&scope, *self);
  SetBase other_set(&scope, *other);
  return Bool::fromBool(setIsSubset(thread, set, other_set));
}

RawObject SetBaseBuiltins::dunderLt(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  Object other(&scope, args.get(1));
  if (!runtime->isInstanceOfSetBase(*self)) {
    return thread->raiseWithFmt(
        LayoutId::kTypeError,
        "__lt__() requires a 'set' or 'frozenset' object");
  }
  if (!runtime->isInstanceOfSetBase(*other)) {
    return NotImplementedType::object();
  }
  SetBase set(&scope, *self);
  SetBase other_set(&scope, *other);
  return Bool::fromBool(setIsProperSubset(thread, set, other_set));
}

RawObject SetBaseBuiltins::dunderGe(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  Object other(&scope, args.get(1));
  if (!runtime->isInstanceOfSetBase(*self)) {
    return thread->raiseWithFmt(
        LayoutId::kTypeError,
        "__ge__() requires a 'set' or 'frozenset' object");
  }
  if (!runtime->isInstanceOfSetBase(*other)) {
    return NotImplementedType::object();
  }
  SetBase set(&scope, *self);
  SetBase other_set(&scope, *other);
  return Bool::fromBool(setIsSubset(thread, other_set, set));
}

RawObject SetBaseBuiltins::dunderGt(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  Object other(&scope, args.get(1));
  if (!runtime->isInstanceOfSetBase(*self)) {
    return thread->raiseWithFmt(
        LayoutId::kTypeError,
        "__gt__() requires a 'set' or 'frozenset' object");
  }
  if (!runtime->isInstanceOfSetBase(*other)) {
    return NotImplementedType::object();
  }
  SetBase set(&scope, *self);
  SetBase other_set(&scope, *other);
  return Bool::fromBool(setIsProperSubset(thread, other_set, set));
}

const BuiltinAttribute FrozenSetBuiltins::kAttributes[] = {
    {SymbolId::kInvalid, Set::kDataOffset},
    {SymbolId::kInvalid, Set::kNumItemsOffset},
    {SymbolId::kSentinelId, -1},
};

const BuiltinMethod FrozenSetBuiltins::kBuiltinMethods[] = {
    {SymbolId::kCopy, copy},
    {SymbolId::kDunderAnd, dunderAnd},
    {SymbolId::kDunderContains, dunderContains},
    {SymbolId::kDunderEq, dunderEq},
    {SymbolId::kDunderGe, dunderGe},
    {SymbolId::kDunderGt, dunderGt},
    {SymbolId::kDunderHash, dunderHash},
    {SymbolId::kDunderIter, dunderIter},
    {SymbolId::kDunderLe, dunderLe},
    {SymbolId::kDunderLen, dunderLen},
    {SymbolId::kDunderLt, dunderLt},
    {SymbolId::kDunderNe, dunderNe},
    {SymbolId::kDunderNew, dunderNew},
    {SymbolId::kIntersection, intersection},
    {SymbolId::kIsDisjoint, isDisjoint},
    {SymbolId::kSentinelId, nullptr},
};

RawObject FrozenSetBuiltins::copy(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfFrozenSet(*self)) {
    return thread->raiseRequiresType(self, SymbolId::kFrozenSet);
  }
  FrozenSet set(&scope, *self);
  if (set.isFrozenSet()) {
    return *set;
  }
  return setCopy(thread, set);
}

RawObject FrozenSetBuiltins::dunderHash(Thread* thread, Frame* frame,
                                        word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfFrozenSet(*self)) {
    return thread->raiseRequiresType(self, SymbolId::kFrozenSet);
  }
  FrozenSet set(&scope, *self);
  return frozensetHash(thread, set);
}

RawObject FrozenSetBuiltins::dunderNew(Thread* thread, Frame* frame,
                                       word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object type_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfType(*type_obj)) {
    return thread->raiseWithFmt(LayoutId::kTypeError, "not a type object");
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
    result.setData(runtime->emptyTuple());
    return *result;
  }
  // Called with iterable, so iterate
  Object iterable(&scope, args.get(1));
  // frozenset(f) where f is a frozenset is idempotent
  if (iterable.isFrozenSet()) {
    return *iterable;
  }
  Object dunder_iter(
      &scope, Interpreter::lookupMethod(thread, thread->currentFrame(),
                                        iterable, SymbolId::kDunderIter));
  if (dunder_iter.isError()) {
    return thread->raiseWithFmt(
        LayoutId::kTypeError,
        "frozenset.__new__ must be called with an iterable");
  }
  FrozenSet result(&scope, runtime->newFrozenSet());
  result = setUpdate(thread, result, iterable);
  if (result.numItems() == 0) {
    return runtime->emptyFrozenSet();
  }
  return *result;
}

const BuiltinAttribute SetBuiltins::kAttributes[] = {
    {SymbolId::kInvalid, Set::kDataOffset},
    {SymbolId::kInvalid, Set::kNumItemsOffset},
    {SymbolId::kSentinelId, -1},
};

const BuiltinMethod SetBuiltins::kBuiltinMethods[] = {
    {SymbolId::kAdd, add},
    {SymbolId::kClear, clear},
    {SymbolId::kCopy, copy},
    {SymbolId::kDiscard, discard},
    {SymbolId::kDunderAnd, dunderAnd},
    {SymbolId::kDunderContains, dunderContains},
    {SymbolId::kDunderEq, dunderEq},
    {SymbolId::kDunderGe, dunderGe},
    {SymbolId::kDunderGt, dunderGt},
    {SymbolId::kDunderIand, dunderIand},
    {SymbolId::kDunderInit, dunderInit},
    {SymbolId::kDunderNew, dunderNew},
    {SymbolId::kDunderIter, dunderIter},
    {SymbolId::kDunderLe, dunderLe},
    {SymbolId::kDunderLen, dunderLen},
    {SymbolId::kDunderLt, dunderLt},
    {SymbolId::kDunderNe, dunderNe},
    {SymbolId::kDunderNew, dunderNew},
    {SymbolId::kIntersection, intersection},
    {SymbolId::kIsDisjoint, isDisjoint},
    {SymbolId::kPop, pop},
    {SymbolId::kRemove, remove},
    {SymbolId::kUpdate, update},
    {SymbolId::kSentinelId, nullptr},
};

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
  for (word i = SetBase::Bucket::kFirst;
       SetBase::Bucket::nextItem(*data, &i);) {
    value = SetBase::Bucket::value(*data, i);
    word hash = SetBase::Bucket::hash(*data, i);
    SetBase::Bucket::set(*new_data, i, hash, *value);
  }
  new_set.setData(*new_data);
  new_set.setNumItems(set.numItems());
  return *new_set;
}

bool setIsSubset(Thread* thread, const SetBase& set, const SetBase& other) {
  HandleScope scope(thread);
  Tuple data(&scope, set.data());
  Object value(&scope, NoneType::object());
  for (word i = SetBase::Bucket::kFirst;
       SetBase::Bucket::nextItem(*data, &i);) {
    value = RawSetBase::Bucket::value(*data, i);
    word hash = RawSetBase::Bucket::hash(*data, i);
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
    for (word i = SetBase::Bucket::kFirst;
         SetBase::Bucket::nextItem(*data, &i);) {
      Object value(&scope, Set::Bucket::value(*data, i));
      Set::Bucket::setTombstone(*data, i);
      set.setNumItems(num_items - 1);
      return *value;
    }
  }
  // num_items == 0 or all buckets were found empty
  return thread->raiseWithFmt(LayoutId::kKeyError, "pop from an empty set");
}

RawObject setIteratorNext(Thread* thread, const SetIterator& iter) {
  word idx = iter.index();
  HandleScope scope(thread);
  SetBase underlying(&scope, iter.iterable());
  Tuple data(&scope, underlying.data());
  // Find the next non empty bucket
  if (!SetBase::Bucket::nextItem(*data, &idx)) {
    return Error::noMoreItems();
  }
  iter.setConsumedCount(iter.consumedCount() + 1);
  iter.setIndex(idx);
  return RawSet::Bucket::value(*data, idx);
}

RawObject SetBuiltins::add(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfSet(*self)) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "'add' requires a 'set' object");
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

RawObject SetBuiltins::clear(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfSet(*self)) {
    return thread->raiseRequiresType(self, SymbolId::kSet);
  }
  Set set(&scope, *self);
  if (set.numItems() == 0) {
    return NoneType::object();
  }
  set.setNumItems(0);
  MutableTuple data(&scope, set.data());
  data.fill(NoneType::object());
  return NoneType::object();
}

RawObject SetBuiltins::copy(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfSet(*self)) {
    return thread->raiseRequiresType(self, SymbolId::kSet);
  }
  Set set(&scope, *self);
  return setCopy(thread, set);
}

RawObject SetBuiltins::discard(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfSet(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kSet);
  }
  Set self(&scope, *self_obj);
  Object key(&scope, args.get(1));
  Object hash_obj(&scope, Interpreter::hash(thread, key));
  if (hash_obj.isErrorException()) return *hash_obj;
  word hash = SmallInt::cast(*hash_obj).value();
  setRemove(thread, self, key, hash);
  return NoneType::object();
}

RawObject SetBuiltins::dunderIand(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  Object other(&scope, args.get(1));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfSet(*self)) {
    return thread->raiseRequiresType(self, SymbolId::kSet);
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
  return *set;
}

RawObject SetBuiltins::dunderInit(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  if (!runtime->isInstanceOfSet(*self)) {
    return thread->raiseRequiresType(self, SymbolId::kSet);
  }
  Set set(&scope, *self);
  Object iterable(&scope, args.get(1));
  Object result(&scope, setUpdate(thread, set, iterable));
  if (result.isError()) {
    return *result;
  }
  return NoneType::object();
}

RawObject SetBuiltins::pop(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfSet(*self)) {
    return thread->raiseRequiresType(self, SymbolId::kSet);
  }
  Set set(&scope, args.get(0));
  return setPop(thread, set);
}

RawObject SetBuiltins::remove(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfSet(*self)) {
    return thread->raiseRequiresType(self, SymbolId::kSet);
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

RawObject SetBuiltins::update(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Runtime* runtime = thread->runtime();
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfSet(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kSet);
  }
  Set self(&scope, *self_obj);
  Object starargs_obj(&scope, args.get(1));
  if (!runtime->isInstanceOfTuple(*starargs_obj)) {
    return thread->raiseRequiresType(starargs_obj, SymbolId::kTuple);
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

RawObject SetBuiltins::dunderNew(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object type_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfType(*type_obj)) {
    return thread->raiseWithFmt(LayoutId::kTypeError, "not a type object");
  }
  Type type(&scope, *type_obj);
  if (type.builtinBase() != LayoutId::kSet) {
    return thread->raiseWithFmt(LayoutId::kTypeError, "not a subtype of set");
  }
  Layout layout(&scope, type.instanceLayout());
  Set result(&scope, runtime->newInstance(layout));
  result.setNumItems(0);
  result.setData(runtime->emptyTuple());
  return *result;
}

const BuiltinMethod SetIteratorBuiltins::kBuiltinMethods[] = {
    {SymbolId::kDunderIter, dunderIter},
    {SymbolId::kDunderLengthHint, dunderLengthHint},
    {SymbolId::kDunderNext, dunderNext},
    {SymbolId::kSentinelId, nullptr},
};

RawObject SetIteratorBuiltins::dunderIter(Thread* thread, Frame* frame,
                                          word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isSetIterator()) {
    return thread->raiseRequiresType(self, SymbolId::kSetIterator);
  }
  return *self;
}

RawObject SetIteratorBuiltins::dunderNext(Thread* thread, Frame* frame,
                                          word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  if (!self_obj.isSetIterator()) {
    return thread->raiseRequiresType(self_obj, SymbolId::kSetIterator);
  }
  SetIterator self(&scope, *self_obj);
  Object value(&scope, setIteratorNext(thread, self));
  if (value.isError()) {
    return thread->raise(LayoutId::kStopIteration, NoneType::object());
  }
  return *value;
}

RawObject SetIteratorBuiltins::dunderLengthHint(Thread* thread, Frame* frame,
                                                word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isSetIterator()) {
    return thread->raiseRequiresType(self, SymbolId::kSetIterator);
  }
  SetIterator set_iterator(&scope, *self);
  Set set(&scope, set_iterator.iterable());
  return SmallInt::fromWord(set.numItems() - set_iterator.consumedCount());
}

}  // namespace py
