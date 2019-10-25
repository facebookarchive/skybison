#include "set-builtins.h"

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"
#include "tuple-builtins.h"

namespace py {

RawSmallInt frozensetHash(Thread* thread, const Object& frozenset) {
  HandleScope scope(thread);
  FrozenSet set(&scope, *frozenset);
  Tuple data(&scope, set.data());

  uword result = 0;
  for (word i = SetBase::Bucket::kFirst;
       SetBase::Bucket::nextItem(*data, &i);) {
    word value_hash = SmallInt::cast(SetBase::Bucket::hash(*data, i)).value();
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
  Object key_hash(&scope, Interpreter::hash(thread, key));
  if (key_hash.isErrorException()) return *key_hash;
  return Bool::fromBool(
      thread->runtime()->setIncludes(thread, set, key, key_hash));
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
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  Object other(&scope, args.get(1));
  Object value(&scope, NoneType::object());
  Object value_hash(&scope, NoneType::object());
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
      value_hash = SetBase::Bucket::hash(*data, i);
      if (runtime->setIncludes(thread, b, value, value_hash)) {
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
  for (;;) {
    value = Interpreter::callMethod1(thread, thread->currentFrame(),
                                     next_method, iterator);
    if (value.isError()) {
      if (thread->clearPendingStopIteration()) break;
      return *value;
    }
    value_hash = Interpreter::hash(thread, value);
    if (value_hash.isErrorException()) return *value_hash;
    if (runtime->setIncludes(thread, a, value, value_hash)) {
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
  return thread->runtime()->setIntersection(thread, set, other_set);
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
  Object result(&scope, thread->runtime()->setIntersection(thread, set, other));
  if (result.isError() || nargs == 2) {
    return *result;
  }

  SetBase base(&scope, *result);
  for (word i = 2; i < nargs; i++) {
    other = args.get(i);
    result = thread->runtime()->setIntersection(thread, base, other);
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
  result = runtime->setUpdate(thread, result, iterable);
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
  Object value_hash(&scope, NoneType::object());
  for (word i = SetBase::Bucket::kFirst;
       SetBase::Bucket::nextItem(*data, &i);) {
    value = SetBase::Bucket::value(*data, i);
    value_hash = SetBase::Bucket::hash(*data, i);
    SetBase::Bucket::set(*new_data, i, *value_hash, *value);
  }
  new_set.setData(*new_data);
  new_set.setNumItems(set.numItems());
  return *new_set;
}

bool setIsSubset(Thread* thread, const SetBase& set, const SetBase& other) {
  HandleScope scope(thread);
  Tuple data(&scope, set.data());
  Object value(&scope, NoneType::object());
  Object value_hash(&scope, NoneType::object());
  for (word i = SetBase::Bucket::kFirst;
       SetBase::Bucket::nextItem(*data, &i);) {
    value = RawSetBase::Bucket::value(*data, i);
    value_hash = RawSetBase::Bucket::hash(*data, i);
    if (!thread->runtime()->setIncludes(thread, other, value, value_hash)) {
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
  Object value_hash(&scope, Interpreter::hash(thread, value));
  if (value_hash.isErrorException()) return *value_hash;

  Object result(&scope, runtime->setAdd(thread, set, value, value_hash));
  if (result.isError()) return *result;
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
  Object key_hash(&scope, Interpreter::hash(thread, key));
  if (key_hash.isErrorException()) return *key_hash;
  runtime->setRemove(thread, self, key, key_hash);
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
  Object intersection(&scope,
                      thread->runtime()->setIntersection(thread, set, other));
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
  Object result(&scope, runtime->setUpdate(thread, set, iterable));
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
  Object key_hash(&scope, Interpreter::hash(thread, key));
  if (key_hash.isErrorException()) return *key_hash;
  if (!runtime->setRemove(thread, set, key, key_hash)) {
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
  Tuple starargs(&scope, tupleUnderlying(thread, starargs_obj));
  Object result(&scope, NoneType::object());
  for (word i = 0; i < starargs.length(); i++) {
    Object other(&scope, starargs.at(i));
    result = runtime->setUpdate(thread, self, other);
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
