#include "set-builtins.h"

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"
#include "trampolines-inl.h"

namespace python {

RawObject SetBaseBuiltins::dunderLen(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("__len__() takes no arguments");
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfSetBase(*self)) {
    return thread->raiseTypeErrorWithCStr(
        "'__len__' requires a 'set' or 'frozenset' object");
  }
  SetBase set(&scope, *self);
  return SmallInt::fromWord(set.numItems());
}

RawObject SetBaseBuiltins::dunderContains(Thread* thread, Frame* frame,
                                          word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("__contains__ takes 1 argument");
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  Object value(&scope, args.get(1));
  if (!thread->runtime()->isInstanceOfSetBase(*self)) {
    return thread->raiseTypeErrorWithCStr(
        "__contains__() requires a 'set' or 'frozenset' object");
  }
  SetBase set(&scope, *self);
  return Bool::fromBool(thread->runtime()->setIncludes(set, value));
}

RawObject SetBaseBuiltins::dunderIter(Thread* thread, Frame* frame,
                                      word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("__iter__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfSetBase(*self)) {
    return thread->raiseTypeErrorWithCStr(
        "__iter__() must be called with a 'set' or 'frozenset' instance as the "
        "first argument");
  }
  return thread->runtime()->newSetIterator(self);
}

RawObject SetBaseBuiltins::isDisjoint(Thread* thread, Frame* frame,
                                      word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr(
        "isdisjoint() takes exactly one argument");
  }
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  Object other(&scope, args.get(1));
  Object value(&scope, NoneType::object());
  if (!thread->runtime()->isInstanceOfSetBase(*self)) {
    return thread->raiseTypeErrorWithCStr(
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
      if (runtime->setIncludes(b, value)) {
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
    return thread->raiseTypeErrorWithCStr("object is not iterable");
  }
  Object iterator(&scope,
                  Interpreter::callMethod1(thread, thread->currentFrame(),
                                           iter_method, other));
  if (iterator.isError()) {
    return thread->raiseTypeErrorWithCStr("object is not iterable");
  }
  Object next_method(
      &scope, Interpreter::lookupMethod(thread, thread->currentFrame(),
                                        iterator, SymbolId::kDunderNext));
  if (next_method.isError()) {
    return thread->raiseTypeErrorWithCStr("iter() returned a non-iterator");
  }
  for (;;) {
    value = Interpreter::callMethod1(thread, thread->currentFrame(),
                                     next_method, iterator);
    if (value.isError()) {
      if (thread->clearPendingStopIteration()) break;
      return *value;
    }
    if (runtime->setIncludes(a, value)) {
      return Bool::falseObj();
    }
  }
  return Bool::trueObj();
}

RawObject SetBaseBuiltins::dunderAnd(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("__and__ takes 1 argument");
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  Object other(&scope, args.get(1));
  if (!thread->runtime()->isInstanceOfSetBase(*self)) {
    return thread->raiseTypeErrorWithCStr("__and__() requires a 'set' object");
  }
  if (!thread->runtime()->isInstanceOfSetBase(*other)) {
    return thread->runtime()->notImplemented();
  }
  SetBase set(&scope, *self);
  SetBase other_set(&scope, *other);
  return thread->runtime()->setIntersection(thread, set, other_set);
}

RawObject SetBaseBuiltins::intersection(Thread* thread, Frame* frame,
                                        word nargs) {
  if (nargs == 0) {
    return thread->raiseTypeErrorWithCStr(
        "intersection() of 'set' object needs an argument");
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfSetBase(*self)) {
    return thread->raiseTypeErrorWithCStr(
        "intersection() requires a 'set' or 'frozenset' object");
  }
  SetBase set(&scope, *self);
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
  if (nargs == 0) {
    return thread->raiseTypeErrorWithCStr(
        "__eq__() of 'set' object needs an argument");
  }
  if (nargs != 2) {
    return thread->raiseTypeError(
        runtime->newStrFromFormat("expected 1 arguments, got %ld", nargs - 1));
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  Object other(&scope, args.get(1));
  if (!runtime->isInstanceOfSetBase(*self)) {
    return thread->raiseTypeErrorWithCStr(
        "__eq__() requires a 'set' or 'frozenset' object");
  }
  if (!runtime->isInstanceOfSetBase(*other)) {
    return runtime->notImplemented();
  }
  SetBase set(&scope, *self);
  SetBase other_set(&scope, *other);
  return Bool::fromBool(setEquals(thread, set, other_set));
}

RawObject SetBaseBuiltins::dunderNe(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  if (nargs == 0) {
    return thread->raiseTypeErrorWithCStr(
        "__ne__() of 'set' object needs an argument");
  }
  if (nargs != 2) {
    return thread->raiseTypeError(
        runtime->newStrFromFormat("expected 1 argument, got %ld", nargs - 1));
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  Object other(&scope, args.get(1));
  if (!runtime->isInstanceOfSetBase(*self)) {
    return thread->raiseTypeErrorWithCStr(
        "__ne__() requires a 'set' or 'frozenset' object");
  }
  if (!runtime->isInstanceOfSetBase(*other)) {
    return runtime->notImplemented();
  }
  SetBase set(&scope, *self);
  SetBase other_set(&scope, *other);
  return Bool::fromBool(!setEquals(thread, set, other_set));
}

RawObject SetBaseBuiltins::dunderLe(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  if (nargs == 0) {
    return thread->raiseTypeErrorWithCStr(
        "__le__() of 'set' object needs an argument");
  }
  if (nargs != 2) {
    return thread->raiseTypeError(
        runtime->newStrFromFormat("expected 1 argument, got %ld", nargs - 1));
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  Object other(&scope, args.get(1));
  if (!runtime->isInstanceOfSetBase(*self)) {
    return thread->raiseTypeErrorWithCStr(
        "__le__() requires a 'set' or 'frozenset' object");
  }
  if (!runtime->isInstanceOfSetBase(*other)) {
    return runtime->notImplemented();
  }
  SetBase set(&scope, *self);
  SetBase other_set(&scope, *other);
  return Bool::fromBool(setIsSubset(thread, set, other_set));
}

RawObject SetBaseBuiltins::dunderLt(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  if (nargs == 0) {
    return thread->raiseTypeErrorWithCStr(
        "__lt__() of 'set' object needs an argument");
  }
  if (nargs != 2) {
    return thread->raiseTypeError(
        runtime->newStrFromFormat("expected 1 argument, got %ld", nargs - 1));
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  Object other(&scope, args.get(1));
  if (!runtime->isInstanceOfSetBase(*self)) {
    return thread->raiseTypeErrorWithCStr(
        "__lt__() requires a 'set' or 'frozenset' object");
  }
  if (!runtime->isInstanceOfSetBase(*other)) {
    return runtime->notImplemented();
  }
  SetBase set(&scope, *self);
  SetBase other_set(&scope, *other);
  return Bool::fromBool(setIsProperSubset(thread, set, other_set));
}

RawObject SetBaseBuiltins::dunderGe(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  if (nargs == 0) {
    return thread->raiseTypeErrorWithCStr(
        "__ge__() of 'set' object needs an argument");
  }
  if (nargs != 2) {
    return thread->raiseTypeError(
        runtime->newStrFromFormat("expected 1 argument, got %ld", nargs - 1));
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  Object other(&scope, args.get(1));
  if (!runtime->isInstanceOfSetBase(*self)) {
    return thread->raiseTypeErrorWithCStr(
        "__ge__() requires a 'set' or 'frozenset' object");
  }
  if (!runtime->isInstanceOfSetBase(*other)) {
    return runtime->notImplemented();
  }
  SetBase set(&scope, *self);
  SetBase other_set(&scope, *other);
  return Bool::fromBool(setIsSubset(thread, other_set, set));
}

RawObject SetBaseBuiltins::dunderGt(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  if (nargs == 0) {
    return thread->raiseTypeErrorWithCStr(
        "__gt__() of 'set' object needs an argument");
  }
  if (nargs != 2) {
    return thread->raiseTypeError(
        runtime->newStrFromFormat("expected 1 argument, got %ld", nargs - 1));
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  Object other(&scope, args.get(1));
  if (!runtime->isInstanceOfSetBase(*self)) {
    return thread->raiseTypeErrorWithCStr(
        "__gt__() requires a 'set' or 'frozenset' object");
  }
  if (!runtime->isInstanceOfSetBase(*other)) {
    return runtime->notImplemented();
  }
  SetBase set(&scope, *self);
  SetBase other_set(&scope, *other);
  return Bool::fromBool(setIsProperSubset(thread, other_set, set));
}

const BuiltinAttribute FrozenSetBuiltins::kAttributes[] = {
    {SymbolId::kItems, Set::kDataOffset},
    {SymbolId::kAllocated, Set::kNumItemsOffset},
};

const BuiltinMethod FrozenSetBuiltins::kMethods[] = {
    {SymbolId::kDunderAnd, nativeTrampoline<dunderAnd>},
    {SymbolId::kDunderContains, nativeTrampoline<dunderContains>},
    {SymbolId::kDunderEq, nativeTrampoline<dunderEq>},
    {SymbolId::kDunderGe, nativeTrampoline<dunderGe>},
    {SymbolId::kDunderGt, nativeTrampoline<dunderGt>},
    {SymbolId::kDunderIter, nativeTrampoline<dunderIter>},
    {SymbolId::kDunderLe, nativeTrampoline<dunderLe>},
    {SymbolId::kDunderLen, nativeTrampoline<dunderLen>},
    {SymbolId::kDunderLt, nativeTrampoline<dunderLt>},
    {SymbolId::kDunderNe, nativeTrampoline<dunderNe>},
    {SymbolId::kDunderNew, nativeTrampoline<dunderNew>},
    {SymbolId::kIsDisjoint, nativeTrampoline<isDisjoint>},
    {SymbolId::kIntersection, nativeTrampoline<intersection>}};

void FrozenSetBuiltins::initialize(Runtime* runtime) {
  HandleScope scope;
  Type frozen_set(&scope, runtime->addBuiltinType(
                              SymbolId::kFrozenSet, LayoutId::kFrozenSet,
                              LayoutId::kObject, kAttributes, kMethods));
  frozen_set.sealAttributes();
}

RawObject FrozenSetBuiltins::dunderNew(Thread* thread, Frame* frame,
                                       word nargs) {
  if (nargs < 1 || nargs > 2) {
    return thread->raiseTypeErrorWithCStr(
        "frozenset.__new__ expects 1-2 arguments");
  }
  Arguments args(frame, nargs);
  if (!args.get(0)->isType()) {
    return thread->raiseTypeErrorWithCStr("not a type object");
  }
  HandleScope scope(thread);
  Type type(&scope, args.get(0));
  if (type.builtinBase() != LayoutId::kFrozenSet) {
    return thread->raiseTypeErrorWithCStr("not a subtype of frozenset");
  }
  if (nargs == 1 && type.isBuiltin() &&
      type.builtinBase() == LayoutId::kFrozenSet) {
    return thread->runtime()->emptyFrozenSet();
  }
  if (nargs > 1) {
    Object iterable(&scope, args.get(1));
    // frozenset(f) where f is a frozenset is idempotent
    if (iterable.isFrozenSet()) {
      return *iterable;
    }
    Object dunder_iter(
        &scope, Interpreter::lookupMethod(thread, thread->currentFrame(),
                                          iterable, SymbolId::kDunderIter));
    if (dunder_iter.isError()) {
      return thread->raiseTypeErrorWithCStr(
          "frozenset.__new__ must be called with an iterable");
    }
    FrozenSet result(&scope, thread->runtime()->newFrozenSet());
    result = thread->runtime()->setUpdate(thread, result, iterable);
    if (result.numItems() == 0) {
      return thread->runtime()->emptyFrozenSet();
    }
    return *result;
  }
  Layout layout(&scope, type.instanceLayout());
  FrozenSet result(&scope, thread->runtime()->newInstance(layout));
  result.setNumItems(0);
  result.setData(thread->runtime()->newTuple(0));
  return *result;
}

const BuiltinAttribute SetBuiltins::kAttributes[] = {
    {SymbolId::kItems, Set::kDataOffset},
    {SymbolId::kAllocated, Set::kNumItemsOffset},
};

const BuiltinMethod SetBuiltins::kMethods[] = {
    {SymbolId::kAdd, nativeTrampoline<add>},
    {SymbolId::kDunderAnd, nativeTrampoline<dunderAnd>},
    {SymbolId::kDunderContains, nativeTrampoline<dunderContains>},
    {SymbolId::kDunderEq, nativeTrampoline<dunderEq>},
    {SymbolId::kDunderGe, nativeTrampoline<dunderGe>},
    {SymbolId::kDunderGt, nativeTrampoline<dunderGt>},
    {SymbolId::kDunderIand, nativeTrampoline<dunderIand>},
    {SymbolId::kDunderInit, nativeTrampoline<dunderInit>},
    {SymbolId::kDunderIter, nativeTrampoline<dunderIter>},
    {SymbolId::kDunderLe, nativeTrampoline<dunderLe>},
    {SymbolId::kDunderLen, nativeTrampoline<dunderLen>},
    {SymbolId::kDunderLt, nativeTrampoline<dunderLt>},
    {SymbolId::kDunderNe, nativeTrampoline<dunderNe>},
    {SymbolId::kDunderNew, nativeTrampoline<dunderNew>},
    {SymbolId::kIsDisjoint, nativeTrampoline<isDisjoint>},
    {SymbolId::kIntersection, nativeTrampoline<intersection>},
    {SymbolId::kPop, nativeTrampoline<pop>}};

void SetBuiltins::initialize(Runtime* runtime) {
  HandleScope scope;
  Type set(&scope,
           runtime->addBuiltinType(SymbolId::kSet, LayoutId::kSet,
                                   LayoutId::kObject, kAttributes, kMethods));
  set.sealAttributes();
  ;
}

// TODO(T36810889): implement high-level setAdd function with error handling
RawObject setAdd(Thread* thread, const Set& set, const Object& key) {
  // TODO(T36756972): raise MemoryError when heap is full
  // TODO(T36757907): raise TypeError if key is unhashable
  return thread->runtime()->setAdd(set, key);
}

RawObject setCopy(Thread* thread, const SetBase& set) {
  word num_items = set.numItems();
  Runtime* runtime = thread->runtime();
  if (num_items == 0) {
    return runtime->isInstanceOfSet(*set) ? runtime->newSet()
                                          : runtime->emptyFrozenSet();
  }

  HandleScope scope;
  SetBase new_set(&scope, runtime->isInstanceOfSet(*set)
                              ? runtime->newSet()
                              : runtime->newFrozenSet());
  Tuple data(&scope, set.data());
  Tuple new_data(&scope, runtime->newTuple(data.length()));
  Object key(&scope, NoneType::object());
  Object key_hash(&scope, NoneType::object());
  for (word i = SetBase::Bucket::kFirst;
       SetBase::Bucket::nextItem(*data, &i);) {
    key = SetBase::Bucket::key(*data, i);
    key_hash = SetBase::Bucket::hash(*data, i);
    SetBase::Bucket::set(*new_data, i, *key_hash, *key);
  }
  new_set.setData(*new_data);
  new_set.setNumItems(set.numItems());
  return *new_set;
}

bool setIsSubset(Thread* thread, const SetBase& set, const SetBase& other) {
  HandleScope scope(thread);
  Tuple data(&scope, set.data());
  Object key(&scope, NoneType::object());
  for (word i = SetBase::Bucket::kFirst;
       SetBase::Bucket::nextItem(*data, &i);) {
    key = RawSetBase::Bucket::key(*data, i);
    if (!thread->runtime()->setIncludes(other, key)) {
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
      Object value(&scope, Set::Bucket::key(*data, i));
      Set::Bucket::setTombstone(*data, i);
      set.setNumItems(num_items - 1);
      return *value;
    }
  }
  // num_items == 0 or all buckets were found empty
  return thread->raiseKeyErrorWithCStr("pop from an empty set");
}

RawObject setIteratorNext(Thread* thread, const SetIterator& iter) {
  word idx = iter.index();
  HandleScope scope(thread);
  SetBase underlying(&scope, iter.iterable());
  Tuple data(&scope, underlying.data());
  // Find the next non empty bucket
  if (!SetBase::Bucket::nextItem(*data, &idx)) {
    return Error::object();
  }
  iter.setConsumedCount(iter.consumedCount() + 1);
  iter.setIndex(idx);
  return RawSet::Bucket::key(*data, idx);
}

RawObject SetBuiltins::add(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("add() takes exactly one argument");
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  Object key(&scope, args.get(1));
  if (!thread->runtime()->isInstanceOfSet(*self)) {
    return thread->raiseTypeErrorWithCStr("'add' requires a 'set' object");
  }
  Set set(&scope, *self);

  if (setAdd(thread, set, key)->isError()) {
    return Error::object();
  }

  return NoneType::object();
}

RawObject SetBuiltins::dunderIand(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("__iand__ takes 1 argument");
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  Object other(&scope, args.get(1));
  if (!thread->runtime()->isInstanceOfSet(*self)) {
    return thread->raiseTypeErrorWithCStr("__iand__() requires a 'set' object");
  }
  if (!thread->runtime()->isInstanceOfSet(*other)) {
    return thread->runtime()->notImplemented();
  }
  Set set(&scope, *self);
  Object intersection(&scope,
                      thread->runtime()->setIntersection(thread, set, other));
  if (intersection.isError()) {
    return *intersection;
  }
  RawSet intersection_set = RawSet::cast(*intersection);
  set.setData(intersection_set->data());
  set.setNumItems(intersection_set->numItems());
  return *set;
}

RawObject SetBuiltins::dunderInit(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  if (nargs == 0) {
    return thread->raiseTypeErrorWithCStr(
        "__init__() of 'set' object needs an argument");
  }
  if (nargs > 2) {
    return thread->raiseTypeError(runtime->newStrFromFormat(
        "set expected at most 1 argument, got %ld", nargs - 1));
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  if (!runtime->isInstanceOfSet(*self)) {
    return thread->raiseTypeErrorWithCStr("__init__() requires a 'set' object");
  }
  if (nargs == 2) {
    Set set(&scope, *self);
    Object iterable(&scope, args.get(1));
    Object result(&scope, runtime->setUpdate(thread, set, iterable));
    if (result.isError()) {
      return *result;
    }
  }
  return NoneType::object();
}

RawObject SetBuiltins::pop(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("pop() takes no arguments");
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfSet(*self)) {
    return thread->raiseTypeErrorWithCStr("pop() requires a 'set' object");
  }
  Set set(&scope, args.get(0));
  return setPop(thread, set);
}

RawObject SetBuiltins::dunderNew(Thread* thread, Frame* frame, word nargs) {
  if (nargs < 1) {
    return thread->raiseTypeErrorWithCStr(
        "set.__new__ expects at least 1 argument");
  }
  Arguments args(frame, nargs);
  if (!args.get(0)->isType()) {
    return thread->raiseTypeErrorWithCStr("not a type object");
  }
  HandleScope scope(thread);
  Type type(&scope, args.get(0));
  if (type.builtinBase() != LayoutId::kSet) {
    return thread->raiseTypeErrorWithCStr("not a subtype of set");
  }
  Layout layout(&scope, type.instanceLayout());
  Set result(&scope, thread->runtime()->newInstance(layout));
  result.setNumItems(0);
  result.setData(thread->runtime()->newTuple(0));
  return *result;
}

const BuiltinMethod SetIteratorBuiltins::kMethods[] = {
    {SymbolId::kDunderIter, nativeTrampoline<dunderIter>},
    {SymbolId::kDunderNext, nativeTrampoline<dunderNext>},
    {SymbolId::kDunderLengthHint, nativeTrampoline<dunderLengthHint>},
};

void SetIteratorBuiltins::initialize(Runtime* runtime) {
  HandleScope scope;
  Type set_iter(&scope, runtime->addBuiltinTypeWithMethods(
                            SymbolId::kSetIterator, LayoutId::kSetIterator,
                            LayoutId::kObject, kMethods));
}

RawObject SetIteratorBuiltins::dunderIter(Thread* thread, Frame* frame,
                                          word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("__iter__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isSetIterator()) {
    return thread->raiseTypeErrorWithCStr(
        "__iter__() must be called with a set iterator instance as the first "
        "argument");
  }
  return *self;
}

RawObject SetIteratorBuiltins::dunderNext(Thread* thread, Frame* frame,
                                          word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("__next__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  if (!self_obj.isSetIterator()) {
    return thread->raiseTypeErrorWithCStr(
        "__next__() must be called with a set iterator instance as the first "
        "argument");
  }
  SetIterator self(&scope, *self_obj);
  Object value(&scope, setIteratorNext(thread, self));
  if (value.isError()) {
    return thread->raiseStopIteration(NoneType::object());
  }
  return *value;
}

RawObject SetIteratorBuiltins::dunderLengthHint(Thread* thread, Frame* frame,
                                                word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr(
        "__length_hint__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isSetIterator()) {
    return thread->raiseTypeErrorWithCStr(
        "__length_hint__() must be called with a set iterator instance as "
        "the first argument");
  }
  SetIterator set_iterator(&scope, *self);
  Set set(&scope, set_iterator.iterable());
  return SmallInt::fromWord(set.numItems() - set_iterator.consumedCount());
}

}  // namespace python
