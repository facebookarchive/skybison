#include "set-builtins.h"

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"
#include "tuple-builtins.h"

namespace python {

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
  Object value(&scope, args.get(1));
  if (!thread->runtime()->isInstanceOfSetBase(*self)) {
    return thread->raiseWithFmt(
        LayoutId::kTypeError,
        "__contains__() requires a 'set' or 'frozenset' object");
  }
  SetBase set(&scope, *self);
  return Bool::fromBool(thread->runtime()->setIncludes(thread, set, value));
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
    SetIterator set_iter(&scope, runtime->newSetIterator(a));
    for (;;) {
      value = setIteratorNext(thread, set_iter);
      if (value.isError()) {
        break;
      }
      if (runtime->setIncludes(thread, b, value)) {
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
    if (runtime->setIncludes(thread, a, value)) {
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

RawObject FrozenSetBuiltins::dunderNew(Thread* thread, Frame* frame,
                                       word nargs) {
  Arguments args(frame, nargs);
  if (!args.get(0).isType()) {
    return thread->raiseWithFmt(LayoutId::kTypeError, "not a type object");
  }
  HandleScope scope(thread);
  Type type(&scope, args.get(0));
  if (type.builtinBase() != LayoutId::kFrozenSet) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "not a subtype of frozenset");
  }
  if (args.get(1).isUnbound()) {
    // Iterable not provided
    if (type.isBuiltin() && type.builtinBase() == LayoutId::kFrozenSet) {
      // Called with exact frozenset type, should return singleton
      return thread->runtime()->emptyFrozenSet();
    }
    // Not called with exact frozenset type, should return new distinct
    // frozenset
    Layout layout(&scope, type.instanceLayout());
    FrozenSet result(&scope, thread->runtime()->newInstance(layout));
    result.setNumItems(0);
    result.setData(thread->runtime()->emptyTuple());
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
  FrozenSet result(&scope, thread->runtime()->newFrozenSet());
  result = thread->runtime()->setUpdate(thread, result, iterable);
  if (result.numItems() == 0) {
    return thread->runtime()->emptyFrozenSet();
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

// TODO(T36810889): implement high-level setAdd function with error handling
RawObject setAdd(Thread* thread, const Set& set, const Object& value) {
  // TODO(T36756972): raise MemoryError when heap is full
  // TODO(T36757907): raise TypeError if value is unhashable
  return thread->runtime()->setAdd(thread, set, value);
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
  Tuple new_data(&scope, runtime->newTuple(data.length()));
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
  for (word i = SetBase::Bucket::kFirst;
       SetBase::Bucket::nextItem(*data, &i);) {
    value = RawSetBase::Bucket::value(*data, i);
    if (!thread->runtime()->setIncludes(thread, other, value)) {
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
  Object value(&scope, args.get(1));
  if (!thread->runtime()->isInstanceOfSet(*self)) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "'add' requires a 'set' object");
  }
  Set set(&scope, *self);

  Object result(&scope, setAdd(thread, set, value));
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
  Object value(&scope, args.get(1));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfSet(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kSet);
  }
  Set self(&scope, *self_obj);
  runtime->setRemove(thread, self, value);
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
  Object value(&scope, args.get(1));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfSet(*self)) {
    return thread->raiseRequiresType(self, SymbolId::kSet);
  }
  Set set(&scope, *self);
  if (!runtime->setRemove(thread, set, value)) {
    return thread->raise(LayoutId::kKeyError, *value);
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
  if (!args.get(0).isType()) {
    return thread->raiseWithFmt(LayoutId::kTypeError, "not a type object");
  }
  HandleScope scope(thread);
  Type type(&scope, args.get(0));
  if (type.builtinBase() != LayoutId::kSet) {
    return thread->raiseWithFmt(LayoutId::kTypeError, "not a subtype of set");
  }
  Layout layout(&scope, type.instanceLayout());
  Set result(&scope, thread->runtime()->newInstance(layout));
  result.setNumItems(0);
  result.setData(thread->runtime()->emptyTuple());
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

}  // namespace python
